SOURCEFILES = records/genera.xml records/taxa.xml
OBJECECTFILES = *.mf *.LCK zebrasrv.pid zebraidx.time

isamcA-0.mf: $(SOURCEFILES)
	../../index/zebraidx -t grs.sgml update records

clean:
	rm -f $(OBJECECTFILES)
