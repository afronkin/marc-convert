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
	this->outputFile = outputFile == NULL ? stdout : outputFile;
	this->outputEncoding = outputEncoding == NULL ? "" : outputEncoding;
}

/*
 * Close output file.
 */
void MarcXmlWriter::close(void)
{
	/* Clear output stream parameters. */
	this->outputFile = NULL;
	this->outputEncoding = "";
}

/*
 * Write header to MARCXML file.
 */
void MarcXmlWriter::writeHeader(void)
{
	/* Write XML header. */
	if (outputEncoding != "") {
		fprintf(outputFile, "<?xml version=\"1.0\" encoding=\"%s\"?>\n",
			outputEncoding.c_str());
	} else {
		fputs("<?xml version=\"1.0\"?>\n", outputFile);
	}

	/* Write tag '<collection>'. */
	fputs("<collection xmlns=\"http://www.loc.gov/MARC21/slim\">\n", outputFile);
}

/*
 * Write footer to MARCXML file.
 */
void MarcXmlWriter::writeFooter(void)
{
	/* Write tag '</collection>'. */
	fputs("</collection>\n", outputFile);
}

/*
 * Write record to MARCXML file.
 */
bool MarcXmlWriter::write(MarcRecord &record)
{
	/* Write tag '<record>'. */
	fputs("  <record>\n", outputFile);

	/* Write record leader. */
	fprintf(outputFile, "    <leader>     %.*s</leader>\n",
		(int) sizeof(struct MarcRecord::Leader) - 5, (char *) &record.leader + 5);

	/* Iterate all fields. */
	for (MarcRecord::FieldIt fieldIt = record.fieldList.begin();
		fieldIt != record.fieldList.end(); fieldIt++)
	{
		std::string xmlData;

		if (fieldIt->tag < "010") {
			/* Write control field. */
			xmlData = serialize_xml(fieldIt->data);
			fprintf(outputFile, "    <controlfield tag=\"%s\">%.*s</controlfield>\n",
				fieldIt->tag.c_str(), (int) xmlData.size(), xmlData.c_str());
		} else {
			/* Write tag '<datafield>'. */
			fprintf(outputFile, "    <datafield tag=\"%s\" ind1=\"%c\" ind2=\"%c\">\n",
				fieldIt->tag.c_str(), fieldIt->ind1, fieldIt->ind2);

			/* Iterate all subfields. */
			for (MarcRecord::SubfieldIt subfieldIt = fieldIt->subfieldList.begin();
				subfieldIt != fieldIt->subfieldList.end(); subfieldIt++)
			{
				/* Write subfield. */
				xmlData = serialize_xml(subfieldIt->data);
				fprintf(outputFile, "      <subfield code=\"%c\">%.*s</subfield>\n",
					subfieldIt->id, (int) xmlData.size(), xmlData.c_str());
			}

			/* Write tag '</datafield>'. */
			fputs("    </datafield>\n", outputFile);
		}
	}

	/* Write tag '<record>'. */
	fputs("  </record>\n", outputFile);

	return true;
}
