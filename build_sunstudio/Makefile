SRC_DIR=../src
OBJS=marc_convert.o marc_reader.o marc_writer.o marcrecord.o marcrecord_field.o \
  marcrecord_subfield.o marcrecord_tools.o marcxml_reader.o marcxml_writer.o  \
  xmlparse.o xmlrole.o xmltok.o
PROGRAM=marc-convert

CXX=CC
CXXFLAGS=-O2 -I$(SRC_DIR) -I$(SRC_DIR)/marcrecord -I$(SRC_DIR)/marcrecord/expat
CXXFLAGS_MARCRECORD=-D_XPG6
CC=cc
CFLAGS_EXPAT=-O2 -I$(SRC_DIR)/marcrecord/expat -DHAVE_EXPAT_CONFIG_H
LDFLAGS=
LIBS=-lm

.PHONY: all clean verify
.SUFFIXES: .cxx .c .o

all: $(PROGRAM)
	@echo $@ done.

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	@echo $@ done.

clean:
	rm -f $(PROGRAM) $(OBJS) *.txt *.xml
	@echo $@ done.

verify:
	./$(PROGRAM) -vv -o test1.txt ../share/test/rusmarc.iso
	./$(PROGRAM) -vv -f iso2709,cp1251 -t marcxml,utf-8 -o test2.xml ../share/test/rusmarc.iso
	./$(PROGRAM) -vv -f marcxml,utf-8 -t text,cp1251 -o test3.txt test2.xml
	@echo $@ done.

%.o: $(SRC_DIR)/%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: $(SRC_DIR)/marcrecord/%.cxx
	$(CXX) $(CXXFLAGS_MARCRECORD) -c $< -o $@

%.o: $(SRC_DIR)/marcrecord/expat/%.c
	$(CC) $(CFLAGS_EXPAT) -c $< -o $@
