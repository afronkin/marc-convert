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

#ifndef MARCRECORD_MARCISO_READER_H
#define MARCRECORD_MARCISO_READER_H

#include <iconv.h>
#include <string>
#include "marc_reader.h"
#include "marcrecord.h"

namespace marcrecord {

/*
 * ISO 2709 records reader.
 */
class MarcIsoReader : public MarcReader {
protected:
	// Iconv descriptor for input encoding.
	iconv_t m_iconvDesc;

private:
	// Parse field from ISO 2709 buffer.
	inline MarcRecord::Field parseField(const std::string &fieldTag,
		const char *fieldData, unsigned int fieldLength);
	// Parse subfield.
	MarcRecord::Subfield parseSubfield(const char *fieldData,
		unsigned int subfieldStartPos, unsigned int subfieldEndPos);

public:
	// Constructor.
	MarcIsoReader(FILE *inputFile = NULL,
		const char *inputEncoding = NULL);
	// Destructor.
	~MarcIsoReader();

	// Open input file.
	bool open(FILE *inputFile, const char *inputEncoding = NULL);
	// Close input file.
	void close(void);
	// Read next record from file.
	bool next(MarcRecord &record);

	// Parse record from ISO 2709 buffer.
	bool parse(const char *recordBuf, unsigned int recordBufLen,
		MarcRecord &record);
};

} // namespace marcrecord

#endif // MARCRECORD_MARCISO_READER_H
