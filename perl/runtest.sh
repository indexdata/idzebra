#!/bin/sh
PERL_DL_NONLAZY=1 /usr/bin/perl "-MExtUtils::Command::MM" "-e" "test_harness(0,'blib/lib', 'blib/arch')" $*
