:
echo Loading Records
if [ -x ../../bin/zmbolidx ]; then
	../../bin/zmbolidx update records
fi
if [ -x ../../bin/zebraidx ]; then
	../../bin/zebraidx update records
fi
echo Starting Server
if [ -x ../../bin/zmbolsrv ]; then
	../../bin/zmbolsrv
fi
if [ -x ../../bin/zebrasrv ]; then
	../../bin/zebrasrv
fi
