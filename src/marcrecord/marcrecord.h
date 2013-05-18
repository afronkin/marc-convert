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

#ifndef MARCRECORD_MARCRECORD_H
#define MARCRECORD_MARCRECORD_H

#include <list>
#include <string>
#include <vector>

namespace marcrecord {

/*
 * MARC record class.
 */
class MarcRecord {
public:
	// Enum of MARC record format variants.
	enum FormatVariant {
		UNIMARC = 1,
		MARC21 = 2
	};

	// Enum of error codes.
	enum ErrorCode {
		OK = 0,
		ERROR = -1
	};

	#pragma pack(1)

	/*
	 * Structure of MARC record leader.
	 */
	struct Leader {
		// [ 0] Record length.
		char recordLength[5];
		// [ 5] Record status.
		char recordStatus;
		// [ 6] Type of record.
		char recordType;
		// [ 7] Bibliographic level.
		char bibliographicLevel;
		// [ 8] Hierarchical level code.
		char hierarchicalLevel;
		// [ 9] Undefined = '#'.
		char undefined1;
		// [10] Indicator length = '2'.
		char indicatorLength;
		// [11] Subfield identifier length = '2'.
		char subfieldIdLength;
		// [12] Base address of data.
		char baseAddress[5];
		// [17] Encoding level.
		char encodingLevel;
		// [18] Descriptive cataloguing form.
		char cataloguingForm;
		// [19] Undefined = '#'.
		char undefined2;
		// [20] Length of 'Length of field' = '4'.
		char lengthOfFieldLength;
		// [21] Length of 'Starting character position' = '5'.
		char startingPositionLength;
		// [22] Length of implementationdefined portion = '0'.
		char implementationDefinedLength;
		// [23] Undefined = '#'.
		char undefined3;
	};
	typedef struct Leader Leader;

	#pragma pack()

	// MARC field class.
	class Field;
	// MARC subfield class.
	class Subfield;

	// MARC (ISO 2709) reader class.
	friend class MarcReader;
	// MARC (ISO 2709) writer class.
	friend class MarcWriter;
	// MARCXML reader class.
	friend class MarcXmlReader;
	// MARCXML writer class.
	friend class MarcXmlWriter;
	// UNIMARCXML writer class.
	friend class UnimarcXmlWriter;

	// List of fields.
	typedef std::list<Field> FieldList;
	typedef FieldList::iterator FieldIt;
	// List of fields iterators.
	typedef std::list<FieldIt> FieldRefList;
	typedef FieldRefList::iterator FieldRefIt;

	// List of subfields.
	typedef std::list<Subfield> SubfieldList;
	typedef SubfieldList::iterator SubfieldIt;
	// List of subfields iterators.
	typedef std::list<SubfieldIt> SubfieldRefList;
	typedef SubfieldRefList::iterator SubfieldRefIt;

	// List of embedded fields.
	typedef std::list<SubfieldRefList> EmbeddedFieldList;
	typedef EmbeddedFieldList::iterator EmbeddedFieldIt;

private:
	// Variant of record format.
	FormatVariant m_formatVariant;

	// Record leader.
	Leader m_leader;
	// List of fields.
	FieldList m_fieldList;

public:
	// Constructors and destructor.
	MarcRecord();
	MarcRecord(FormatVariant formatVariant);
	~MarcRecord();

	// Clear record.
	void clear(void);

	// Get record format variant.
	FormatVariant getFormatVariant(void);
	// Set record format variant.
	void setFormatVariant(FormatVariant formatVariant);

	// Get record leader.
	Leader & getLeader(void);
	// Set record leader.
	void setLeader(const Leader &leader);
	void setLeader(const std::string &leaderData = "");

	// Get list of fields.
	FieldRefList getFields(const std::string &fieldTag = "");
	// Get field.
	FieldIt getField(const std::string &fieldTag);

