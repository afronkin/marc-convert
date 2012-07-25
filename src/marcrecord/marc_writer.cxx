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
#include "marc_writer.h"

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

#pragma pack()

/*
 * Constructor.
 */
MarcWriter::MarcWriter(FILE *outputFile, const char *outputEncoding)
{
	if (outputFile) {
		/* Open output file. */
		open(outputFile, outputEncoding);
	} else {
		/* Clear object state. */
		close();
	}
}

/*
 * Destructor.
 */
MarcWriter::~MarcWriter()
{
	/* Close output file. */
	close();
}

/*
 * Open output file.
 */
void MarcWriter::open(FILE *outputFile, const char *outputEncoding)
{
	/* Initialize output stream parameters. */
	m_outputFile = outputFile == NULL ? stdout : outputFile;
	m_outputEncoding = outputEncoding == NULL ? "" : outputEncoding;
}

/*
 * Close output file.
 */
void MarcWriter::close(void)
{
	/* Clear output stream parameters. */
	m_outputFile = NULL;
	m_outputEncoding = "";
}

/*
 * Write record to ISO 2709 file.
 */
bool MarcWriter::write(MarcRecord &record)
{
	char recordBuf[100000];

	/* Copy record leader to buffer. */
	memcpy(recordBuf, (char *) &record.m_leader, sizeof(struct MarcRecord::Leader));

	/* Calculate base address of data and copy it to record buffer. */
	unsigned int baseAddress = sizeof(struct MarcRecord::Leader) + record.m_fieldList.size()
		* sizeof(struct RecordDirectoryEntry) + 1;
	char baseAddressBuf[6];
	sprintf(baseAddressBuf, "%05d", baseAddress);
	memcpy(recordBuf + 12, baseAddressBuf, 5);

	/* Iterate all fields. */
	char *directoryData = recordBuf + sizeof(struct MarcRecord::Leader);
	char *fieldData = recordBuf + baseAddress;
	for (MarcRecord::FieldIt fieldIt = record.m_fieldList.begin();
		fieldIt != record.m_fieldList.end(); fieldIt++)
	{
		int fieldLength = 0;
		if (fieldIt->m_tag < "010") {
			/* Copy control field to buffer. */
			fieldLength = fieldIt->m_data.size();
			memcpy(fieldData, fieldIt->m_data.c_str(), fieldLength);
			fieldData += fieldLength;
		} else {
			/* Copy indicators of data field to buffer. */
			*(fieldData++) = fieldIt->m_ind1;
			*(fieldData++) = fieldIt->m_ind2;
			fieldLength += 2;

			/* Iterate all subfields. */
			for (MarcRecord::SubfieldIt subfieldIt = fieldIt->m_subfieldList.begin();
				subfieldIt != fieldIt->m_subfieldList.end(); subfieldIt++)
			{
				int subfieldLength = subfieldIt->m_data.size();

				/* Copy subfield to buffer. */
				*(fieldData++) = ISO2709_IDENTIFIER_DELIMITER;
				*(fieldData++) = subfieldIt->m_id;
				memcpy(fieldData, subfieldIt->m_data.c_str(), subfieldLength);
				fieldData += subfieldLength;
				fieldLength += subfieldLength + 2;
			}
		}

		/* Set field separator at the end of field. */
		*(fieldData++) = ISO2709_FIELD_SEPARATOR;
		fieldLength++;

		/* Fill directory entry (it is safe to do this way because null character
		   will be overwritten in next iteration and right after the cycle). */
		sprintf(directoryData, "%.3s%04d%05d", fieldIt->m_tag.c_str(), fieldLength,
			(int) (fieldData - recordBuf) - baseAddress - fieldLength);
		directoryData += sizeof(struct RecordDirectoryEntry);
	}

	/* Set field separator at the end of directory. */
	directoryData[0] = ISO2709_FIELD_SEPARATOR;
	/* Set record separator at the end of record. */
	*(fieldData++) = ISO2709_RECORD_SEPARATOR;

	/* Calculate directory length and copy it to record buffer (can't use easy way here). */
	int recordLength = (int) (fieldData - recordBuf);
	char recordLengthBuf[6];
	sprintf(recordLengthBuf, "%05d", recordLength);
	memcpy(recordBuf, recordLengthBuf, 5);

	/* Write record buffer to file. */
	if (fwrite(recordBuf, recordLength, 1, m_outputFile) != 1) {
		return false;
	}

	return true;
}
