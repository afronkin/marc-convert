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
#include <ctime>
extern "C" {
#include <getopt.h>
}
#include <math.h>
#include "marcrecord/marcrecord.h"
#include "marcrecord/marc_reader.h"
#include "marcrecord/marc_writer.h"
#include "marcrecord/marctext_writer.h"
#include "marcrecord/marcxml_reader.h"
#include "marcrecord/marcxml_writer.h"

using namespace marcrecord;

// Record format variants.
enum RecordFormat { FORMAT_NULL, FORMAT_ISO2709, FORMAT_MARCXML, FORMAT_TEXT };

// Application options structure.
struct Options {
	int verboseLevel;
	bool permissiveRead;
	int skipRecs;
	int numRecs;
	const char *inputFileName;
	const char *outputFileName;
	enum RecordFormat inputFormat;
	enum RecordFormat outputFormat;
	const char *inputEncoding;
	const char *outputEncoding;
};
typedef struct Options Options;

// Records counters.
struct Counters {
	int recNo;
	int numBadRecs;
	int numConvertedRecs;
};
typedef Counters Counters;

// Application options.
static Options options = {
	0, false, 0, 0, NULL, NULL,
	FORMAT_ISO2709, FORMAT_TEXT, NULL, NULL };

// Records readers.
MarcReader marcReader;
MarcXmlReader marcXmlReader;

// Records writers.
MarcWriter marcWriter;
MarcTextWriter marcTextWriter;
MarcXmlWriter marcXmlWriter;

/*
 * Convert record (read it from input file and write to output file).
 * Side effect: updates counters.
 */
static bool
convertRecord(Counters &counters)
{
	// Read record from input file.
	MarcRecord record;
	bool readStatus;

	switch (options.inputFormat) {
	case FORMAT_ISO2709:
		readStatus = marcReader.next(record);
		if (readStatus) {
			break;
		}

		switch (marcReader.getErrorCode()) {
		case MarcReader::END_OF_FILE:
			return false;
		case MarcReader::ERROR_INVALID_RECORD:
			if (options.permissiveRead) {
				counters.numBadRecs++;
				break;
			}
			throw marcReader.getErrorMessage();
		default:
			throw marcReader.getErrorMessage();
		}
		break;
	case FORMAT_MARCXML:
		readStatus = marcXmlReader.next(record);
		if (readStatus) {
			break;
		}

		switch (marcXmlReader.getErrorCode()) {
		case MarcXmlReader::END_OF_FILE:
			return false;
		default:
			throw marcXmlReader.getErrorMessage();
		}
		break;
	default:
		throw std::string("unknown input format");
	}

	// Write record to output file.
	if (readStatus && counters.recNo > options.skipRecs) {
		counters.numConvertedRecs++;
		std::string textRecord, textRecordRecoded;

		switch (options.outputFormat) {
		case FORMAT_ISO2709:
			marcWriter.write(record);
			break;
		case FORMAT_MARCXML:
			marcXmlWriter.write(record);
			break;
		case FORMAT_TEXT:
			char recordHeader[30];
			if (counters.numConvertedRecs > 1) {
				sprintf(recordHeader, "\nRecord %d\n",
					counters.recNo);
			} else {
				sprintf(recordHeader, "Record %d\n",
					counters.recNo);
			}

			marcTextWriter.write(record, recordHeader, "\n");
			break;
		default:
			throw std::string("unknown output format");
		}
	}

	return true;
}

/*
 * Convert records from MARC file.
 */
