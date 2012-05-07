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

/* Version: 1.0 (6 May 2012) */

#include <errno.h>
#include <iconv.h>
#include <math.h>
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
*/
#include "marcrecord/marcrecord.h"
#include "marcrecord/marc_reader.h"
#include "marcrecord/marc_writer.h"
#include "marcrecord/marcxml_reader.h"
#include "marcrecord/marcxml_writer.h"

/* Record format variants */
enum RecordFormat { ISO2709, MARCXML, TEXT };

/* Application options structure. */
struct Options {
	int verboseLevel;
	const char *encoding;
	int skipRecs;
	int numRecs;
	const char *inputFileName;
	const char *outputFileName;
	enum RecordFormat inputFormat;
	enum RecordFormat outputFormat;
	const char *inputEncoding;
	const char *outputEncoding;
};

/* Application options. */
static struct Options options = { 0, NULL, 0, 0, NULL, NULL, ISO2709, TEXT, NULL, NULL };

/* Convert encoding for MarcRecord. */
bool iconvRecord(iconv_t iconvDesc, MarcRecord &record);
/* Convert encoding for std::string. */
bool iconv(iconv_t iconvDesc, const std::string &src, std::string &dest);

/*
 * Convert records from MARC file.
 */
bool convertFile(void)
{
	FILE *inputFile = NULL, *outputFile = NULL;
	iconv_t iconvDesc = (iconv_t) -1;

	try {
		/* Open input file. */
		if (options.inputFileName == NULL || strcmp(options.inputFileName, "-") == 0) {
			inputFile = stdin;
		} else {
			inputFile = fopen(options.inputFileName, "rb");
			if (inputFile == NULL) {
				throw std::string("can't open input file");
			}
		}

		/* Open output file. */
		if (options.outputFileName == NULL || strcmp(options.outputFileName, "-") == 0) {
			outputFile = stdout;
		} else {
			outputFile = fopen(options.outputFileName, "wb");
			if (outputFile == NULL) {
				throw std::string("can't open output file.");
			}
		}

		/* Initialize iconv when specified different input and output encodings. */
		if (options.inputEncoding && options.outputEncoding
			&& strcmp(options.inputEncoding, options.outputEncoding) != 0)
		{
			iconvDesc = iconv_open(options.outputEncoding, options.inputEncoding);
			if (iconvDesc == (iconv_t) -1) {
				if (errno == EINVAL) {
					throw std::string("conversion from '")
						+ options.inputEncoding + "' to '"
						+ options.outputEncoding + "' is not supported";
				} else {
					throw std::string("can't initialize iconv");
				}
			}
		}

		/* Open input file in MarcReader or MarcXmlReader. */
		MarcReader marcReader;
		MarcXmlReader marcXmlReader;

		switch (options.inputFormat) {
		case ISO2709:
			marcReader.open(inputFile);
			break;
		case MARCXML:
			marcXmlReader.open(inputFile);
			break;
		default:
			throw std::string("wrong input format specified");
		}

		/* Open output file in MarcWriter or MarcXmlWriter. */
		MarcWriter marcWriter;
		MarcXmlWriter marcXmlWriter;

		switch (options.outputFormat) {
		case ISO2709:
			marcWriter.open(outputFile);
			break;
		case MARCXML:
			marcXmlWriter.open(outputFile, options.outputEncoding);
			marcXmlWriter.writeHeader();
			break;
		case TEXT:
			break;
		default:
			throw std::string("wrong input format specified");
		}

		/* Get process start time. */
		time_t startTime, curTime, prevTime;
		time(&startTime);
		prevTime = startTime;

		/* Iterate records in input file. */
		int recNo, numBadRecs = 0;
		for (recNo = 1;; recNo++) {
			/* Read record from input file. */
			MarcRecord record;
			bool readStatus;

			switch (options.inputFormat) {
			case ISO2709:
				readStatus = marcReader.next(record);
				if (!readStatus && marcReader.getErrorCode()
					!= MarcReader::END_OF_FILE)
				{
					throw marcReader.getErrorMessage();
				}
				break;
			case MARCXML:
				readStatus = marcXmlReader.next(record);
				if (!readStatus && marcXmlReader.getErrorCode()
					!= MarcXmlReader::END_OF_FILE)
				{
					throw marcXmlReader.getErrorMessage();
				}
				break;
			default:
				throw std::string("unknown input format");
			}

			/* Exit when end of file reached. */
			if (!readStatus) {
				break;
			}

			/* Convert record encoding. */
			if (iconvDesc != (iconv_t) -1) {
				if (!iconvRecord(iconvDesc, record)) {
					throw std::string("can't convert record encoding");
				}
			}

			/* Write record to output file. */
			switch (options.outputFormat) {
			case ISO2709:
				marcWriter.write(record);
				break;
			case MARCXML:
				marcXmlWriter.write(record);
				break;
			case TEXT:
				/* Display record header. */
				if (recNo > 1) {
					fprintf(outputFile, "\nRecord %d\n", recNo);
				} else {
					fprintf(outputFile, "Record %d\n", recNo);
				}

				/* Write record. */
				std::string textRecord = record.toString();
				fwrite(textRecord.c_str(), textRecord.size(), 1, outputFile);
				putc('\n', outputFile);
				break;
			}

			/* Print process status. */
			time(&curTime);
			if (curTime > prevTime) {
				if (options.verboseLevel > 1) {
					fprintf(stderr, "\rRecord: %d", recNo);
				}

				prevTime = curTime;
				fflush(stderr);
			}
		}

		/* Write MARCXML footer to output file. */
		if (options.outputFormat == MARCXML) {
			marcXmlWriter.writeFooter();
		}

		/* Finalize iconv. */
		if (iconvDesc != (iconv_t) -1) {
			iconv_close(iconvDesc);
			iconvDesc = (iconv_t) -1;
		}

		/* Close files. */
		if (inputFile != stdin) {
			fclose(inputFile);
			inputFile = NULL;
		}
		if (outputFile != stdout) {
			fclose(outputFile);
			outputFile = NULL;
		}

		/* Print used time. */
		if (options.verboseLevel > 0) {
			time(&curTime);
			double usedTime = (double) (curTime - startTime);
			int usedHours = (int) floor(usedTime / 3600);
			int usedMinutes = (int) floor((usedTime - (usedHours * 3600)) / 60);
			int usedSeconds = (int) (usedTime - (usedHours * 3600) - (usedMinutes * 60));

			if (options.verboseLevel > 1) {
				fputc('\r', stderr);
			}
			fprintf(stderr, "Total records: %d\n", recNo - 1);
			fprintf(stderr, "Records with errors: %d\n", numBadRecs);
			fprintf(stderr, "Done in %d:%02d:%02d.\n",
				usedHours, usedMinutes, usedSeconds);
		}
	} catch (std::string errorMessage) {
		/* Print error message. */
		fprintf(stderr, "Error: %s.\n", errorMessage.c_str());

		/* Finalize iconv. */
		if (iconvDesc != (iconv_t) -1) {
			iconv_close(iconvDesc);
		}

		/* Close files. */
		if (inputFile && inputFile != stdin) {
			fclose(inputFile);
		}
		if (outputFile && outputFile != stdout) {
			fclose(outputFile);
		}

		return false;
	}

	return true;
}

