SRC_DIR=../src
OBJS=marc_convert.o marc_reader.o marc_writer.o marcrecord.o marcrecord_field.o \
  marcrecord_subfield.o marcrecord_tools.o marctext_writer.o marcxml_reader.o marcxml_writer.o
PROGRAM=marc-convert

CXX=g++
CXXFLAGS=-O2 -W -Wall -Wextra -ansi -pedantic -Wpointer-arith -Wwrite-strings -Wno-long-long \
  -I$(SRC_DIR) -I$(SRC_DIR)/marcrecord
CXXFLAGS_MARCRECORD=$(CXXFLAGS)
LDFLAGS=
LIBS=-lm -lexpat -liconv

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
	./$(PROGRAM) -vv -f iso2709 -t marcxml -e cp1251 -r utf-8 -o test2.xml ../share/test/rusmarc.iso
	./$(PROGRAM) -vv -f marcxml -t text -r cp1251 -o test3.txt test2.xml
	./$(PROGRAM) -vv -f iso2709 -t marcxml -e windows-1251 -o test4.xml ../share/test/rusmarc.iso
	@echo $@ done.

%.o: $(SRC_DIR)/%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: $(SRC_DIR)/marcrecord/%.cxx
	$(CXX) $(CXXFLAGS_MARCRECORD) -c $< -o $@
