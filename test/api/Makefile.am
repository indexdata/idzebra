
noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c 

simpletests = t0 t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11 t12 t13 t14 t15 t16 t17
safaritests = test_safari
check_PROGRAMS = $(simpletests) $(safaritests)
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg zebra8.cfg zebra10.cfg zebra15.cfg safari.cfg \
	t10.att t10.abs zebra17.cfg t17.idx 

noinst_LIBRARIES = libtestlib.a

libtestlib_a_SOURCES = testlib.c testlib.h

t0_SOURCES = t0.c
t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c
t4_SOURCES = t4.c
t5_SOURCES = t5.c
t6_SOURCES = t6.c
t7_SOURCES = t7.c
t8_SOURCES = t8.c
t9_SOURCES = t9.c rankingrecords.h
t10_SOURCES = t10.c
t11_SOURCES = t11.c
t12_SOURCES = t12.c
t13_SOURCES = t13.c
t14_SOURCES = t14.c
t15_SOURCES = t15.c
t16_SOURCES = t16.c
t17_SOURCES = t17.c

test_safari_SOURCES = test_safari.c testlib.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

zebralibs = ../../$(main_zebralib)

LDADD = libtestlib.a $(zebralibs) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
	-rm -rf register/*.LCK 
	-rm -rf register/*.log 
	-rm -rf register/*.mf 
