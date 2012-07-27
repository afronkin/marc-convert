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

#if !defined(MARC_WRITER_H)
#define MARC_WRITER_H

#include <iconv.h>
#include <string>
#include "marcrecord.h"

/*
 * MARC (ISO 2709) records writer.
 */
class MarcWriter {
public:
	/* Error codes. */
	enum ErrorCode {
		OK = 0,
		ERROR_ICONV = -1,
		ERROR_DATASIZE = -2,
		ERROR_IO = -3
	};

protected:
	/* Code of last error. */
	ErrorCode m_errorCode;
	/* Message of last error. */
	std::string m_errorMessage;

	/* Output ISO 2709 file. */
	FILE *m_outputFile;
	/* Encoding of output ISO 2709 file. */
	std::string m_outputEncoding;
	/* Iconv descriptor for output encoding. */
	iconv_t m_iconvDesc;

public:
	/* Constructor. */
	MarcWriter(FILE *outputFile = NULL, const char *outputEncoding = NULL);
	/* Destructor. */
	~MarcWriter();

	/* Get last error code. */
	ErrorCode getErrorCode(void);
	/* Get last error message. */
	std::string & getErrorMessage(void);

	/* Open output file. */
	bool open(FILE *outputFile, const char *outputEncoding = NULL);
	/* Close output file. */
	void close(void);

	/* Write record to ISO 2709 file. */
	bool write(MarcRecord &record);
};

#endif /* MARC_WRITER_H */
