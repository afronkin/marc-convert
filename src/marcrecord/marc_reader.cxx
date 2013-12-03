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
#include "marc_reader.h"

using namespace marcrecord;

/*
 * Constructor.
 */
MarcReader::MarcReader()
{
	// Clear member variables.
	m_errorCode = OK;
	m_autoCorrectionMode = false;
}

/*
 * Get last error code.
 */
MarcReader::ErrorCode
MarcReader::getErrorCode(void)
{
	return m_errorCode;
}

/*
 * Get last error message.
 */
std::string &
MarcReader::getErrorMessage(void)
{
	return m_errorMessage;
}

/*
 * Return input file handle.
 */
FILE *
MarcReader::getInputFile()
{
	return m_inputFile;
}

/*
 * Set automatic error correction mode.
 */
void
MarcReader::setAutoCorrectionMode(bool autoCorrectionMode)
{
	m_autoCorrectionMode = autoCorrectionMode;
}
