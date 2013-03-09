SRC_DIR=..\src
SRC_DIR_EXPAT=$(SRC_DIR)\expat
SRC_DIR_GETOPT=$(SRC_DIR)\getopt
SRC_DIR_MARCRECORD=$(SRC_DIR)\marcrecord
SRC_DIR_WIN_ICONV=$(SRC_DIR)\win-iconv

OBJS_EXPAT=xmlparse.obj xmlrole.obj xmltok.obj
OBJS_GETOPT=getopt_long.obj
OBJS_MARCRECORD=marc_reader.obj marc_writer.obj marcrecord.obj marcrecord_field.obj \
  marcrecord_subfield.obj marcrecord_tools.obj marctext_writer.obj \
  marcxml_reader.obj marcxml_writer.obj
OBJS_WIN_ICONV=win_iconv.obj
OBJS=marc_convert.obj $(OBJS_EXPAT) $(OBJS_GETOPT) $(OBJS_MARCRECORD) $(OBJS_WIN_ICONV)

PROGRAM=marc-convert.exe

PLATFORM_SDK=C:\Program Files\Microsoft SDKs\Windows\v7.1

CXXFLAGS=/nologo /MT /O2 /EHsc /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
  /FD /I "$(PLATFORM_SDK)\Include"
CXXFLAGS_PROGRAM=$(CXXFLAGS) /I "$(SRC_DIR)" /I "$(SRC_DIR_EXPAT)" \
  /I $(SRC_DIR_GETOPT) /I "$(SRC_DIR_WIN_ICONV)" /D "XML_STATIC"
CXXFLAGS_MARCRECORD=$(CXXFLAGS) /I "$(SRC_DIR_MARCRECORD)" /I "$(SRC_DIR_EXPAT)" \
  /I "$(SRC_DIR_WIN_ICONV)" /D "XML_STATIC"

CFLAGS=/nologo /MT /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
  /FD /I "$(PLATFORM_SDK)\Include"
CFLAGS_EXPAT=$(CFLAGS) /I "$(SRC_DIR_EXPAT)" /D "HAVE_EXPAT_CONFIG_H" /D "XML_STATIC"
CFLAGS_GETOPT=$(CFLAGS) /I $(SRC_DIR_GETOPT)

LDFLAGS=/nologo /machine:I386
LIBS=

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	link $** $(LIBS) $(LDFLAGS) /out:$@

clean:
	del *.exe *.idb *.obj *.iso *.txt *.xml

verify:
	$(PROGRAM)

marc_convert.obj: $(SRC_DIR)\marc_convert.cxx
	cl /c $(CXXFLAGS_PROGRAM) $**

xmlparse.obj: $(SRC_DIR_EXPAT)\xmlparse.c
	cl /c $(CFLAGS_EXPAT) $**

xmlrole.obj: $(SRC_DIR_EXPAT)\xmlrole.c
	cl /c $(CFLAGS_EXPAT) $**

xmltok.obj: $(SRC_DIR_EXPAT)\xmltok.c
	cl /c $(CFLAGS_EXPAT) $**

getopt_long.obj: $(SRC_DIR_GETOPT)\getopt_long.c
        cl /c $(CFLAGS_GETOPT) $**

marc_reader.obj: $(SRC_DIR_MARCRECORD)\marc_reader.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marc_writer.obj: $(SRC_DIR_MARCRECORD)\marc_writer.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcrecord.obj: $(SRC_DIR_MARCRECORD)\marcrecord.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcrecord_field.obj: $(SRC_DIR_MARCRECORD)\marcrecord_field.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcrecord_subfield.obj: $(SRC_DIR_MARCRECORD)\marcrecord_subfield.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcrecord_tools.obj: $(SRC_DIR_MARCRECORD)\marcrecord_tools.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marctext_writer.obj: $(SRC_DIR_MARCRECORD)\marctext_writer.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcxml_reader.obj: $(SRC_DIR_MARCRECORD)\marcxml_reader.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

marcxml_writer.obj: $(SRC_DIR_MARCRECORD)\marcxml_writer.cxx
	cl /c $(CXXFLAGS_MARCRECORD) $**

win_iconv.obj: $(SRC_DIR_WIN_ICONV)\win_iconv.c
	cl /c $(CFLAGS_WIN_ICONV) $**
