/*
 * Copyright (c) 2012, Alexander Fronkin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/* Version: 2.0 (27 Feb 2011) */

#include <stdio.h>
#include <string.h>
#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marc_reader.h"

#define ISO2709_RECORD_SEPARATOR	'\x1D'
#define ISO2709_FIELD_SEPARATOR		'\x1E'
#define ISO2709_IDENTIFIER_DELIMITER	'\x1F'

#pragma pack(push)
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

#pragma pack(pop)

/*
 * Constructor.
 */
MarcReader::MarcReader(FILE *inputFile, const char *inputEncoding)
{
	if (inputFile) {
		/* Open input file. */
		open(inputFile, inputEncoding);
	} else {
		/* Clear object state. */
		close();
	}
}

/*
 * Destructor.
 */
MarcReader::~MarcReader()
{
	/* Close input file. */
	close();
}

/*
 * Get last error code.
 */
MarcReader::ErrorCode MarcReader::getErrorCode(void)
{
	return errorCode;
}

/*
 * Get last error message.
 */
std::string & MarcReader::getErrorMessage(void)
{
	return errorMessage;
}

/*
 * Open input file.
 */
void MarcReader::open(FILE *inputFile, const char *inputEncoding)
{
	/* Clear error code and message. */
	errorCode = OK;
	errorMessage = "";

	/* Initialize input stream parameters. */
	this->inputFile = inputFile == NULL ? stdin : inputFile;
	this->inputEncoding = inputEncoding == NULL ? "" : inputEncoding;
}

/*
 * Close input file.
 */
void MarcReader::close(void)
{
	/* Clear error code and message. */
	errorCode = OK;
	errorMessage = "";

	/* Clear input stream parameters. */
	this->inputFile = NULL;
	this->inputEncoding = "";
}

/*
 * Read next record from MARCXML file.
 */
bool MarcReader::next(MarcRecord &record)
{
	int symbol;
	char recordBuf[100000];
	unsigned int recordLen;

	/* Clear error code and message. */
	errorCode = OK;
	errorMessage = "";

	/* Skip possible wrong symbols. */
	do {
		symbol = fgetc(inputFile);
	} while (symbol >= 0 && isdigit(symbol) == 0);

	if (symbol < 0) {
		errorCode = END_OF_FILE;
		return false;
	}

	/* Read record length. */
	recordBuf[0] = (char) symbol;
	if (fread(recordBuf + 1, 1, 4, inputFile) != 4) {
		errorCode = END_OF_FILE;
		return false;
	}

	/* Parse record length. */
	if (!is_numeric(recordBuf, 5) || sscanf(recordBuf, "%5u", &recordLen) != 1) {
		errorCode = ERROR_INVALID_RECORD;
		errorMessage = "invalid record length";
		return false;
	}

	/* Read record. */
	if (fread(recordBuf + 5, 1, recordLen - 5, inputFile) != recordLen - 5) {
		errorCode = ERROR_INVALID_RECORD;
		errorMessage = "invalid record length or record data incomplete";
		return false;
	}

	/* Parse record. */
	return parse(recordBuf, recordLen, record);
}

/*
 * Parse record from ISO 2709 buffer.
 */
