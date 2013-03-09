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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marc_reader.h"

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
MarcReader::MarcReader(FILE *inputFile, const char *inputEncoding)
{
	// Clear member variables.
	m_iconvDesc = (iconv_t) -1;
	m_autoCorrectionMode = false;

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
MarcReader::~MarcReader()
{
	// Close input file.
	close();
}

/*
 * Get last error code.
 */
MarcReader::ErrorCode MarcReader::getErrorCode(void)
{
	return m_errorCode;
}

/*
 * Get last error message.
 */
std::string &
MarcReader::getErrorMessage(void)
{
	return m_errorMessage;
}

/*
 * Set automatic error correction mode.
 */
void
MarcReader::setAutoCorrectionMode(bool autoCorrectionMode)
{
	m_autoCorrectionMode = autoCorrectionMode;
}

/*
 * Open input file.
 */
bool
MarcReader::open(FILE *inputFile, const char *inputEncoding)
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
MarcReader::close(void)
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
MarcReader::next(MarcRecord &record)
{
	int symbol;
	char recordBuf[100000];
	unsigned int recordLen;

	// Clear error code and message.
	m_errorCode = OK;
	m_errorMessage = "";

	// Skip possible wrong symbols.
	do {
		symbol = fgetc(m_inputFile);
	} while (symbol >= 0 && isdigit(symbol) == 0);

	if (symbol < 0) {
		m_errorCode = END_OF_FILE;
		return false;
	}

	// Read record length.
	recordBuf[0] = (char) symbol;
	if (fread(recordBuf + 1, 1, 4, m_inputFile) != 4) {
		m_errorCode = END_OF_FILE;
		return false;
	}

	// Parse record length.
	if (!is_numeric(recordBuf, 5)
		|| sscanf(recordBuf, "%5u", &recordLen) != 1)
	{
		m_errorCode = ERROR_INVALID_RECORD;
		m_errorMessage = "invalid record length";
		return false;
	}

	// Read record.
	if (fread(recordBuf + 5, 1, recordLen - 5, m_inputFile)
		!= recordLen - 5)
	{
		m_errorCode = ERROR_INVALID_RECORD;
		m_errorMessage =
			"invalid record length or record data incomplete";
		return false;
	}

	// Parse record.
	return parse(recordBuf, recordLen, record);
}

/*
 * Parse record from ISO 2709 buffer.
 */
bool
MarcReader::parse(const char *recordBuf, unsigned int recordBufLen,
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
		if (!is_numeric(record.m_leader.baseAddress, 5)
			|| sscanf(record.m_leader.baseAddress, "%05u",
				&baseAddress) != 1
			|| recordLen < baseAddress)
		{
			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid base address of data";
			throw m_errorCode;
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
		int fieldNo = 0;
		for (; fieldNo < numFields; fieldNo++, directoryEntry++) {
			// Check directory entry.
			if (!is_numeric((const char *) directoryEntry,
				sizeof(RecordDirectoryEntry)))
			{
				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "invalid directory entry";
				throw m_errorCode;
			}

			// Parse directory entry.
			std::string fieldTag(directoryEntry->fieldTag, 0, 3);
			unsigned int fieldLength, fieldStartPos;
			if (sscanf(directoryEntry->fieldLength, "%4u%5u",
				&fieldLength, &fieldStartPos) != 2)
			{
				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage =
					"invalid base address of data";
				throw m_errorCode;
			}

			// Check field starting position and length.
			if (baseAddress + fieldStartPos + fieldLength
				> recordLen)
			{
				m_errorCode = ERROR_INVALID_RECORD;
				m_errorMessage = "invalid field starting "
					"position or length";
				throw m_errorCode;
			}

			// Parse field.
			MarcRecord::Field field = parseField(fieldTag,
				recordData + fieldStartPos, fieldLength);
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
MarcReader::parseField(const std::string &fieldTag,
	const char *fieldData, unsigned int fieldLength)
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

	if (fieldTag < "010" || fieldLength < 2) {
		// Parse control field.
		field.m_type = MarcRecord::Field::CONTROLFIELD;
		if (m_iconvDesc == (iconv_t) -1) {
			field.m_data.assign(fieldData, fieldLength);
		} else {
			if (!iconv(m_iconvDesc, fieldData, fieldLength,
				field.m_data))
			{
				m_errorCode = ERROR_ICONV;
				m_errorMessage = "encoding conversion failed";
				throw m_errorCode;
			}
		}
	} else {
		// Check field length.
		if (fieldLength < 2) {
			m_errorCode = ERROR_INVALID_RECORD;
			m_errorMessage = "invalid length of data field";
			throw m_errorCode;
		}

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
MarcReader::parseSubfield(const char *fieldData, unsigned int subfieldStartPos,
	unsigned int subfieldEndPos)
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