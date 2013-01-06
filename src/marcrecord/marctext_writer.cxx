/*
 * Copyright (c) 2012, Alexander Fronkin
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "marcrecord.h"
#include "marcrecord_tools.h"
#include "marctext_writer.h"

/*
 * Constructor.
 */
MarcTextWriter::MarcTextWriter(FILE *outputFile, const char *outputEncoding)
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
MarcTextWriter::~MarcTextWriter()
{
	/* Close output file. */
	close();
}

/*
 * Get last error code.
 */
MarcTextWriter::ErrorCode MarcTextWriter::getErrorCode(void)
{
	return m_errorCode;
}

/*
 * Get last error message.
 */
std::string & MarcTextWriter::getErrorMessage(void)
{
	return m_errorMessage;
}

/*
 * Open output file.
 */
bool MarcTextWriter::open(FILE *outputFile, const char *outputEncoding)
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
void MarcTextWriter::close(void)
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
 * Write record to MARC text file.
 */
bool MarcTextWriter::write(MarcRecord &record, const char *header, const char *footer)
{
	std::string recordBuf = header + record.toString() + footer;

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