bool MarcReader::parse(const char *recordBuf, unsigned int recordBufLen, MarcRecord &record)
{
	/* Clear error code and message. */
	errorCode = OK;
	errorMessage = "";

	/* Clear current record data. */
	record.clear();

	try {
		/* Check record length. */
		unsigned int recordLen;
		if (!is_numeric(recordBuf, 5)
			|| sscanf(recordBuf, "%5u", &recordLen) != 1
			|| recordLen != recordBufLen
			|| recordLen < sizeof(struct MarcRecord::Leader))
		{
			errorCode = ERROR_INVALID_RECORD;
			errorMessage = "invalid record length";
			throw;
		}

		/* Copy record leader. */
		memcpy(&record.leader, recordBuf, sizeof(struct MarcRecord::Leader));

		/* Get base address of data. */
		unsigned int baseAddress;
		if (!is_numeric(record.leader.baseAddress, 5)
			|| sscanf(record.leader.baseAddress, "%05u", &baseAddress) != 1
			|| recordLen < baseAddress)
		{
			errorCode = ERROR_INVALID_RECORD;
			errorMessage = "invalid base address of data";
			throw;
		}

		/* Get number of fields. */
		int numFields = (baseAddress - sizeof(struct MarcRecord::Leader) - 1) /
			sizeof(struct RecordDirectoryEntry);
		if (recordLen < sizeof(struct MarcRecord::Leader)
			+ (sizeof(struct RecordDirectoryEntry) * numFields))
		{
			errorCode = ERROR_INVALID_RECORD;
			errorMessage = "invalid record length";
			throw;
		}

		/* Parse list of fields. */
		struct RecordDirectoryEntry *directoryEntry = 
			(RecordDirectoryEntry *) (recordBuf + sizeof(struct MarcRecord::Leader));
		const char *recordData = recordBuf + baseAddress;
		for (int fieldNo = 0; fieldNo < numFields; fieldNo++, directoryEntry++) {
			/* Check directory entry. */
			if (!is_numeric((const char *) directoryEntry,
				sizeof(struct RecordDirectoryEntry)))
			{
				errorCode = ERROR_INVALID_RECORD;
				errorMessage = "invalid directory entry";
				throw;
			}

			/* Parse directory entry. */
			std::string fieldTag(directoryEntry->fieldTag, 0, 3);
			// fieldTag.assign(directoryEntry->fieldTag, 0, 3);
			unsigned int fieldLength, fieldStartPos;
			if (sscanf(directoryEntry->fieldLength, "%4u%5u",
				&fieldLength, &fieldStartPos) != 2)
			{
				errorCode = ERROR_INVALID_RECORD;
				errorMessage = "invalid base address of data";
				throw;
			}

			/* Check field starting position and length. */
			if (baseAddress + fieldStartPos + fieldLength > recordLen) {
				errorCode = ERROR_INVALID_RECORD;
				errorMessage = "invalid field starting position or length";
				throw;
			}

			/* Parse field. */
			MarcRecord::Field field =
				parseField(fieldTag, recordData + fieldStartPos, fieldLength);
			/* Append field to list. */
			record.fieldList.push_back(field);
		}
	} catch (...) {
		record.clear();
		return false;
	}

	return true;
}

/*
 * Parse field from ISO 2709 buffer.
 */
MarcRecord::Field MarcReader::parseField(const std::string &fieldTag,
	const char *fieldData, unsigned int fieldLength)
{
	/* Adjust field length. */
	if (fieldData[fieldLength - 1] == '\x1E') {
		fieldLength--;
	}

	MarcRecord::Field field;
	field.tag = fieldTag;
	if (fieldTag < "010" || fieldLength < 2) {
		/* Parse control field. */
		field.type = MarcRecord::Field::CONTROLFIELD;
		field.data.assign(fieldData, fieldLength);
	} else {
		/* Check field length. */
		if (fieldLength < 2) {
			errorCode = ERROR_INVALID_RECORD;
			errorMessage = "invalid length of data field";
			throw;
		}

		/* Parse data field. */
		field.type = MarcRecord::Field::DATAFIELD;
		field.ind1 = fieldData[0];
		field.ind2 = fieldData[1];

		/* Parse list of subfields. */
		unsigned int subfieldStartPos = 0;
		for (unsigned int symbolPos = 2; symbolPos <= fieldLength; symbolPos++) {
			/* Skip symbols of subfield data. */
			if (fieldData[symbolPos] != '\x1F' && symbolPos != fieldLength) {
				continue;
			}

			if (symbolPos > 2) {
				/* Parse regular subfield. */
				MarcRecord::Subfield subfield;
				subfield.clear();
				subfield.id = fieldData[subfieldStartPos + 1];
				subfield.data.assign(
					fieldData + subfieldStartPos + 2,
					symbolPos - subfieldStartPos - 2);

				/* Append subfield to list. */
				field.subfieldList.push_back(subfield);
			}

			subfieldStartPos = symbolPos;
		}
	}

	return field;
}
