

EXTRA_PROGRAMS = t1 t2 t3

EXTRA_DIST=t1.cfg t2.cfg

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)