/*
 * Convert encoding for MarcRecord.
 */
bool iconvRecord(iconv_t iconvDesc, MarcRecord &record)
{
	/* Get list of specified fields. */
	MarcRecord::FieldRefList fieldList = record.getFields();
	for (MarcRecord::FieldRefIt fieldIt = fieldList.begin();
		fieldIt != fieldList.end(); fieldIt++)
	{
		if ((*fieldIt)->isControlField()) {
			/* Convert data of control field. */
			std::string &fieldData = (*fieldIt)->getData();
			std::string fieldDataRecoded;
			if (!iconv(iconvDesc, fieldData, fieldDataRecoded)) {
				return false;
			}
			fieldData = fieldDataRecoded;
		} else {
			/* Get list of specified subfields from field. */
			MarcRecord::SubfieldRefList subfieldList = (*fieldIt)->getSubfields();
			/* Convert data of subfields. */
			for (MarcRecord::SubfieldRefIt subfieldIt = subfieldList.begin();
				subfieldIt != subfieldList.end();  subfieldIt++)
			{
				std::string &subfieldData = (*subfieldIt)->getData();
				std::string subfieldDataRecoded;
				if (!iconv(iconvDesc, subfieldData, subfieldDataRecoded)) {
					return false;
				}
				subfieldData = subfieldDataRecoded;
			}
		}
	}

	return true;
}

