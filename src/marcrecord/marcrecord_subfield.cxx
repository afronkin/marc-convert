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

#include "marcrecord.h"

/*
 * Constructor.
 */
MarcRecord::Subfield::Subfield(char id, const std::string &data)
{
	clear();
	m_id = id;
	m_data = data;
}

/*
 * Clear subfield data.
 */
void MarcRecord::Subfield::clear(void)
{
	m_id = ' ';
	m_data.erase();
}

/*
 * Get identifier of subfield.
 */
char & MarcRecord::Subfield::getId(void)
{
	return m_id;
}

/*
 * Set identifier of subfield.
 */
void MarcRecord::Subfield::setId(const char &id)
{
	m_id = id;
}

/*
 * Get data of subfield.
 */
std::string & MarcRecord::Subfield::getData(void)
{
	return m_data;
}

/*
 * Set data of subfield.
 */
void MarcRecord::Subfield::setData(const std::string &data)
{
	m_data = data;
}

/*
 * Check presence of embedded field.
 */
bool MarcRecord::Subfield::isEmbedded(void)
{
	return (m_id == '1' ? true : false);
}

/*
 * Get tag of embedded field.
 */
std::string MarcRecord::Subfield::getEmbeddedTag(void)
{
	if (m_id != '1') {
		return "";
	}

	return m_data.substr(0, 3);
}

/*
 * Get indicator 1 of embedded field.
 */
char MarcRecord::Subfield::getEmbeddedInd1(void)
{
	if (m_id != '1' || m_data.substr(0, 3) < "010") {
		return '?';
	}

	return m_data[3];
}

/*
 * Get indicator 2 of embedded field.
 */
char MarcRecord::Subfield::getEmbeddedInd2(void)
{
	if (m_id != '1' || m_data.substr(0, 3) < "010") {
		return '?';
	}

	return m_data[4];
}

/*
 * Get data of embedded field.
 */
std::string MarcRecord::Subfield::getEmbeddedData(void)
{
	if (m_id != '1' || m_data.substr(0, 3) >= "010") {
		return "";
	}

	return m_data.substr(3);
}
