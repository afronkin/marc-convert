SRC_DIR=../../src
SRC_DIR_MARC_CONVERT=$(SRC_DIR)
SRC_DIR_MARCRECORD=$(SRC_DIR)/marcrecord

OBJS_DIR=objs
OBJS_DIR_MARC_CONVERT=$(OBJS_DIR)
OBJS_DIR_MARCRECORD=$(OBJS_DIR)/marcrecord

OBJS_MARC_CONVERT=\
  $(OBJS_DIR_MARC_CONVERT)/marc_convert.o
OBJS_MARCRECORD=\
  $(OBJS_DIR_MARCRECORD)/marc_reader.o \
  $(OBJS_DIR_MARCRECORD)/marc_writer.o \
  $(OBJS_DIR_MARCRECORD)/marciso_reader.o \
  $(OBJS_DIR_MARCRECORD)/marciso_writer.o \
  $(OBJS_DIR_MARCRECORD)/marcrecord.o \
  $(OBJS_DIR_MARCRECORD)/marcrecord_field.o \
  $(OBJS_DIR_MARCRECORD)/marcrecord_subfield.o \
  $(OBJS_DIR_MARCRECORD)/marcrecord_tools.o \
  $(OBJS_DIR_MARCRECORD)/marctext_writer.o \
  $(OBJS_DIR_MARCRECORD)/marcxml_reader.o \
  $(OBJS_DIR_MARCRECORD)/marcxml_writer.o \
  $(OBJS_DIR_MARCRECORD)/unimarcxml_writer.o

BIN_DIR=bin
BIN_MARC_CONVERT=$(BIN_DIR)/marc-convert

CXX=g++
CXXFLAGS=-O2 -W -Wall -Wextra -ansi -pedantic -Wpointer-arith -Wwrite-strings -Wno-long-long
CXXFLAGS_MARC_CONVERT=$(CXXFLAGS) -I$(SRC_DIR_MARC_CONVERT) -I$(SRC_DIR_MARCRECORD)
CXXFLAGS_MARCRECORD=$(CXXFLAGS) -I$(SRC_DIR_MARCRECORD)

LINK=g++
LDFLAGS=
LIBS=-lm -lexpat -liconv

.PHONY: all clean verify
.SUFFIXES: .cxx .c .o

all: depend $(BIN_MARC_CONVERT)

clean:
	$(RM) -R $(BIN_DIR) $(OBJS_DIR) *.txt *.xml

depend:
	mkdir -p $(BIN_DIR) $(OBJS_DIR) $(OBJS_DIR_MARC_CONVERT) $(OBJS_DIR_MARCRECORD)

verify:
	./$(BIN_MARC_CONVERT) -vv -o test1.txt ../../share/test/rusmarc.iso
	./$(BIN_MARC_CONVERT) -vv -f iso2709 -t marcxml -e cp1251 -r utf-8 -o test2.xml ../../share/test/rusmarc.iso
	./$(BIN_MARC_CONVERT) -vv -f marcxml -t text -r cp1251 -o test3.txt test2.xml
	./$(BIN_MARC_CONVERT) -vv -f iso2709 -t marcxml -e windows-1251 -o test4.xml ../../share/test/rusmarc.iso

$(BIN_MARC_CONVERT): | $(BIN_DIR)

$(BIN_DIR):
	mkdir -p $@

$(OBJS_MARC_CONVERT): | $(OBJS_DIR_MARC_CONVERT)

$(OBJS_DIR_MARC_CONVERT):
	mkdir -p $@

$(OBJS_MARCRECORD): | $(OBJS_DIR_MARCRECORD)

$(OBJS_DIR_MARCRECORD):
	mkdir -p $@

$(BIN_MARC_CONVERT): $(OBJS_MARC_CONVERT) $(OBJS_MARCRECORD)
	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJS_DIR_MARC_CONVERT)/%.o: $(SRC_DIR_MARC_CONVERT)/%.cxx
	$(CXX) $(CXXFLAGS_MARC_CONVERT) -c -o $@ $<

$(OBJS_DIR_MARCRECORD)/%.o: $(SRC_DIR_MARCRECORD)/%.cxx
	$(CXX) $(CXXFLAGS_MARCRECORD) -c $< -o $@
