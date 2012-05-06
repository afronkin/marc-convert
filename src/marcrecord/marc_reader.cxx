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
 * Open input file.
 */
void MarcReader::open(FILE *inputFile, const char *inputEncoding)
{
	/* Initialize input stream parameters. */
	this->inputFile = inputFile == NULL ? stdin : inputFile;
	this->inputEncoding = inputEncoding == NULL ? "" : inputEncoding;
}

/*
 * Close input file.
 */
void MarcReader::close(void)
{
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

	/* Skip possible wrong symbols. */
	do {
		symbol = fgetc(inputFile);
	} while (symbol >= 0 && isdigit(symbol) == 0);

	if (symbol < 0) {
		return false;
	}

	/* Read record length. */
	recordBuf[0] = (char) symbol;
	if (fread(recordBuf + 1, 1, 4, inputFile) != 4) {
		return false;
	}

	/* Parse record length. */
	if (sscanf(recordBuf, "%5u", &recordLen) != 1) {
		return false;
	}

	/* Read record. */
	if (fread(recordBuf + 5, 1, recordLen - 5, inputFile) != recordLen - 5) {
		return false;
	}

	/* Parse record. */
	return parseRecord(recordBuf, record);
}

/*
 * Parse record from ISO 2709 buffer.
 */
bool MarcReader::parseRecord(const char *recordBuf, MarcRecord &record)
{
	unsigned int baseAddress, numFields;
	RecordDirectoryEntry *directoryEntry;
	const char *recordData;
	unsigned int fieldNo;
	std::string fieldTag;
	unsigned int fieldLength, fieldStartPos;
	MarcRecord::Field field;

	try {
		/* Clear current record data. */
		record.clear();

		/* Copy record leader. */
		memcpy(&record.leader, recordBuf, sizeof(struct MarcRecord::Leader));

		/* Get base address of data. */
		if (sscanf(record.leader.baseAddress, "%05u", &baseAddress) != 1) {
			throw MarcRecord::ERROR;
		}

		/* Get number of fields. */
		numFields = (baseAddress - sizeof(struct MarcRecord::Leader) - 1) /
			sizeof(struct RecordDirectoryEntry);

		/* Parse list of fields. */
		directoryEntry =
			(RecordDirectoryEntry *) (recordBuf + sizeof(struct MarcRecord::Leader));
		recordData = recordBuf + baseAddress;
		for (fieldNo = 0; fieldNo < numFields; fieldNo++, directoryEntry++) {
			/* Parse directory entry. */
			fieldTag.assign(directoryEntry->fieldTag, 0, 3);
			if (sscanf(directoryEntry->fieldLength, "%4u%5u",
				&fieldLength, &fieldStartPos) != 2)
			{
				throw MarcRecord::ERROR;
			}

			/* Parse field. */
			field = parseField(fieldTag, recordData + fieldStartPos, fieldLength);
			/* Append field to list. */
			record.fieldList.push_back(field);
		}
	} catch (int errorCode) {
		if (errorCode != 0) {
			record.clear();
			return false;
		}
	}

	return true;
}

/*
 * Parse field from ISO 2709 buffer.
 */
MarcRecord::Field MarcReader::parseField(const std::string &fieldTag,
	const char *fieldData, unsigned int fieldLength)
{
	MarcRecord::Field field;
	MarcRecord::Subfield subfield;
	unsigned int symbolPos, subfieldStartPos;

	/* Clear field. */
	/* field.clear(); */

	/* Adjust field length. */
	if (fieldData[fieldLength - 1] == '\x1E') {
		fieldLength--;
	}

	field.tag = fieldTag;
	if (fieldTag < "010" || fieldLength < 2) {
		/* Parse control field. */
		field.type = MarcRecord::Field::CONTROLFIELD;
		field.data.assign(fieldData, fieldLength);
	} else {
		/* Parse data field. */
		field.type = MarcRecord::Field::DATAFIELD;
		field.ind1 = fieldData[0];
		field.ind2 = fieldData[1];

		/* Parse list of subfields. */
		subfieldStartPos = 0;
		for (symbolPos = 2; symbolPos <= fieldLength; symbolPos++) {
			/* Skip symbols of subfield data. */
			if (fieldData[symbolPos] != '\x1F' && symbolPos != fieldLength) {
				continue;
			}

			if (symbolPos > 2) {
				/* Parse regular subfield. */
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
