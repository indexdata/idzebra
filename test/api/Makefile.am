
noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c 

check_PROGRAMS = test_start_stop test_result_sets \
	test_insert_fetch test_search test_trunc test_sort_set \
	test_rank test_private_attset \
	test_scan test_create_databases test_resources test_update_record \
	test_zebra_fork test_special_elements test_icu_indexing \
	test_safari test_sort1 test_sort2 test_sort3 \
	test_sortidx

TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg test_trunc.cfg test_private_attset.cfg \
	private_attset.att private_attset.abs test_search.abs \
	test_zebra_fork.cfg \
	test_icu_indexing.cfg test_icu_indexing.idx \
	test_sort1.cfg test_sort1.idx test_sort1.chr sort1.abs \
	test_sort2.cfg test_sort2.idx test_sort2.chr sort2.abs \
	test_safari.cfg test_sort3.cfg string.chr

noinst_LIBRARIES = libtestlib.a

libtestlib_a_SOURCES = testlib.c testlib.h

test_start_stop_SOURCES = test_start_stop.c
test_result_sets_SOURCES = test_result_sets.c
test_insert_fetch_SOURCES = test_insert_fetch.c
test_search_SOURCES = test_search.c
test_trunc_SOURCES = test_trunc.c
test_sort_set_SOURCES = test_sort_set.c
test_rank_SOURCES = test_rank.c 
test_private_attset_SOURCES = test_private_attset.c
test_scan_SOURCES = test_scan.c
test_create_databases_SOURCES = test_create_databases.c
test_resources_SOURCES = test_resources.c
test_update_record_SOURCES = test_update_record.c
test_zebra_fork_SOURCES = test_zebra_fork.c
test_special_elements_SOURCES = test_special_elements.c
test_icu_indexing_SOURCES = test_icu_indexing.c
test_safari_SOURCES = test_safari.c 
test_sort1_SOURCES = test_sort1.c
test_sort2_SOURCES = test_sort2.c
test_sort3_SOURCES = test_sort3.c
test_sortidx_SOURCES = test_sortidx.c

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