/*
 * Convert encoding for std::string.
 */
bool iconv(iconv_t iconvDesc, const std::string &src, std::string &dest)
{
	char buf[4096];
	char *p = (char *) src.c_str();
	size_t srcLen = src.size();

	dest = "";
	while (srcLen > 0) {
		size_t destLen = sizeof(buf);
		char *q = buf;
		if (iconv(iconvDesc, &p, &srcLen, &q, &destLen) == (size_t) -1) {
			if (errno != E2BIG) {
				return false;
			}
		}

		dest.append(buf, sizeof(buf) - destLen);
	}

	return true;
}

/*
 * Parse format string.
 */
bool parseFormatString(const char *formatString, RecordFormat *format, const char **encoding)
{
	/* Check presence of format string. */
	if (!formatString || strlen(formatString) == 0) {
		return false;
	}

	/* Convert format name to format code. */
	const char *arg = formatString;
	if (strcmp(arg, "iso2709") == 0 || strncmp(arg, "iso2709,", 8) == 0) {
		*format = ISO2709;
	} else if (strcmp(arg, "marcxml") == 0 || strncmp(arg, "marcxml,", 8) == 0) {
		*format = MARCXML;
	} else if (strcmp(arg, "text") == 0 || strncmp(arg, "text,", 5) == 0) {
		*format = TEXT;
	} else {
		return false;
	}

	/* Get encoding. */
	arg = strchr(formatString, ',');
	if (arg != NULL) {
		*encoding = arg + 1;
	}
	
	return true;
}

/*
 * Print help information.
 */
void print_help(void)
{
	int i;
	const char *help[] = {
		"marc-convert 1.0 (6 May 2012)\n",
		"Convert MARC records between different formats and encodings.\n",
		"Copyright (C) 2012  Alexander Fronkin\n",
		"\n",
		"usage: marc-convert [-hv]\n",
		"                    [-f <format>[,<encoding>]] [-t <format>[,<encoding>]]\n",
		"                    [-s <number of records>] [-n <number of records>]\n",
		"                    [-o <output file>] [<input file>]\n",
		"\n",
		"  -h --help      give this help\n",
		"  -f --from      format and encoding of records in input file\n",
		"                 formats: 'iso2709' (default), 'marcxml'\n",
		"  -n --numrecs   number of records to convert\n",
		"  -o --output    name of output file ('-' for stdout)\n",
		"  -s --skiprecs  number of records to skip\n",
		"  -t --to        format and encoding of records in output file\n",
		"                 formats: 'iso2709', 'marcxml', 'text' (default)\n",
		"  -v --verbose   increase verbosity level (can be specified multiple times)\n",
		"  <input file>   name of input file ('-' for stdin)\n",
		"\n",
		NULL};

	/* Printing help information this way due to theoretic
	   limit of string length in c89-compliant compilers. */
	for (i = 0; help[i] != NULL; i++) {
		fputs(help[i], stderr);
	}
}

/*
 * Main function.
 */
