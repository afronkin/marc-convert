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

#if !defined(MARCXML_READER_H)
#define MARCXML_READER_H

#include <string>
#include "marcrecord.h"
#include "expat/expat.h"

/*
 * MARCXML records reader.
 */
class MarcXmlReader {
public:
	/* XML parser state structure definition. */
	struct XmlParserState {
		XML_Parser xmlParser;
		bool done;
		bool paused;
		std::string parentTag;

		MarcRecord *record;
		MarcRecord::FieldIt fieldIt;
		MarcRecord::SubfieldIt subfieldIt;
		std::string characterData;
	};

	/* Error codes. */
	enum ErrorCode {
		OK = 0,
		END_OF_FILE = 1,
		ERROR_XML_PARSER = -1
	};

protected:
	/* Code of last error. */
	ErrorCode m_errorCode;
	/* Message of last error. */
	std::string m_errorMessage;

	/* Input MARCXML file. */
	FILE *m_inputFile;
	/* Encoding of input MARCXML file. */
	std::string m_inputEncoding;

	/* Automatic error correction mode. */
	bool m_autoCorrectionMode;

	/* XML parser. */
	XML_Parser m_xmlParser;
	/* XML parser state. */
	struct XmlParserState m_parserState;
	/* Record buffer. */
	char m_buffer[4096];

public:
	/* Constructor. */
	MarcXmlReader(FILE *inputFile = NULL, const char *inputEncoding = NULL);
	/* Destructor. */
	~MarcXmlReader();

	/* Get last error code. */
	ErrorCode getErrorCode(void);
	/* Get last error message. */
	std::string & getErrorMessage(void);

	/* Set automatic error correction mode. */
	void setAutoCorrectionMode(bool autoCorrectionMode = true);

	/* Open input file and initialize parser. */
	void open(FILE *inputFile, const char *inputEncoding = NULL);
	/* Close input file and finalize parser. */
	void close(void);

	/* Read next record from MARCXML file. */
	bool next(MarcRecord &record);
};

#endif /* MARCXML_READER_H */
