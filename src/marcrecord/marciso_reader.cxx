/*
 * Copyright (c) 2013, Alexander Fronkin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cerrno>
#include <cstdio>
#include <cstring>
#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marciso_reader.h"

namespace marcrecord {

#define ISO2709_RECORD_SEPARATOR	'\x1D'
#define ISO2709_FIELD_SEPARATOR		'\x1E'
#define ISO2709_IDENTIFIER_DELIMITER	'\x1F'

#pragma pack(1)

/* Structure of record directory entry. */
struct RecordDirectoryEntry {
	// Field tag.
	char fieldTag[3];
	// Field length.
	char fieldLength[4];
	// Field starting position.
	char fieldStartingPosition[5];
};
typedef struct RecordDirectoryEntry RecordDirectoryEntry;

#pragma pack()

} // namespace marcrecord

using namespace marcrecord;

/*
 * Constructor.
 */
MarcIsoReader::MarcIsoReader(FILE *inputFile, const char *inputEncoding)
	: MarcReader()
{
	// Clear member variables.
	m_iconvDesc = (iconv_t) -1;

	if (inputFile) {
		// Open input file.
		open(inputFile, inputEncoding);
	} else {
		// Clear object state.
		close();
	}
}

/*
 * Destructor.
 */
MarcIsoReader::~MarcIsoReader()
{
	// Close input file.
	close();
}

/*
 * Open input file.
 */
bool
MarcIsoReader::open(FILE *inputFile, const char *inputEncoding)
{
	// Clear error code and message.
	m_errorCode = OK;
	m_errorMessage = "";

	// Initialize input stream parameters.
	m_inputFile = inputFile == NULL ? stdin : inputFile;
	m_inputEncoding = inputEncoding == NULL ? "" : inputEncoding;

	// Initialize encoding conversion.
	if (inputEncoding == NULL
		|| strcmp(inputEncoding, "UTF-8") == 0
		|| strcmp(inputEncoding, "utf-8") == 0)
	{
		m_iconvDesc = (iconv_t) -1;
	} else {
		// Create iconv descriptor for input encoding conversion.
		m_iconvDesc = iconv_open("UTF-8", inputEncoding);
		if (m_iconvDesc == (iconv_t) -1) {
			m_errorCode = ERROR_ICONV;
			if (errno == EINVAL) {
				m_errorMessage =
					"encoding conversion is not supported";
			} else {
				m_errorMessage = "iconv initialization failed";
			}
			return false;
		}
	}

	return true;
}

/*
 * Close input file.
 */
void
MarcIsoReader::close(void)
{
	// Finalize iconv.
	if (m_iconvDesc != (iconv_t) -1) {
		iconv_close(m_iconvDesc);
	}

	// Clear member variables.
	m_errorCode = OK;
	m_errorMessage = "";
	m_inputFile = NULL;
	m_inputEncoding = "";
	m_iconvDesc = (iconv_t) -1;
	m_autoCorrectionMode = false;
}

/*
 * Read next record from MARCXML file.
 */
bool
MarcIsoReader::next(MarcRecord &record)
{
	int symbol;
	char recordBuf[100000];
	unsigned int recordLen;

	// Clear error code and message.
	m_errorCode = OK;
	m_errorMessage = "";

	if (!m_autoCorrectionMode) {
		// Read record length.
		if (fread(recordBuf, 1, 5, m_inputFile) != 5) {
			m_errorCode = END_OF_FILE;
			return false;
		}

		// Parse record length.
		if (!is_numeric(recordBuf, 5)
			|| sscanf(recordBuf, "%5u", &recordLen) != 1)
		{
			// Skip until record separator.
			do {
				symbol = fgetc(m_inputFile);
			} while (symbol >= 0 && symbol != ISO2709_RECORD_SEPARATOR);

			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid record length";
			return false;
		}

		// Read record.
		if (fread(recordBuf + 5, 1, recordLen - 5, m_inputFile)
			!= recordLen - 5)
		{
			// Skip until record separator.
			do {
				symbol = fgetc(m_inputFile);
			} while (!feof(m_inputFile) && symbol != ISO2709_RECORD_SEPARATOR);

			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage =
				"invalid record length or record data incomplete";
			return false;
		}
	} else {
		// Read record until record separator.
		char* recordBufPtr = recordBuf;
		recordLen = 0;
		do {
			symbol = fgetc(m_inputFile);
			if (!feof(m_inputFile)) {
				*recordBufPtr = static_cast<char>(symbol);
				recordBufPtr++;
				recordLen++;
			}
		} while (!feof(m_inputFile) && symbol != ISO2709_RECORD_SEPARATOR);

		if (feof(m_inputFile)) {
			m_errorCode = END_OF_FILE;
			return false;
		}
 		if (recordLen == 0) {
			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid record length";
			return false;
 		}

		// Replace record length.
		char lengthBuf[6];
		sprintf(lengthBuf, "%05d", recordLen);
		memcpy(recordBuf, lengthBuf, 5);
	}

	// Parse record.
	return parse(recordBuf, recordLen, record);
}