int main(int argc, char *argv[])
{
	const char *fromInfo = NULL, *toInfo = NULL;

	/* Parse arguments. */
	for (int argNo = 1; argNo < argc; argNo++) {
		if (argv[argNo][0] == '-' && argv[argNo][1] != '-') {
			if (strchr(argv[argNo], 'h') != NULL) {
				print_help();
				return 1;
			}

			for (char *p = argv[argNo] + 1; *p; p++) {
				switch (*p) {
				case 'f':
					if (argNo + 1 >= argc) {
						fprintf(stderr, "Error: records format "
							"must be specified.\n");
						return 1;
					}
					fromInfo = argv[++argNo];
					p = (char *) "-";
					break;
				case 'n':
					if (argNo + 1 >= argc) {
						fprintf(stderr, "Error: number of records to "
							"convert must be specified.\n");
						return 1;
					}
					options.numRecs = atol(argv[++argNo]);
					p = (char *) "-";
					break;
				case 'o':
					if (argNo + 1 >= argc) {
						fprintf(stderr,
							"Error: output file must be specified.\n");
						return 1;
					}
					options.outputFileName = argv[++argNo];
					p = (char *) "-";
					break;
				case 's':
					if (argNo + 1 >= argc) {
						fprintf(stderr, "Error: number of records to skip "
							"must be specified.\n");
						return 1;
					}
					options.skipRecs = atol(argv[++argNo]);
					p = (char *) "-";
					break;
				case 't':
					if (argNo + 1 >= argc) {
						fprintf(stderr, "Error: records format "
							"must be specified.\n");
						return 1;
					}
					toInfo = argv[++argNo];
					p = (char *) "-";
					break;
				case 'v':
					options.verboseLevel++;
					break;
				default:
					fprintf(stderr, "Error: wrong argument \"-%c\".\n\n", *p);
					return 1;
				}
			}
		} else if (strcmp(argv[argNo], "--help") == 0) {
			print_help();
			return 1;
		} else if (strncmp(argv[argNo], "--from", 6) == 0) {
			if (strchr(argv[argNo], '=') == NULL) {
				fprintf(stderr, "Error: records format must be specified.\n");
				return 1;
			}
			fromInfo = strchr(argv[argNo], '=') + 1;
		} else if (strncmp(argv[argNo], "--numrecs", 9) == 0) {
			if (strchr(argv[argNo], '=') == NULL) {
				fprintf(stderr,
					"Error: number of records to convert must be specified.\n");
				return 1;
			}
			options.numRecs = atol(strchr(argv[argNo], '=') + 1);
		} else if (strncmp(argv[argNo], "--output", 8) == 0) {
			if (strchr(argv[argNo], '=') == NULL) {
				fprintf(stderr, "Error: output file must be specified.\n");
				return 1;
			}
			options.outputFileName = strchr(argv[argNo], '=') + 1;
		} else if (strncmp(argv[argNo], "--skiprecs", 10) == 0) {
			if (strchr(argv[argNo], '=') == NULL) {
				fprintf(stderr,
					"Error: number of records to skip must be specified.\n");
				return 1;
			}
			options.skipRecs = atol(strchr(argv[argNo], '=') + 1);
		} else if (strncmp(argv[argNo], "--to", 4) == 0) {
			if (strchr(argv[argNo], '=') == NULL) {
				fprintf(stderr, "Error: records format must be specified.\n");
				return 1;
			}
			toInfo = strchr(argv[argNo], '=') + 1;
		} else if (strcmp(argv[argNo], "--verbose") == 0) {
			options.verboseLevel++;
		} else if (options.inputFileName == NULL) {
			options.inputFileName = argv[argNo];
		} else {
			fprintf(stderr, "Error: wrong argument \"%s\".\n\n", argv[argNo]);
			return 1;
		}
	}

	/* Parse input format and encoding. */
	if (fromInfo
		&& !parseFormatString(fromInfo, &options.inputFormat, &options.inputEncoding))
	{
		fprintf(stderr, "Error: wrong format string '%s' specified.\n", fromInfo);
		return 1;
	}
	
	/* Parse output format and encoding. */
	if (toInfo
		&& !parseFormatString(toInfo, &options.outputFormat, &options.outputEncoding))
	{
		fprintf(stderr, "Error: wrong format string '%s' specified.\n", toInfo);
		return 1;
	}

	/* Convert file. */
	if(!convertFile()) {
		fprintf(stderr, "Operation failed.\n");
		return 1;
	}

	return 0;
}

