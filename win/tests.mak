# Keep the program list in step with check_PROGRAMS in the Automake files.
# Mirror source directories so tests with the same name have distinct outputs.
TEST_DIRS = \
    $(TESTBINDIR)\util \
    $(TESTBINDIR)\bfile \
    $(TESTBINDIR)\dfa \
    $(TESTBINDIR)\dict \
    $(TESTBINDIR)\isamb \
    $(TESTBINDIR)\test\codec \
    $(TESTBINDIR)\test\api \
    $(TESTBINDIR)\test\filters \
    $(TESTBINDIR)\test\xslt \
    $(TESTBINDIR)\test\xpath \
    $(TESTBINDIR)\test\rusmarc \
    $(TESTBINDIR)\test\marcxml \
    $(TESTBINDIR)\test\charmap \
    $(TESTBINDIR)\test\mbox \
    $(TESTBINDIR)\test\espec

TEST_PROGRAMS = \
    $(TESTBINDIR)\util\tstcharmap.exe \
    $(TESTBINDIR)\util\tstflock.exe \
    $(TESTBINDIR)\util\tstlockscope.exe \
    $(TESTBINDIR)\util\tstpass.exe \
    $(TESTBINDIR)\util\tstres.exe \
    $(TESTBINDIR)\util\test_strmap.exe \
    $(TESTBINDIR)\bfile\tstmfile1.exe \
    $(TESTBINDIR)\bfile\tstbfile2.exe \
    $(TESTBINDIR)\dfa\test_dfa.exe \
    $(TESTBINDIR)\dict\scantest.exe \
    $(TESTBINDIR)\dict\inserttest.exe \
    $(TESTBINDIR)\isamb\tstisamb.exe \
    $(TESTBINDIR)\test\codec\tstcodec.exe \
    $(TESTBINDIR)\test\api\test_start_stop.exe \
    $(TESTBINDIR)\test\api\test_result_sets.exe \
    $(TESTBINDIR)\test\api\test_insert_fetch.exe \
    $(TESTBINDIR)\test\api\test_search.exe \
    $(TESTBINDIR)\test\api\test_trunc.exe \
    $(TESTBINDIR)\test\api\test_sort_set.exe \
    $(TESTBINDIR)\test\api\test_rank.exe \
    $(TESTBINDIR)\test\api\test_private_attset.exe \
    $(TESTBINDIR)\test\api\test_scan.exe \
    $(TESTBINDIR)\test\api\test_create_databases.exe \
    $(TESTBINDIR)\test\api\test_resources.exe \
    $(TESTBINDIR)\test\api\test_update_record.exe \
    $(TESTBINDIR)\test\api\test_zebra_fork.exe \
    $(TESTBINDIR)\test\api\test_special_elements.exe \
    $(TESTBINDIR)\test\api\test_icu_indexing.exe \
    $(TESTBINDIR)\test\api\test_safari.exe \
    $(TESTBINDIR)\test\api\test_sort1.exe \
    $(TESTBINDIR)\test\api\test_sort2.exe \
    $(TESTBINDIR)\test\api\test_sort3.exe \
    $(TESTBINDIR)\test\api\test_sortidx.exe \
    $(TESTBINDIR)\test\filters\text.exe \
    $(TESTBINDIR)\test\filters\grs.xml.exe \
    $(TESTBINDIR)\test\filters\grs.xml.idzebra.exe \
    $(TESTBINDIR)\test\filters\grs.marc.exe \
    $(TESTBINDIR)\test\xslt\xslt1.exe \
    $(TESTBINDIR)\test\xslt\xslt2.exe \
    $(TESTBINDIR)\test\xslt\xslt3.exe \
    $(TESTBINDIR)\test\xslt\xslt4.exe \
    $(TESTBINDIR)\test\xslt\xslt5.exe \
    $(TESTBINDIR)\test\xslt\dom1.exe \
    $(TESTBINDIR)\test\xpath\xpath1.exe \
    $(TESTBINDIR)\test\xpath\xpath2.exe \
    $(TESTBINDIR)\test\xpath\xpath3.exe \
    $(TESTBINDIR)\test\xpath\xpath4.exe \
    $(TESTBINDIR)\test\xpath\xpath5.exe \
    $(TESTBINDIR)\test\xpath\xpath6.exe \
    $(TESTBINDIR)\test\rusmarc\t1.exe \
    $(TESTBINDIR)\test\marcxml\t1.exe \
    $(TESTBINDIR)\test\marcxml\t2.exe \
    $(TESTBINDIR)\test\charmap\charmap1.exe \
    $(TESTBINDIR)\test\mbox\mbox1.exe \
    $(TESTBINDIR)\test\espec\t1.exe

# Archive the Zebra objects so tests can exercise private APIs.
TEST_LIB = $(OBJDIR)\zebra_test.lib
TEST_SUPPORT = $(OBJDIR)\zebra_testlib.obj
TEST_MAKEFILE = $(WINDIR)\tests.mak
COMPILE_TEST = $(CPP) $(COPT) /Fo"$(@R).obj" $<
LINK_TEST = $(LINK) $(LNKOPT) $(CLIENT_LINK_OPTIONS) "$(TEST_LIB)" $(YAZLIB) $(LINK_LIBS) "$(@R).obj" /out:$@

check: all testprog
	$(TCL) $(WINDIR)\run-tests.tcl "$(ROOTDIR)" "$(BINDIR)" "$(TESTBINDIR)" $(TEST_PROGRAMS)

test: check

testprog: dirs generate $(TEST_DIRS) $(TEST_PROGRAMS)

$(TEST_DIRS):
	if not exist "$@\$(NUL)" mkdir "$@"

$(TEST_SUPPORT): $(ROOTDIR)\test\api\testlib.c $(ROOTDIR)\test\api\testlib.h
	$(CPP) $(COPT) /Fo"$@" $(ROOTDIR)\test\api\testlib.c

$(TEST_LIB): $(ZEBRALIB_OBJS) $(TEST_SUPPORT) $(TEST_MAKEFILE)
	lib.exe /nologo /out:$@ @<<
$(ZEBRALIB_OBJS) $(TEST_SUPPORT)
<<

$(TEST_PROGRAMS): $(TEST_LIB) $(TEST_MAKEFILE)

# Compile directly from C to avoid chaining inference rules on a clean tree.
{$(ROOTDIR)\util}.c{$(TESTBINDIR)\util}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\bfile}.c{$(TESTBINDIR)\bfile}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\dfa}.c{$(TESTBINDIR)\dfa}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\dict}.c{$(TESTBINDIR)\dict}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\isamb}.c{$(TESTBINDIR)\isamb}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\codec}.c{$(TESTBINDIR)\test\codec}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\api}.c{$(TESTBINDIR)\test\api}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\filters}.c{$(TESTBINDIR)\test\filters}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\xslt}.c{$(TESTBINDIR)\test\xslt}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\xpath}.c{$(TESTBINDIR)\test\xpath}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\rusmarc}.c{$(TESTBINDIR)\test\rusmarc}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\marcxml}.c{$(TESTBINDIR)\test\marcxml}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\charmap}.c{$(TESTBINDIR)\test\charmap}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\mbox}.c{$(TESTBINDIR)\test\mbox}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)

{$(ROOTDIR)\test\espec}.c{$(TESTBINDIR)\test\espec}.exe:
	$(COMPILE_TEST)
	$(LINK_TEST)
