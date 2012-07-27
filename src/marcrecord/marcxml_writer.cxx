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

#include <errno.h>
#include <stdio.h>
#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marcxml_writer.h"

/*
 * Constructor.
 */
MarcXmlWriter::MarcXmlWriter(FILE *outputFile, const char *outputEncoding)
{
	/* Clear member variables. */
	m_iconvDesc = (iconv_t) -1;

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
MarcXmlWriter::~MarcXmlWriter()
{
	/* Close output file. */
	close();
}

/*
 * Get last error code.
 */
MarcXmlWriter::ErrorCode MarcXmlWriter::getErrorCode(void)
{
	return m_errorCode;
}

/*
 * Get last error message.
 */
std::string & MarcXmlWriter::getErrorMessage(void)
{
	return m_errorMessage;
}

/*
 * Open output file.
 */
bool MarcXmlWriter::open(FILE *outputFile, const char *outputEncoding)
{
	/* Clear error code and message. */
	m_errorCode = OK;
	m_errorMessage = "";

	/* Initialize output stream parameters. */
	m_outputFile = outputFile == NULL ? stdout : outputFile;
	m_outputEncoding = outputEncoding == NULL ? "" : outputEncoding;

	/* Initialize encoding conversion. */
	if (outputEncoding == NULL
		|| strcmp(outputEncoding, "UTF-8") == 0 || strcmp(outputEncoding, "utf-8") == 0)
	{
		m_iconvDesc = (iconv_t) -1;
	} else {
		/* Create iconv descriptor for output encoding conversion from UTF-8. */
		m_iconvDesc = iconv_open(outputEncoding, "UTF-8");
		if (m_iconvDesc == (iconv_t) -1) {
			m_errorCode = ERROR_ICONV;
			if (errno == EINVAL) {
				m_errorMessage = "encoding conversion is not supported";
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
void MarcXmlWriter::close(void)
{
	/* Finalize iconv. */
	if (m_iconvDesc != (iconv_t) -1) {
		iconv_close(m_iconvDesc);
	}

	/* Clear member variables. */
	m_errorCode = OK;
	m_errorMessage = "";
	m_outputFile = NULL;
	m_outputEncoding = "";
	m_iconvDesc = (iconv_t) -1;
}

/*
 * Write header to MARCXML file.
 */
bool MarcXmlWriter::writeHeader(void)
{
	std::string header = "";

	/* Create MARCXML header. */
	if (m_outputEncoding != "") {
		header += "<?xml version=\"1.0\" encoding=\"" + m_outputEncoding + "\"?>\n";
	} else {
		header = "<?xml version=\"1.0\"?>\n";
	}

	header += "<collection xmlns=\"http://www.loc.gov/MARC21/slim\">\n";

	if (m_iconvDesc == (iconv_t) -1) {
		/* Write MARCXML header. */
		if (fwrite(header.c_str(), header.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		/* Write MARCXML header with encoding conversion. */
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, header, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}

/*
 * Write footer to MARCXML file.
 */
bool MarcXmlWriter::writeFooter(void)
{
	std::string footer = "</collection>\n";

	if (m_iconvDesc == (iconv_t) -1) {
		/* Write MARCXML footer. */
		if (fwrite(footer.c_str(), footer.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		/* Write MARCXML footer with encoding conversion. */
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, footer, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}

/*
 * Write record to MARCXML file.
 */
bool MarcXmlWriter::write(MarcRecord &record)
{
	std::string recordBuf = "";

	/* Append tag '<record>'. */
	recordBuf += "  <record>\n";

	/* Append record leader. */
	recordBuf += "    <leader>     " + std::string((char *) &record.m_leader + 5,
		(size_t) sizeof(struct MarcRecord::Leader) - 5) + "</leader>\n";

	/* Iterate all fields. */
	for (MarcRecord::FieldIt fieldIt = record.m_fieldList.begin();
		fieldIt != record.m_fieldList.end(); fieldIt++)
	{
		std::string xmlData;

		if (fieldIt->m_tag < "010") {
			/* Append control field. */
			xmlData = serialize_xml(fieldIt->m_data);
			recordBuf += "    <controlfield tag=\"" + fieldIt->m_tag + "\">"
				+ xmlData + "</controlfield>\n";
		} else {
			/* Append tag '<datafield>'. */
			recordBuf += "    <datafield tag=\"" + fieldIt->m_tag
				+ "\" ind1=\"" + fieldIt->m_ind1
				+ "\" ind2=\"" + fieldIt->m_ind2 + "\">\n";

			/* Iterate all subfields. */
			for (MarcRecord::SubfieldIt subfieldIt = fieldIt->m_subfieldList.begin();
				subfieldIt != fieldIt->m_subfieldList.end(); subfieldIt++)
			{
				/* Append subfield. */
				xmlData = serialize_xml(subfieldIt->m_data);
				recordBuf = recordBuf + "      <subfield code=\""
					+ subfieldIt->m_id + "\">" + xmlData + "</subfield>\n";
			}

			/* Append tag '</datafield>'. */
			recordBuf += "    </datafield>\n";
		}
	}

	/* Append tag '<record>'. */
	recordBuf += "  </record>\n";

	if (m_iconvDesc == (iconv_t) -1) {
		/* Write MARCXML record. */
		if (fwrite(recordBuf.c_str(), recordBuf.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	} else {
		/* Write MARCXML record with encoding conversion. */
		std::string iconvBuf;
		if (!iconv(m_iconvDesc, recordBuf, iconvBuf)) {
			m_errorCode = ERROR_ICONV;
			m_errorMessage = "encoding conversion failed";
			return false;
		}
		if (fwrite(iconvBuf.c_str(), iconvBuf.size(), 1, m_outputFile) != 1) {
			m_errorCode = ERROR_IO;
			m_errorMessage = "i/o operation failed";
			return false;
		}
	}

	return true;
}