static bool
convertFile(void)
{
	FILE *inputFile = NULL, *outputFile = NULL;
	Counters counters = { 0, 0, 0 };

	try {
		// Open input file.
		if (options.inputFileName == NULL
			|| strcmp(options.inputFileName, "-") == 0)
		{
			inputFile = stdin;
		} else {
			inputFile = fopen(options.inputFileName, "rb");
			if (inputFile == NULL) {
				throw std::string("can't open input file");
			}
		}

		// Open output file.
		if (options.outputFileName == NULL
			|| strcmp(options.outputFileName, "-") == 0)
		{
			outputFile = stdout;
		} else {
			outputFile = fopen(options.outputFileName, "wb");
			if (outputFile == NULL) {
				throw std::string("can't open output file.");
			}
		}

		// Open input file in MarcReader or MarcXmlReader.
		switch (options.inputFormat) {
		case FORMAT_ISO2709:
			marcReader.open(inputFile, options.inputEncoding);
			marcReader.setAutoCorrectionMode(
				options.permissiveRead);
			break;
		case FORMAT_MARCXML:
			marcXmlReader.open(inputFile, options.inputEncoding);
			marcXmlReader.setAutoCorrectionMode(
				options.permissiveRead);
			break;
		default:
			throw std::string("wrong input format specified");
		}

		// Open output file in *Writer.
		switch (options.outputFormat) {
		case FORMAT_ISO2709:
			marcWriter.open(outputFile, options.outputEncoding);
			break;
		case FORMAT_MARCXML:
			marcXmlWriter.open(outputFile, options.outputEncoding);
			marcXmlWriter.writeHeader();
			break;
		case FORMAT_TEXT:
			marcTextWriter.open(outputFile,
				options.outputEncoding);
			break;
		default:
			throw std::string("wrong input format specified");
		}

		// Get process start time.
		time_t startTime, curTime, prevTime;
		time(&startTime);
		prevTime = startTime;

		// Convert records from input file to output file.
		for (counters.recNo = 1; options.numRecs == 0
			|| counters.numConvertedRecs < options.numRecs;
			counters.recNo++)
		{
			if (!convertRecord(counters)) {
				break;
			}

			// Print process status.
			time(&curTime);
			if (curTime > prevTime) {
				if (options.verboseLevel > 1) {
					fprintf(stderr, "\rRecord: %d",
						counters.recNo);
				}

				prevTime = curTime;
				fflush(stderr);
			}
		}

		// Write MARCXML footer to output file.
		if (options.outputFormat == FORMAT_MARCXML) {
			marcXmlWriter.writeFooter();
		}

		// Close files.
		if (inputFile != stdin) {
			fclose(inputFile);
			inputFile = NULL;
		}
		if (outputFile != stdout) {
			fclose(outputFile);
			outputFile = NULL;
		}

		// Print used time.
		if (options.verboseLevel > 0) {
			time(&curTime);
			double usedTime = (double) (curTime - startTime);
			int usedHours = (int) floor(usedTime / 3600);
			int usedMinutes = (int) floor((usedTime
				- (usedHours * 3600)) / 60);
			int usedSeconds = (int) (usedTime - (usedHours * 3600)
				- (usedMinutes * 60));

			if (options.verboseLevel > 1) {
				fputc('\r', stderr);
			}
			fprintf(stderr, "Readed records: %d\n",
				counters.recNo - 1);
			fprintf(stderr, "Converted records: %d\n",
				counters.numConvertedRecs);
			fprintf(stderr, "Records with errors: %d\n",
				counters.numBadRecs);
			fprintf(stderr, "Done in %d:%02d:%02d.\n",
				usedHours, usedMinutes, usedSeconds);
		}
	} catch (std::string errorMessage) {
		// Print error message.
		if (options.verboseLevel > 1) {
			fputc('\r', stderr);
		}
		fprintf(stderr, "Error in record %d: %s.\n",
			counters.recNo, errorMessage.c_str());

		// Close files.
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
 * Convert record format name to code.
 */
static RecordFormat
parseRecordFormat(const char *formatName)
{
	// Check presence of format name.
	if (!formatName || strlen(formatName) == 0) {
		return FORMAT_NULL;
	}

	// Convert format name to format code.
	if (strcmp(formatName, "iso2709") == 0) {
		return FORMAT_ISO2709;
	} else if (strcmp(formatName, "marcxml") == 0) {
		return FORMAT_MARCXML;
	} else if (strcmp(formatName, "text") == 0) {
		return FORMAT_TEXT;
	}

	return FORMAT_NULL;
}

/*
 * Display usage information.
 */
static void
displayUsage(void)
{
	int i;
	const char *help[] = {
		"marc-convert 1.3 (9 Mar 2013)\n",
		"Convert MARC records between different formats.\n",
		"Copyright (c) 2013, Alexander Fronkin\n",
		"\n",
		"usage: marc-convert [-hpv]\n",
		"  [-f srcfmt] [-t destfmt] [-e srcenc] [-r destenc]\n",
		"  [-s numrecs] [-n numrecs] [-o outfile] [infile]\n",
		"\n",
		"  -h --help        give this help\n",
		"  -e --encoding    encoding of input file\n",
		"                   default encoding: utf-8\n",
		"  -f --from        format of input file\n",
		"                   formats: iso2709, marcxml\n",
		"                   default format: iso2709\n",
		"  -n --numrecs     number of records to convert\n",
		"  -o --output      name of output file ('-' for stdout)\n",
		"  -p --permissive  permissive reading (skip minor errors)\n",
		"  -r --recode      encoding of output file\n",
		"  -s --skiprecs    number of records to skip\n",
		"  -t --to          format of output file\n",
		"                   formats: iso2709, marcxml, text\n",
		"                   default format: text\n",
		"  -v --verbose     increase verbosity level (repeatable)\n",
		"  infile           name of input file ('-' for stdin)\n",
		"\n",
		NULL};

	/*
	 * Printing help information this way due to theoretic
	 * limit of string length in c89-compliant compilers.
	 */
	for (i = 0; help[i] != NULL; i++) {
		fputs(help[i], stderr);
	}
}

/*
 * Parse command line arguments.
 */
static int
parseCommandLine(int argc, char **argv)
{
	static const char *short_options = "hf:e:n:o:pr:s:t:v";
	static struct option long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "encoding", required_argument, 0, 'e' },
		{ "from", required_argument, 0, 'f' },
		{ "numrecs", required_argument, 0, 'n' },
		{ "output", required_argument, 0, 'o' },
		{ "permissive", no_argument, 0, 'p' },
		{ "recode", required_argument, 0, 'r' },
		{ "skiprecs", required_argument, 0, 's' },
		{ "to", required_argument, 0, 't' },
		{ "verbose", no_argument, 0, 'v' },
		{ 0, 0, 0, 0 }
	};
	int option;

	// Parse command line arguments with getopt_long().
	while ((option = getopt_long(argc, argv, short_options,
		long_options, NULL)) != -1)
	{
		switch (option) {
		case 'h':
			displayUsage();
			return 2;
		case 'e':
			options.inputEncoding = optarg;
			break;
		case 'f':
			options.inputFormat = parseRecordFormat(optarg);
			break;
		case 'n':
			options.numRecs = atol(optarg);
			break;
		case 'o':
			options.outputFileName = optarg;
			break;
		case 'p':
			options.permissiveRead = true;
			break;
		case 'r':
			options.outputEncoding = optarg;
			break;
		case 's':
			options.skipRecs = atol(optarg);
			break;
		case 't':
			options.outputFormat = parseRecordFormat(optarg);
			break;
		case 'v':
			options.verboseLevel++;
			break;
		default:
			return 2;
		}
	}

	if (optind < argc) {
		options.inputFileName = argv[optind++];
	}

	// If only input encoding specified then use it as output encoding too.
	if (options.inputEncoding != NULL && options.outputEncoding == NULL) {
		options.outputEncoding = options.inputEncoding;
	}

	// Command line parsed successfully.
	return 0;
}

/*
 * Main function.
 */
int
main(int argc, char **argv)
{
	int result_code;

	// Parse command line arguments.
	result_code = parseCommandLine(argc, argv);
	if (result_code != 0) {
		return result_code;
	}

	// Convert file.
	if(!convertFile()) {
		fprintf(stderr, "Operation failed.\n");
		return 1;
	}

	return 0;
}
