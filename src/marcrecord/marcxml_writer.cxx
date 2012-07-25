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

#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marcxml_writer.h"

/*
 * Constructor.
 */
MarcXmlWriter::MarcXmlWriter(FILE *outputFile, const char *outputEncoding)
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
MarcXmlWriter::~MarcXmlWriter()
{
	/* Close output file. */
	close();
}

/*
 * Open output file.
 */
void MarcXmlWriter::open(FILE *outputFile, const char *outputEncoding)
{
	/* Initialize output stream parameters. */
	m_outputFile = outputFile == NULL ? stdout : outputFile;
	m_outputEncoding = outputEncoding == NULL ? "" : outputEncoding;
}

/*
 * Close output file.
 */
void MarcXmlWriter::close(void)
{
	/* Clear output stream parameters. */
	m_outputFile = NULL;
	m_outputEncoding = "";
}

/*
 * Write header to MARCXML file.
 */
void MarcXmlWriter::writeHeader(void)
{
	/* Write XML header. */
	if (m_outputEncoding != "") {
		fprintf(m_outputFile, "<?xml version=\"1.0\" encoding=\"%s\"?>\n",
			m_outputEncoding.c_str());
	} else {
		fputs("<?xml version=\"1.0\"?>\n", m_outputFile);
	}

	/* Write tag '<collection>'. */
	fputs("<collection xmlns=\"http://www.loc.gov/MARC21/slim\">\n", m_outputFile);
}

/*
 * Write footer to MARCXML file.
 */
void MarcXmlWriter::writeFooter(void)
{
	/* Write tag '</collection>'. */
	fputs("</collection>\n", m_outputFile);
}

/*
 * Write record to MARCXML file.
 */
bool MarcXmlWriter::write(MarcRecord &record)
{
	/* Write tag '<record>'. */
	fputs("  <record>\n", m_outputFile);

	/* Write record leader. */
	fprintf(m_outputFile, "    <leader>     %.*s</leader>\n",
		(int) sizeof(struct MarcRecord::Leader) - 5, (char *) &record.m_leader + 5);

	/* Iterate all fields. */
	for (MarcRecord::FieldIt fieldIt = record.m_fieldList.begin();
		fieldIt != record.m_fieldList.end(); fieldIt++)
	{
		std::string xmlData;

		if (fieldIt->m_tag < "010") {
			/* Write control field. */
			xmlData = serialize_xml(fieldIt->m_data);
			fprintf(m_outputFile, "    <controlfield tag=\"%s\">%.*s</controlfield>\n",
				fieldIt->m_tag.c_str(), (int) xmlData.size(), xmlData.c_str());
		} else {
			/* Write tag '<datafield>'. */
			fprintf(m_outputFile,
				"    <datafield tag=\"%s\" ind1=\"%c\" ind2=\"%c\">\n",
				fieldIt->m_tag.c_str(), fieldIt->m_ind1, fieldIt->m_ind2);

			/* Iterate all subfields. */
			for (MarcRecord::SubfieldIt subfieldIt = fieldIt->m_subfieldList.begin();
				subfieldIt != fieldIt->m_subfieldList.end(); subfieldIt++)
			{
				/* Write subfield. */
				xmlData = serialize_xml(subfieldIt->m_data);
				fprintf(m_outputFile,
					"      <subfield code=\"%c\">%.*s</subfield>\n",
					subfieldIt->m_id, (int) xmlData.size(), xmlData.c_str());
			}

			/* Write tag '</datafield>'. */
			fputs("    </datafield>\n", m_outputFile);
		}
	}

	/* Write tag '<record>'. */
	fputs("  </record>\n", m_outputFile);

	return true;
}
