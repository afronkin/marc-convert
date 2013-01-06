SRC_DIR=..\src
OBJS = marc_reader.obj marc_writer.obj marcrecord.obj marcrecord_field.obj \
  marcrecord_subfield.obj marcrecord_tools.obj marctext_writer.obj \
  marcxml_reader.obj marcxml_writer.obj xmlparse.obj xmlrole.obj xmltok.obj \
  marc_convert.obj
PROGRAM = marc-convert.exe

PLATFORM_SDK=C:\Program Files\Microsoft SDKs\Windows\v7.1
LIBICONV=D:\projects_lib\libiconv

CXXFLAGS=/nologo /MT /O2 /EHsc /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
  /FD /I "$(PLATFORM_SDK)\Include" /I "..\src" /I "$(LIBICONV)\include" \
  /D "XML_STATIC"
CFLAGS_EXPAT=/nologo /MT /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
  /FD /I "$(PLATFORM_SDK)\Include" /I "..\src\expat" /I "$(LIBICONV)\include" \
  /D "HAVE_EXPAT_CONFIG_H"
LDFLAGS=/nologo /machine:I386
LIBS=$(LIBICONV)\lib\libiconv.lib

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	cl $** /link $(LDFLAGS) /out:$@ $(LIBS)

clean:
	del *.exe *.idb *.obj *.iso *.txt *.xml

verify:
	$(PROGRAM)

marc_reader.obj: $(SRC_DIR)\marcrecord\marc_reader.cxx
	cl /c $(CXXFLAGS) $**

marc_writer.obj: $(SRC_DIR)\marcrecord\marc_writer.cxx
	cl /c $(CXXFLAGS) $**

marcrecord.obj: $(SRC_DIR)\marcrecord\marcrecord.cxx
	cl /c $(CXXFLAGS) $**

marcrecord_field.obj: $(SRC_DIR)\marcrecord\marcrecord_field.cxx
	cl /c $(CXXFLAGS) $**

marcrecord_subfield.obj: $(SRC_DIR)\marcrecord\marcrecord_subfield.cxx
	cl /c $(CXXFLAGS) $**

marcrecord_tools.obj: $(SRC_DIR)\marcrecord\marcrecord_tools.cxx
	cl /c $(CXXFLAGS) $**

marctext_writer.obj: $(SRC_DIR)\marcrecord\marctext_writer.cxx
	cl /c $(CXXFLAGS) $**

marcxml_reader.obj: $(SRC_DIR)\marcrecord\marcxml_reader.cxx
	cl /c $(CXXFLAGS) $**

marcxml_writer.obj: $(SRC_DIR)\marcrecord\marcxml_writer.cxx
	cl /c $(CXXFLAGS) $**

xmlparse.obj: $(SRC_DIR)\expat\xmlparse.c
	cl /c $(CFLAGS_EXPAT) $**

xmlrole.obj: $(SRC_DIR)\expat\xmlrole.c
	cl /c $(CFLAGS_EXPAT) $**

xmltok.obj: $(SRC_DIR)\expat\xmltok.c
	cl /c $(CFLAGS_EXPAT) $**

marc_convert.obj: $(SRC_DIR)\marc_convert.cxx
	cl /c $(CXXFLAGS) $**
