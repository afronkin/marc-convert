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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "marcrecord_tools.h"

/*
 * Print formatted output to std::string.
 */
int snprintf(std::string &s, size_t n, const char *format, ...)
{
	va_list ap;
	char *buf;
	int resultCode;

	buf = (char *) malloc(n + 1);
	va_start(ap, format);
	resultCode = vsnprintf(buf, n + 1, format, ap);
	va_end(ap);

	if (resultCode >= 0) {
		s.append(buf);
	}
	free(buf);

	return resultCode;
}

/*
 * Serialize XML string.
 */
std::string serialize_xml(std::string &s)
{
	std::string dest = "";

	/* Copy characters from source sting to destination string, replace special characters. */
	for (std::string::iterator it = s.begin(); it != s.end(); it++) {
		unsigned char c = *it;

		switch (c) {
		case '"':
			dest.append("&quot;");
			break;
		case '&':
			dest.append("&amp;");
			break;
		case '\'':
			dest.append("&apos;");
			break;
		case '<':
			dest.append("&lt;");
			break;
		case '>':
			dest.append("&gt;");
			break;
		default:
			dest += c;
			break;
		}
	}

	return dest;
}

/*
 * Verify that all string characters are decimal digits in ASCII encoding.
 */
int is_numeric(const char *s, size_t n)
{
	char *p, *s_end;

	s_end = (char *) s + n;
	for (p = (char *) s; p < s_end && *p != '\0'; p++) {
		if (*p < 0x30 || *p > 0x39) {
			return 0;
		}
	}

	return 1;
}

/*
 * Convert encoding for std::string.
 */
bool iconv(iconv_t iconv_desc, const std::string &src, std::string &dest)
{
	char buf[4096];
#if defined(WIN32)
	const char *p = src.c_str();
#else
	char *p = (char *) src.c_str();
#endif
	size_t src_len = src.size();

	dest = "";
	while (src_len > 0) {
		size_t dest_len = sizeof(buf);
		char *q = buf;
		if (iconv(iconv_desc, &p, &src_len, &q, &dest_len) == (size_t) -1) {
			if (errno != E2BIG) {
				return false;
			}
		}

		dest.append(buf, sizeof(buf) - dest_len);
	}

	return true;
}

/*
 * Convert encoding for std::string.
 */
bool iconv(iconv_t iconv_desc, const char *src, size_t len, std::string &dest)
{
	char buf[4096];
#if defined(WIN32)
	const char *p = src;
#else
	char *p = (char *) src;
#endif /* WIN32 */
	size_t src_len = len;

	dest = "";
	while (src_len > 0) {
		size_t dest_len = sizeof(buf);
		char *q = buf;
		if (iconv(iconv_desc, &p, &src_len, &q, &dest_len) == (size_t) -1) {
			if (errno != E2BIG) {
				return false;
			}
		}

		dest.append(buf, sizeof(buf) - dest_len);
	}

	return true;
}