	// Add field to the end of record.
	FieldIt addField(const Field &field);
	FieldIt addControlField(const std::string &fieldTag = "",
		const std::string &fieldData = "");
	FieldIt addDataField(const std::string &fieldTag = "",
		char fieldInd1 = ' ', char fieldInd2 = ' ');
	// Add field to the record before specified field.
	FieldIt addFieldBefore(FieldIt nextFieldIt, const Field &field);
	FieldIt addControlFieldBefore(FieldIt nextFieldIt,
		const std::string &fieldTag = "",
		const std::string &fieldData = "");
	FieldIt addDataFieldBefore(FieldIt nextFieldIt,
		const std::string &fieldTag = "",
		char fieldInd1 = ' ', char fieldInd2 = ' ');
	// Remove field from the record.
	void removeField(FieldIt fieldIt);

	// Format record to string for printing.
	std::string toString(void);

	// Return null field value.
	inline FieldIt nullField(void)
	{
		return m_fieldList.end();
	}
};

/*
 * MARC field class.
 */
class MarcRecord::Field {
public:
	// Types of field.
	enum Type {
		CONTROLFIELD,
		DATAFIELD
	};

public:
	// Type of field.
	enum Type m_type;

	// Field tag.
	std::string m_tag;
	// Indicator 1.
	char m_ind1;
	// Indicator 2.
	char m_ind2;
	// Data of control field.
	std::string m_data;
	// List of regular subfields.
	SubfieldList m_subfieldList;

public:
	// Constructors.
	Field(const std::string &tag = "", const std::string &data = "");
	Field(const std::string &tag, char ind1, char ind2);

	// Clear field data.
	void clear();

	// Set type of field to controlfield.
	void setControlFieldType(void);
	// Set type of field to datafield.
	void setDataFieldType(void);

	// Return true if file type is controlfield.
	bool isControlField(void);
	// Return true if file type is datafield.
	bool isDataField(void);

	// Get tag of field.
	std::string & getTag(void);
	// Set tag of field.
	void setTag(const std::string &data);

	// Get indicator 1 of data field.
	char & getInd1(void);
	// Get indicator 2 of data field.
	char & getInd2(void);
	// Set indicator 1 of data field.
	void setInd1(const char ind1);
	// Set indicator 2 of data field.
	void setInd2(const char ind2);

	// Get data of control field.
	std::string & getData(void);
	// Set data of control field.
	void setData(const std::string &data);

	// Get list of subfields.
	SubfieldRefList getSubfields(char subfieldId = ' ');
	// Get subfield.
	SubfieldIt getSubfield(char subfieldId);

	// Get list of embedded fields.
	EmbeddedFieldList getEmbeddedFields(const std::string &fieldTag = "");
	// Get embedded field.
	SubfieldRefList getEmbeddedField(const std::string &fieldTag);

	// Add subfield to the end of field.
	SubfieldIt addSubfield(const Subfield &subfield);
	SubfieldIt addSubfield(char subfieldId = ' ',
		const std::string &subfieldData = "");
	// Add subfield to the field before specified subfield.
	SubfieldIt addSubfieldBefore(SubfieldIt nextSubfieldIt,
		const Subfield &subfield);
	SubfieldIt addSubfieldBefore(SubfieldIt nextSubfieldIt,
		char subfieldId = ' ', const std::string &subfieldData = "");
	// Remove subfield from the field.
	void removeSubfield(SubfieldIt subfieldIt);

	// Return null subfield value.
	inline SubfieldIt nullSubfield()
	{
		return m_subfieldList.end();
	}

	// Format field to string for printing.
	std::string toString();
};

/*
 * MARC subfield class.
 */
class MarcRecord::Subfield {
public:
	// Subfield identifier.
	char m_id;
	// Subfield data.
	std::string m_data;

public:
	// Constructor.
	Subfield(char id = ' ', const std::string &data = "");

	// Clear subfield data.
	void clear(void);

	// Get identifier of subfield.
	char & getId(void);
	// Set identifier of subfield.
	void setId(const char &id);

	// Get data of subfield.
	std::string & getData(void);
	// Set data of subfield.
	void setData(const std::string &data);

	// Check presence of embedded field.
	bool isEmbedded(void);
	// Get tag of embedded field.
	std::string getEmbeddedTag(void);
	// Get indicator 1 of embedded field.
	char getEmbeddedInd1(void);
	// Get indicator 2 of embedded field.
	char getEmbeddedInd2(void);
	// Get data of embedded field.
	std::string getEmbeddedData(void);
};

} // namespace marcrecord

#endif // MARCRECORD_MARCRECORD_H
