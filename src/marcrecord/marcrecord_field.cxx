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

#include "marcrecord.h"
#include "marcrecord_tools.h"

/*
 * Constructor.
 */
MarcRecord::Field::Field(const std::string &tag, const std::string &data)
{
	clear();
	type = CONTROLFIELD;
	this->tag = tag;
	this->data = data;
}

MarcRecord::Field::Field(const std::string &tag, char ind1, char ind2)
{
	clear();
	type = DATAFIELD;
	this->tag = tag;
	this->ind1 = ind1;
	this->ind2 = ind2;
}

/*
 * Clear field data.
 */
void MarcRecord::Field::clear(void)
{
	type = CONTROLFIELD;
	tag = "";
	data.erase();
	ind1 = ' ';
	ind2 = ' ';
	subfieldList.clear();
}

/*
 * Set type of field to control field.
 */
void MarcRecord::Field::setControlFieldType(void)
{
	if (type != CONTROLFIELD) {
		type = CONTROLFIELD;
		clear();
	}
}

/*
 * Set type of field to data field.
 */
void MarcRecord::Field::setDataFieldType(void)
{
	if (type != DATAFIELD) {
		type = DATAFIELD;
		clear();
	}
}

/*
 * Return true if file type is control field.
 */
bool MarcRecord::Field::isControlField(void)
{
	return (type == CONTROLFIELD);
}

/*
 * Return true if file type is data field.
 */
bool MarcRecord::Field::isDataField(void)
{
	return (type == DATAFIELD);
}

/*
 * Get tag of data field.
 */
std::string & MarcRecord::Field::getTag(void)
{
	return tag;
}

/*
 * Set tag of data field.
 */
void MarcRecord::Field::setTag(const std::string &tag)
{
	this->tag = tag;
}

/*
 * Get indicator 1 of data field.
 */
char MarcRecord::Field::getInd1(void)
{
	return ind1;
}

/*
 * Get indicator 2 of data field.
 */
char MarcRecord::Field::getInd2(void)
{
	return ind2;
}

/*
 * Set indicator 1 of data field.
 */
void MarcRecord::Field::setInd1(const char ind1)
{
	this->ind1 = ind1;
}

/*
 * Set indicator 2 of data field.
 */
void MarcRecord::Field::setInd2(const char ind1)
{
	this->ind2 = ind2;
}

/*
 * Get data of control field.
 */
std::string & MarcRecord::Field::getData(void)
{
	return data;
}

/*
 * Set data of control field.
 */
void MarcRecord::Field::setData(const std::string &data)
{
	this->data = data;
}

/*
 * Format field to string.
 */
std::string MarcRecord::Field::toString(void)
{
	/* Format control field to string. */
	if (type == CONTROLFIELD) {
		return (tag + " " + data);
	}

	/* Format data field to string. */
	std::string textField = tag;
	snprintf(textField, 5, " [%c%c]", ind1, ind2);

	/* Iterate all subfields. */
	for (MarcRecord::SubfieldIt subfieldIt = subfieldList.begin();
		subfieldIt != subfieldList.end(); subfieldIt++)
	{
		// if (formatVariant == UNIMARC && subfieldIt->id == '1') {
		if (subfieldIt->id == '1') {
			/* Print header of embedded field. */
			snprintf(textField, 4, " $%c ", subfieldIt->id);
			if (subfieldIt->getEmbeddedTag() < "010") {
				textField += "<" + subfieldIt->getEmbeddedTag() + "> "
					+ subfieldIt->getEmbeddedData();
			} else {
				textField += "<" + subfieldIt->getEmbeddedTag() + "> ["
					+ subfieldIt->getEmbeddedInd1()
					+ subfieldIt->getEmbeddedInd2() + "]";
			}
		} else {
			/* Print regular subfield. */
			snprintf(textField, 4, " $%c ", subfieldIt->id);
			textField += subfieldIt->data;
		}
	}

	return textField;
}

/*
 * Get list of subfields.
 */
MarcRecord::SubfieldRefList MarcRecord::Field::getSubfields(char subfieldId)
{
	SubfieldRefList resultSubfieldList;
	SubfieldIt subfieldIt;

	/* Check subfields in list. */
	for (subfieldIt = subfieldList.begin(); subfieldIt != subfieldList.end(); subfieldIt++) {
		if (subfieldId == ' ' || subfieldIt->id == subfieldId) {
			resultSubfieldList.push_back(subfieldIt);
		}
	}

	return resultSubfieldList;
}

