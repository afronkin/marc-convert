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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "marcrecord.h"
#include "marcrecord_tools.h"

using namespace marcrecord;

/*
 * Constructor.
 */
MarcRecord::MarcRecord()
{
	m_formatVariant = UNIMARC;
	clear();
}

MarcRecord::MarcRecord(FormatVariant formatVariant)
{
	setFormatVariant(formatVariant);
	clear();
}

/*
 * Destructor.
 */
MarcRecord::~MarcRecord()
{
}

/*
 * Clear record.
 */
void
MarcRecord::clear(void)
{
	// Clear field list.
	m_fieldList.clear();

	// Reset record leader.
	memset(m_leader.recordLength, ' ', sizeof(m_leader.recordLength));
	m_leader.recordStatus = 'n';
	m_leader.recordType = 'a';
	m_leader.bibliographicLevel = 'm';
	m_leader.hierarchicalLevel = ' ';
	m_leader.undefined1 = ' ';
	m_leader.indicatorLength = '2';
	m_leader.subfieldIdLength = '2';
	memset(m_leader.baseAddress, ' ', sizeof(m_leader.baseAddress));
	m_leader.encodingLevel = ' ';
	m_leader.cataloguingForm = ' ';
	m_leader.undefined2 = ' ';
	m_leader.lengthOfFieldLength = '4';
	m_leader.startingPositionLength = '5';
	m_leader.implementationDefinedLength = '0';
	m_leader.undefined3 = ' ';
}

/*
 * Get record format variant.
 */
MarcRecord::FormatVariant
MarcRecord::getFormatVariant(void)
{
	return m_formatVariant;
}

/*
 * Set record format variant.
 */
void
MarcRecord::setFormatVariant(FormatVariant formatVariant)
{
	m_formatVariant = formatVariant;
}

/*
 * Get record leader.
 */
MarcRecord::Leader &
MarcRecord::getLeader(void)
{
	return m_leader;
}

/*
 * Set record leader.
 */
void
MarcRecord::setLeader(const Leader &leader)
{
	m_leader = leader;
}

void
MarcRecord::setLeader(const std::string &leaderData)
{
	memcpy((void *) &m_leader, leaderData.c_str(),
		std::min(sizeof(Leader), leaderData.size()));
}

/*
 * Get list of fields.
 */
MarcRecord::FieldRefList
MarcRecord::getFields(const std::string &fieldTag)
{
	FieldRefList resultFieldList;
	FieldIt fieldIt;

	// Check fields in list.
	for (fieldIt = m_fieldList.begin(); fieldIt != m_fieldList.end();
		fieldIt++)
	{
		if (fieldTag == "" || fieldTag == fieldIt->m_tag) {
			resultFieldList.push_back(fieldIt);
		}
	}

	return resultFieldList;
}

/*
 * Get field.
 */
MarcRecord::FieldIt
MarcRecord::getField(const std::string &fieldTag)
{
	FieldIt fieldIt;

	// Check fields in list.
	for (fieldIt = m_fieldList.begin(); fieldIt != m_fieldList.end();
		fieldIt++)
	{
		if (fieldTag == "" || fieldTag == fieldIt->m_tag) {
			return fieldIt;
		}
	}

	return m_fieldList.end();
}

/*
 * Add field to the end of record.
 */
MarcRecord::FieldIt
MarcRecord::addField(const Field &field)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(m_fieldList.end(), field);
	return fieldIt;
}

MarcRecord::FieldIt
MarcRecord::addControlField(const std::string &fieldTag,
	const std::string &fieldData)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(m_fieldList.end(),
		Field(fieldTag, fieldData));
	return fieldIt;
}

MarcRecord::FieldIt
MarcRecord::addDataField(const std::string &fieldTag,
	char fieldInd1, char fieldInd2)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(m_fieldList.end(),
		Field(fieldTag, fieldInd1, fieldInd2));
	return fieldIt;
}

/*
 * Add field to the record before specified field.
 */
MarcRecord::FieldIt
MarcRecord::addFieldBefore(FieldIt nextFieldIt, const Field &field)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(nextFieldIt, field);
	return fieldIt;
}

MarcRecord::FieldIt
MarcRecord::addControlFieldBefore(FieldIt nextFieldIt,
	const std::string &fieldTag, const std::string &fieldData)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(nextFieldIt,
		Field(fieldTag, fieldData));
	return fieldIt;
}

MarcRecord::FieldIt
MarcRecord::addDataFieldBefore(FieldIt nextFieldIt,
	const std::string &fieldTag, char fieldInd1, char fieldInd2)
{
	// Append field to the list.
	FieldIt fieldIt = m_fieldList.insert(nextFieldIt,
		Field(fieldTag, fieldInd1, fieldInd2));
	return fieldIt;
}

/*
 * Remove field from the record.
 */
void
MarcRecord::removeField(FieldIt fieldIt)
{
	// Remove field from the list.
	m_fieldList.erase(fieldIt);
}

/*
 * Format record to string for printing.
 */
std::string
MarcRecord::toString(void)
{
	// Print leader.
	std::string textRecord = "Leader [";
	textRecord.append((const char *) &m_leader, sizeof(Leader));
	textRecord += "]";

	// Iterate all fields.
	for (MarcRecord::FieldIt fieldIt = m_fieldList.begin();
		fieldIt != m_fieldList.end(); fieldIt++)
	{
		// Print field.
		textRecord += "\n" + fieldIt->toString();
	}

	return textRecord;
}
