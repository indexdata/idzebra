:
echo Loading Records
if [ -x ../../index/zmbolidx ]; then
	../../index/zmbolidx -t grs.sgml update records
fi
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx -t grs.sgml update records
fi
echo Starting Server
if [ -x ../../index/zmbolsrv ]; then
	../../index/zmbolsrv
fi
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