/*
 * Get subfield.
 */
MarcRecord::SubfieldIt MarcRecord::Field::getSubfield(char subfieldId)
{
	SubfieldIt subfieldIt;

	/* Check subfields in list. */
	for (subfieldIt = subfieldList.begin(); subfieldIt != subfieldList.end(); subfieldIt++) {
		if (subfieldId == ' ' || subfieldIt->id == subfieldId) {
			return subfieldIt;
		}
	}

	return subfieldList.end();
}

/*
 * Get list of embedded fields.
 */
MarcRecord::EmbeddedFieldList MarcRecord::Field::getEmbeddedFields(const std::string &fieldTag)
{
	EmbeddedFieldList resultFieldList;
	SubfieldRefList embeddedSubfieldList;

	/* Check subfields in list. */
	embeddedSubfieldList.clear();
	for (SubfieldIt subfieldIt = subfieldList.begin();
		subfieldIt != subfieldList.end(); subfieldIt++)
	{
		if (subfieldIt->id == '1') {
			if (embeddedSubfieldList.empty() == false) {
				/* Append embedded field to the result list. */
				resultFieldList.push_back(embeddedSubfieldList);
				embeddedSubfieldList.clear();
			}

			if (fieldTag == "" || fieldTag == subfieldIt->getEmbeddedTag()) {
				/* Append first subfield of embedded field. */
				embeddedSubfieldList.push_back(subfieldIt);
			}
		} else if (embeddedSubfieldList.empty() == false) {
			/* Append subfield to the embedded field. */
			embeddedSubfieldList.push_back(subfieldIt);
		}
	}

	if (embeddedSubfieldList.empty() == false) {
		/* Append embedded field to the result list. */
		resultFieldList.push_back(embeddedSubfieldList);
	}

	return resultFieldList;
}

/*
 * Get embedded field.
 */
MarcRecord::SubfieldRefList MarcRecord::Field::getEmbeddedField(const std::string &fieldTag)
{
	SubfieldRefList embeddedSubfieldList;

	/* Check subfields in list. */
	embeddedSubfieldList.clear();
	for (SubfieldIt subfieldIt = subfieldList.begin();
		subfieldIt != subfieldList.end(); subfieldIt++)
	{
		if (subfieldIt->id == '1') {
			if (embeddedSubfieldList.empty() == false) {
				break;
			}

			if (fieldTag == "" || fieldTag == subfieldIt->getEmbeddedTag()) {
				/* Append first subfield of embedded field. */
				embeddedSubfieldList.push_back(subfieldIt);
			}
		} else if (embeddedSubfieldList.empty() == false) {
			/* Append subfield to the embedded field. */
			embeddedSubfieldList.push_back(subfieldIt);
		}
	}

	return embeddedSubfieldList;
}

/*
 * Add subfield to the end of field.
 */
MarcRecord::SubfieldIt MarcRecord::Field::addSubfield(const Subfield &subfield)
{
	/* Append subfield to the list. */
	SubfieldIt subfieldIt = subfieldList.insert(subfieldList.end(), subfield);
	return subfieldIt;
}

MarcRecord::SubfieldIt MarcRecord::Field::addSubfield(char subfieldId,
	const std::string &subfieldData)
{
	/* Append subfield to the list. */
	SubfieldIt subfieldIt = subfieldList.insert(subfieldList.end(),
		Subfield(subfieldId, subfieldData));
	return subfieldIt;
}

/*
 * Add subfield to the field before specified subfield.
 */
MarcRecord::SubfieldIt MarcRecord::Field::addSubfieldBefore(SubfieldIt nextSubfieldIt,
	const Subfield &subfield)
{
	/* Append subfield to the list. */
	SubfieldIt subfieldIt = subfieldList.insert(nextSubfieldIt, subfield);
	return subfieldIt;
}

MarcRecord::SubfieldIt MarcRecord::Field::addSubfieldBefore(SubfieldIt nextSubfieldIt,
	char subfieldId, const std::string &subfieldData)
{
	/* Append subfield to the list. */
	SubfieldIt subfieldIt = subfieldList.insert(nextSubfieldIt,
		Subfield(subfieldId, subfieldData));
	return subfieldIt;
}

/*
 * Remove subfield from the field.
 */
void MarcRecord::Field::removeSubfield(SubfieldIt subfieldIt)
{
	/* Remove subfield from the list. */
	subfieldList.erase(subfieldIt);
}