/*
 * Parse record from ISO 2709 buffer.
 */
bool
MarcIsoReader::parse(const char *recordBuf, unsigned int recordBufLen,
	MarcRecord &record)
{
	// Clear error code and message.
	m_errorCode = OK;
	m_errorMessage = "";

	// Clear current record data.
	record.clear();

	try {
		// Check record length.
		unsigned int recordLen;
		if (!is_numeric(recordBuf, 5)
			|| sscanf(recordBuf, "%5u", &recordLen) != 1
			|| recordLen != recordBufLen
			|| recordLen < sizeof(MarcRecord::Leader))
		{
			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid record length";
			throw m_errorCode;
		}

		// Copy record leader.
		memcpy(&record.m_leader, recordBuf,
			sizeof(MarcRecord::Leader));

		// Replace incorrect characters in record leader to '?'.
		if (m_autoCorrectionMode) {
			unsigned int i = 0;
			for (; i < sizeof(MarcRecord::Leader); i++) {
				char c = *((char *) &record.m_leader + i);
				if ((c != ' ') && (c != '|')
					&& (c < '0' || c > '9')
					&& (c < 'a' || c > 'z'))
				{
					*((char *) &record.m_leader + i) = '?';
				}
			}
		}

		// Get base address of data.
		unsigned int baseAddress;
		if (!m_autoCorrectionMode) {
			if (!is_numeric(record.m_leader.baseAddress, 5)
				|| sscanf(record.m_leader.baseAddress, "%05u",
					&baseAddress) != 1
				|| recordLen < baseAddress)
			{
				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "invalid base address of data";
				throw m_errorCode;
			}
		} else {
			baseAddress = 24;
			while (baseAddress < recordLen -1
				&& recordBuf[baseAddress] != ISO2709_FIELD_SEPARATOR)
			{
				baseAddress++;
			}
			if (recordBuf[baseAddress] != ISO2709_FIELD_SEPARATOR) {
				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "base address of data cannot be found";
				throw m_errorCode;
			}
			baseAddress++;
		}

		// Get number of fields.
		int numFields = (baseAddress - sizeof(MarcRecord::Leader) - 1)
			/ sizeof(RecordDirectoryEntry);
		if (recordLen < sizeof(MarcRecord::Leader)
			+ (sizeof(RecordDirectoryEntry) * numFields))
		{
			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid record length";
			throw m_errorCode;
		}

		// Parse list of fields.
		RecordDirectoryEntry *directoryEntry = 
			(RecordDirectoryEntry *) (recordBuf
			+ sizeof(MarcRecord::Leader));
		const char *recordData = recordBuf + baseAddress;
		unsigned int recordDataPos = baseAddress;
		int fieldNo = 0;
		for (; fieldNo < numFields; fieldNo++, directoryEntry++) {
			std::string fieldTag(directoryEntry->fieldTag, 0, 3);
			unsigned int fieldLength, fieldStartPos;
			if (!m_autoCorrectionMode) {
				// Check directory entry.
				if (!is_numeric((const char *) directoryEntry,
					sizeof(RecordDirectoryEntry)))
				{
					std::string errorPos;
					snprintf(errorPos, 11, "%d",
						(char *) directoryEntry - recordBuf);

					m_errorCode = ERROR_INVALID_RECORD;
					m_errorMessage = "invalid directory entry at "
						+ errorPos;
					throw m_errorCode;
				}

				// Parse directory entry.
				if (sscanf(directoryEntry->fieldLength, "%4u%5u",
					&fieldLength, &fieldStartPos) != 2)
				{
					std::string errorPos;
					snprintf(errorPos, 11, "%d",
						(char *) directoryEntry->fieldLength
						- recordBuf);

					m_errorCode = ERROR_INVALID_RECORD;
					m_errorMessage = 
						"invalid base address of data at "
						+ errorPos;
					throw m_errorCode;
				}
			} else {
				fieldStartPos = recordDataPos - baseAddress;
				while (recordDataPos < recordLen -1
					&& recordBuf[recordDataPos] != ISO2709_FIELD_SEPARATOR)
				{
					recordDataPos++;
				}
				if (recordBuf[recordDataPos] != ISO2709_FIELD_SEPARATOR) {
					break;
				}
				fieldLength = recordDataPos - baseAddress - fieldStartPos + 1;
				recordDataPos++;
			}

			// Check field starting position and length.
			unsigned int fieldEndPos =
			       baseAddress + fieldStartPos + fieldLength;
			if (fieldEndPos > recordLen
				|| (fieldTag < "010" && fieldLength < 2))
			{
				std::string errorPos;
				if (!m_autoCorrectionMode) {
					snprintf(errorPos, 11, "%d",
						(char *) directoryEntry->fieldLength
						- recordBuf);
				} else {
					snprintf(errorPos, 11, "%d",
						fieldStartPos);
				}

				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "invalid field starting "
					"position or length at " + errorPos;
				throw m_errorCode;
			}

			// Check data field length.
			if (fieldTag < "010" && fieldLength < 2) {
				std::string errorPos;
				snprintf(errorPos, 11, "%d",
					(char *) directoryEntry->fieldLength
					- recordBuf);

				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "invalid length of data field"
					" at " + errorPos;
				throw m_errorCode;
			}

			// Parse field.
			MarcRecord::Field field = parseField(fieldTag,
				recordData + fieldStartPos, fieldLength,
				baseAddress + fieldStartPos);
			// Append field to list.
			record.m_fieldList.push_back(field);
		}
	} catch (ErrorCode errorCode) {
		record.clear();
		return false;
	}

	return true;
}

