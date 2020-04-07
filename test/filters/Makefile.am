AUTOMAKE_OPTIONS = serial-tests

check_PROGRAMS = text grs.xml grs.xml.idzebra grs.marc

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg record.xml record-idzebra.xml record.mrc

text_SOURCES = text.c
grs_xml_SOURCES = grs.xml.c
grs_xml_idzebra_SOURCES = grs.xml.idzebra.c
grs_marc_SOURCES = grs.marc.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZ_CFLAGS)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZ_LIBS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

