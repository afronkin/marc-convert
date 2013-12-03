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

#ifndef MARCRECORD_MARC_WRITER_H
#define MARCRECORD_MARC_WRITER_H

#include <string>
#include "marcrecord.h"

namespace marcrecord {

/*
 * MARC records writer.
 */
class MarcWriter {
public:
	// Error codes.
	enum ErrorCode {
		OK = 0,
		ERROR_ICONV = -1,
		ERROR_DATASIZE = -2,
		ERROR_IO = -3
	};

protected:
	// Code of last error.
	ErrorCode m_errorCode;
	// Message of last error.
	std::string m_errorMessage;

	// Output file.
	FILE *m_outputFile;
	// Encoding of output file.
	std::string m_outputEncoding;

public:
	// Constructor.
	MarcWriter();

	// Get last error code.
	ErrorCode getErrorCode(void);
	// Get last error message.
	std::string & getErrorMessage(void);

	// Return output file handle.
	FILE *getOutputFile();

	// Open output file.
	virtual bool open(FILE *outputFile,
		const char *outputEncoding = NULL) = 0;
	// Close output file.
	virtual void close(void) = 0;
	// Write record to output file.
	virtual bool write(MarcRecord &record) = 0;
};

} // namespace marcrecord

#endif // MARCRECORD_MARC_WRITER_H