/*
 * Parse field from ISO 2709 buffer.
 */
MarcRecord::Field
MarcIsoReader::parseField(const std::string &fieldTag, const char *fieldData,
	unsigned int fieldLength, unsigned int fieldAbsoluteStartPos)
{
	MarcRecord::Field field;

	// Adjust field length.
	if (fieldData[fieldLength - 1] == '\x1E') {
		fieldLength--;
	}

	// Copy field tag.
	field.m_tag = fieldTag;

	// Replace incorrect characters in field tag to '?'.
	if (m_autoCorrectionMode) {
		for (std::string::iterator it = field.m_tag.begin();
			it != field.m_tag.end(); it++)
		{
			if (*it < '0' || *it > '9') {
				*it = '?';
			}
		}
	}

	if (fieldTag < "010") {
		// Parse control field.
		field.m_type = MarcRecord::Field::CONTROLFIELD;
		if (m_iconvDesc == (iconv_t) -1) {
			field.m_data.assign(fieldData, fieldLength);
		} else {
			if (!iconv(m_iconvDesc, fieldData, fieldLength,
				field.m_data))
			{
				std::string errorPos;
				snprintf(errorPos, 11, "%d",
					fieldAbsoluteStartPos);

				m_errorCode = ERROR_ICONV;
				m_errorMessage = "encoding conversion failed "
					"at " + errorPos;
				throw m_errorCode;
			}
		}
	} else {
		// Parse data field.
		field.m_type = MarcRecord::Field::DATAFIELD;
		field.m_ind1 = fieldData[0];
		field.m_ind2 = fieldData[1];

		// Replace invalid indicators to character '?'.
		if (m_autoCorrectionMode) {
			if ((field.m_ind1 != ' ') && (field.m_ind1 != '|')
				&& (field.m_ind1 < '0' || field.m_ind1 > '9')
				&& (field.m_ind1 < 'a' || field.m_ind1 > 'z'))
			{
				field.m_ind1 = '?';
			}

			if ((field.m_ind2 != ' ') && (field.m_ind2 != '|')
				&& (field.m_ind2 < '0' || field.m_ind2 > '9')
				&& (field.m_ind2 < 'a' || field.m_ind2 > 'z'))
			{
				field.m_ind2 = '?';
			}
		}

		// Parse list of subfields.
		unsigned int subfieldStartPos = 0;
		unsigned int symbolPos;
		for (symbolPos = 2; symbolPos <= fieldLength; symbolPos++) {
			// Skip symbols of subfield data.
			if (fieldData[symbolPos] != '\x1F'
				&& symbolPos != fieldLength)
			{
				continue;
			}

			if (symbolPos > 2) {
				// Parse regular subfield.
				MarcRecord::Subfield subfield;
				subfield = parseSubfield(fieldData,
					subfieldStartPos, symbolPos);
				field.m_subfieldList.push_back(subfield);
			}

			subfieldStartPos = symbolPos;
		}
	}

	return field;
}

/*
 * Parse subfield.
 */
MarcRecord::Subfield
MarcIsoReader::parseSubfield(const char *fieldData,
	unsigned int subfieldStartPos, unsigned int subfieldEndPos)
{
	// Parse regular subfield.
	MarcRecord::Subfield subfield;
	subfield.clear();

	// Copy subfield identifier.
	subfield.m_id = fieldData[subfieldStartPos + 1];
	// Replace invalid subfield identifier.
	if (m_autoCorrectionMode
		&& (subfield.m_id < '0' || subfield.m_id > '9')
		&& (subfield.m_id < 'a' || subfield.m_id > 'z'))
	{
		subfield.m_id = '?';
	}

	// Check subfield length.
	if (subfieldEndPos - subfieldStartPos < 2) {
		if (m_autoCorrectionMode) {
			subfield.m_data = "?";
			return subfield;
		}
		m_errorCode = ERROR_INVALID_RECORD;
		m_errorMessage = "invalid subfield";
		throw m_errorCode;
	}

	if (m_iconvDesc == (iconv_t) -1) {
		// Copy subfield data.
		subfield.m_data.assign(
			fieldData + subfieldStartPos + 2,
			subfieldEndPos - subfieldStartPos - 2);
	} else {
		// Copy subfield data with encoding conversion.
		if (!iconv(m_iconvDesc,
			fieldData + subfieldStartPos + 2,
			subfieldEndPos - subfieldStartPos - 2,
			subfield.m_data))
		{
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			throw m_errorCode;
		}
	}

	return subfield;
}
