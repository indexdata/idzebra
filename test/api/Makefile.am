
noinst_PROGRAMS = t1 t2

t1_SOURCES = t1.c
t2_SOURCES = t2.c

CFLAGS = -I$(top_srcdir)/index $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)
