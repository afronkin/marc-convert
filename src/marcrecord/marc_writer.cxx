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
#include "marc_writer.h"

namespace marcrecord {

#define ISO2709_RECORD_SEPARATOR	'\x1D'
#define ISO2709_FIELD_SEPARATOR		'\x1E'
#define ISO2709_IDENTIFIER_DELIMITER	'\x1F'

#pragma pack(1)

/*
 * Structure of record directory entry.
 */
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
MarcWriter::MarcWriter(FILE *outputFile, const char *outputEncoding)
{
	// Clear member variables.
	m_iconvDesc = (iconv_t) -1;

	if (outputFile) {
		// Open output file.
		open(outputFile, outputEncoding);
	} else {
		// Clear object state.
		close();
	}
}

/*
 * Destructor.
 */
MarcWriter::~MarcWriter()
{
	// Close output file.
	close();
}

/*
 * Get last error code.
 */
MarcWriter::ErrorCode
MarcWriter::getErrorCode(void)
{
	return m_errorCode;
}

/*
 * Get last error message.
 */
std::string &
MarcWriter::getErrorMessage(void)
{
	return m_errorMessage;
}

/*
 * Open output file.
 */
bool
MarcWriter::open(FILE *outputFile, const char *outputEncoding)
{
	// Clear error code and message.
	m_errorCode = OK;
	m_errorMessage = "";

	// Initialize output stream parameters.
	m_outputFile = outputFile == NULL ? stdout : outputFile;
	m_outputEncoding = outputEncoding == NULL ? "" : outputEncoding;

	// Initialize encoding conversion.
	if (outputEncoding == NULL
		|| strcmp(outputEncoding, "UTF-8") == 0
		|| strcmp(outputEncoding, "utf-8") == 0)
	{
		m_iconvDesc = (iconv_t) -1;
	} else {
		// Create iconv descriptor for output encoding conversion.
		m_iconvDesc = iconv_open(outputEncoding, "UTF-8");
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
 * Close output file.
 */
void
MarcWriter::close(void)
{
	// Finalize iconv.
	if (m_iconvDesc != (iconv_t) -1) {
		iconv_close(m_iconvDesc);
	}

	// Clear member variables.
	m_errorCode = OK;
	m_errorMessage = "";
	m_outputFile = NULL;
	m_outputEncoding = "";
	m_iconvDesc = (iconv_t) -1;
}

/*
 * Write record to ISO 2709 file.
 */
bool
MarcWriter::write(MarcRecord &record)
{
	char recordBuf[100000];

	// Copy record leader to buffer.
	memcpy(recordBuf, (char *) &record.m_leader,
		sizeof(MarcRecord::Leader));

	// Calculate base address of data and copy it to record buffer.
	unsigned int baseAddress = sizeof(MarcRecord::Leader)
		+ record.m_fieldList.size()
		* sizeof(RecordDirectoryEntry) + 1;
	char baseAddressBuf[6];
	sprintf(baseAddressBuf, "%05d", baseAddress);
	memcpy(recordBuf + 12, baseAddressBuf, 5);

	// Iterate all fields.
	char *directoryData = recordBuf + sizeof(MarcRecord::Leader);
	char *fieldData = recordBuf + baseAddress;
	for (MarcRecord::FieldIt fieldIt = record.m_fieldList.begin();
		fieldIt != record.m_fieldList.end(); fieldIt++)
	{
		int fieldLength = 0;
		if (fieldIt->m_tag < "010") {
			fieldLength = appendControlField(fieldData, fieldIt);
			fieldData += fieldLength;
		} else {
			// Copy indicators of data field to buffer.
			*(fieldData++) = fieldIt->m_ind1;
			*(fieldData++) = fieldIt->m_ind2;
			fieldLength += 2;

			// Iterate all subfields.
			MarcRecord::SubfieldIt subfieldIt =
				fieldIt->m_subfieldList.begin();
			for (; subfieldIt != fieldIt->m_subfieldList.end();
				subfieldIt++)
			{
				int subfieldLength =
					appendSubfield(fieldData, subfieldIt);
				fieldData += subfieldLength;
				fieldLength += subfieldLength;
			}
		}

		// Set field separator at the end of field.
		*(fieldData++) = ISO2709_FIELD_SEPARATOR;
		fieldLength++;

		/*
		 * Fill directory entry (it is safe to do this way because
		 * null character will be overwritten in next iteration and
		 * right after the cycle).
		 */
		int fieldOffset = (int) (fieldData - recordBuf) - baseAddress
			- fieldLength;
		sprintf(directoryData, "%.3s%04d%05d",
			fieldIt->m_tag.c_str(), fieldLength, fieldOffset);
		directoryData += sizeof(RecordDirectoryEntry);
	}

	// Set field separator at the end of directory.
	directoryData[0] = ISO2709_FIELD_SEPARATOR;
	// Set record separator at the end of record.
	*(fieldData++) = ISO2709_RECORD_SEPARATOR;

	// Calculate directory length and copy it to record buffer.
	int recordLength = (int) (fieldData - recordBuf);
	char recordLengthBuf[6];
	sprintf(recordLengthBuf, "%05d", recordLength);
	memcpy(recordBuf, recordLengthBuf, 5);

	// Write record buffer to file.
	if (fwrite(recordBuf, recordLength, 1, m_outputFile) != 1) {
		m_errorCode = ERROR_IO;
		m_errorMessage = "i/o operation failed";
		return false;
	}

	return true;
}

/*
 * Append control field data to the write buffer.
 */
int
MarcWriter::appendControlField(char *fieldData, MarcRecord::FieldIt &fieldIt)
{
	int fieldLength;

	if (m_iconvDesc == (iconv_t) -1) {
		// Copy control field to buffer.
		fieldLength = fieldIt->m_data.size();
		if (fieldLength > 10000) {
			m_errorCode = ERROR_DATASIZE;
			m_errorMessage = "field size exceed ISO2709 limit";
			return false;
		}
		memcpy(fieldData, fieldIt->m_data.c_str(), fieldLength);
	} else {
		// Copy control field to buffer with encoding conversion.
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, fieldIt->m_data, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		fieldLength = iconvBuf.size();
		if (fieldLength > 10000) {
			m_errorCode = ERROR_DATASIZE;
			m_errorMessage = "field size exceed ISO2709 limit";
			return false;
		}
		memcpy(fieldData, iconvBuf.c_str(), fieldLength);
	}

	return fieldLength;
}

/*
 * Append subfield data to the write buffer.
 */
int
MarcWriter::appendSubfield(char *fieldData, MarcRecord::SubfieldIt &subfieldIt)
{
	int subfieldLength;

	*(fieldData) = ISO2709_IDENTIFIER_DELIMITER;
	*(fieldData + 1) = subfieldIt->m_id;
	if (m_iconvDesc == (iconv_t) -1) {
		// Copy subfield to buffer.
		subfieldLength = subfieldIt->m_data.size();
		if (subfieldLength > 10000) {
			m_errorCode = ERROR_DATASIZE;
			m_errorMessage = "field size exceed ISO2709 limit";
			return false;
		}
		memcpy(fieldData + 2, subfieldIt->m_data.c_str(),
			subfieldLength);
	} else {
		// Copy subfield to buffer with encoding conversion.
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, subfieldIt->m_data, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		subfieldLength = iconvBuf.size();
		if (subfieldLength > 10000) {
			m_errorCode = ERROR_DATASIZE;
			m_errorMessage = "field size exceed ISO2709 limit";
			return false;
		}
		memcpy(fieldData + 2, iconvBuf.c_str(), subfieldLength);
	}

	return subfieldLength + 2;
}