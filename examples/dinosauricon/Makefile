DBFILES = *.mf *.LCK zebrasrv.pid

isamcA-0.mf: records/genera.xml
	zebraidx -t grs.sgml update records

clean:
	rm -f $(DBFILES)
