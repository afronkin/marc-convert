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
#include "unimarcxml_writer.h"

using namespace marcrecord;

/*
 * Constructor.
 */
UnimarcXmlWriter::UnimarcXmlWriter(FILE *outputFile,
	const char *outputEncoding)
	: MarcWriter()
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
UnimarcXmlWriter::~UnimarcXmlWriter()
{
	// Close output file.
	close();
}

/*
 * Open output file.
 */
bool
UnimarcXmlWriter::open(FILE *outputFile, const char *outputEncoding)
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
UnimarcXmlWriter::close(void)
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
 * Write record to output file.
 */
bool
UnimarcXmlWriter::write(MarcRecord &record)
{
	std::string recordBuf = "";

	// Append tag '<record>'.
	recordBuf += "  <record>\n";

	// Append record leader.
	recordBuf += "    <leader>     "
		+ std::string((char *) &record.m_leader + 5,
		(size_t) sizeof(MarcRecord::Leader) - 5)
		+ "</leader>\n";

	// Iterate all fields.
	for (MarcRecord::FieldIt fieldIt = record.m_fieldList.begin();
		fieldIt != record.m_fieldList.end(); fieldIt++)
	{
		std::string xmlData;

		if (fieldIt->m_tag < "010") {
			// Append control field.
			xmlData = serialize_xml(fieldIt->m_data);
			recordBuf += "    <controlfield tag=\""
				+ fieldIt->m_tag + "\">"
				+ xmlData + "</controlfield>\n";
		} else {
			// Append data field.
			appendDataField(recordBuf, fieldIt);
		}
	}

	// Append tag '<record>'.
	recordBuf += "  </record>\n";

	if (m_iconvDesc == (iconv_t) -1) {
		// Write UNIMARCXML record.
		if (fwrite(recordBuf.c_str(), recordBuf.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		// Write UNIMARCXML record with encoding conversion.
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, recordBuf, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}

/*
 * Write header to output file.
 */
bool
UnimarcXmlWriter::writeHeader(void)
{
	std::string header = "";

	// Create UNIMARCXML header.
	if (m_outputEncoding != "") {
		header += "<?xml version=\"1.0\" encoding=\""
			+ m_outputEncoding + "\"?>\n";
	} else {
		header = "<?xml version=\"1.0\"?>\n";
	}

	header += "<collection xmlns="
		"\"http://www.rusmarc.ru/shema/UNISlim.xsd\">\n";

	if (m_iconvDesc == (iconv_t) -1) {
		// Write UNIMARCXML header.
		if (fwrite(header.c_str(), header.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		// Write UNIMARCXML header with encoding conversion.
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, header, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}

/*
 * Write footer to output file.
 */
bool
UnimarcXmlWriter::writeFooter(void)
{
	std::string footer = "</collection>\n";

	if (m_iconvDesc == (iconv_t) -1) {
		// Write UNIMARCXML footer.
		if (fwrite(footer.c_str(), footer.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		// Write UNIMARCXML footer with encoding conversion.
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, footer, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1,
			m_outputFile) != 1)
		{
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}

/*
 * Append data field to UNIMARCXML file to record buffer.
 */
void
UnimarcXmlWriter::appendDataField(std::string &recordBuf,
	MarcRecord::FieldIt &fieldIt)
{
	// Append tag '<datafield>'.
	recordBuf += "    <datafield tag=\"" + fieldIt->m_tag
		+ "\" ind1=\"" + fieldIt->m_ind1
		+ "\" ind2=\"" + fieldIt->m_ind2 + "\">\n";

	// Iterate all subfields.
	MarcRecord::SubfieldIt subfieldIt = fieldIt->m_subfieldList.begin();
	bool isEmbeddedDataField = false;
	for (; subfieldIt != fieldIt->m_subfieldList.end();
		subfieldIt++)
	{
		std::string xmlData;

		if (subfieldIt->isEmbedded()) {
			if (isEmbeddedDataField) {
				// Append embedded data field footer.
				recordBuf = recordBuf
					+ "        </datafield>\n"
					+ "      </s1>\n";
			}

			// Append embedded field header.
			std::string embeddedTag = subfieldIt->getEmbeddedTag();
			if (embeddedTag < "010") {
				// Append embedded control field.
				std::string embeddedData =
					subfieldIt->getEmbeddedData();
				xmlData = serialize_xml(embeddedData);
				recordBuf = recordBuf
					+ "      <s1>\n"
					+ "        <controlfield tag=\""
					+ embeddedTag + "\">"
					+ xmlData + "</controlfield>\n"
					+ "      </s1>\n";
				isEmbeddedDataField = false;
			} else {
				recordBuf = recordBuf
					+ "      <s1>\n"
					+ "        <datafield tag=\""
					+ embeddedTag
					+ "\" ind1=\""
					+ subfieldIt->getEmbeddedInd1()
					+ "\" ind2=\""
					+ subfieldIt->getEmbeddedInd2()
					+ "\">\n";
				isEmbeddedDataField = true;
			}
			continue;
		}

		// Append indent for embedded field.
		if (isEmbeddedDataField) {
			recordBuf += "    ";
		}

		// Append subfield.
		xmlData = serialize_xml(subfieldIt->m_data);
		recordBuf = recordBuf
			+ "      <subfield code=\""
			+ subfieldIt->m_id + "\">"
			+ xmlData + "</subfield>\n";
	}

	// Append embedded data field footer.
	if (isEmbeddedDataField) {
		recordBuf = recordBuf
			+ "        </datafield>\n"
			+ "      </s1>\n";
	}

	// Append tag '</datafield>'.
	recordBuf += "    </datafield>\n";
}
