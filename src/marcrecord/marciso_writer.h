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

#ifndef MARCRECORD_MARCISO_WRITER_H
#define MARCRECORD_MARCISO_WRITER_H

#include <iconv.h>
#include <string>
#include "marc_writer.h"
#include "marcrecord.h"

namespace marcrecord {

/*
 *ISO 2709 records writer.
 */
class MarcIsoWriter : public MarcWriter {
protected:
	// Iconv descriptor for output encoding.
	iconv_t m_iconvDesc;

private:
	// Append control field data to the write buffer.
	int appendControlField(char *fieldData, MarcRecord::FieldIt &fieldIt);
	// Append subfield data to the write buffer.
	int appendSubfield(char *fieldData,
		MarcRecord::SubfieldIt &subfieldIt);

public:
	// Constructor.
	MarcIsoWriter(FILE *outputFile = NULL,
		const char *outputEncoding = NULL);
	// Destructor.
	~MarcIsoWriter();

	// Open output file.
	bool open(FILE *outputFile, const char *outputEncoding = NULL);
	// Close output file.
	void close(void);
	// Write record to output file.
	bool write(MarcRecord &record);
};

} // namespace marcrecord

#endif // MARCRECORD_MARCISO_WRITER_H
