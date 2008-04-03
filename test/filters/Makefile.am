
check_PROGRAMS = text grs.xml grs.xml.idzebra grs.marc

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg record.xml record-idzebra.xml record.mrc

text_SOURCES = text.c
grs_xml_SOURCES = grs.xml.c
grs_xml_idzebra_SOURCES = grs.xml.idzebra.c
grs_marc_SOURCES = grs.marc.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

