EXTRA_DIST = zebra.cfg Zthes.abs zthes.att tree2xml.pl dino.tree

# optional: tagsetg-zthes.tag tagsetm-zthes.tag zthes.tag 
dino: reci-0.mf

reci-0.mf: records/dino.xml
	../../index/zebraidx init
	../../index/zebraidx update records

records/dino.xml: tree2xml.pl dino.tree
	./tree2xml.pl dino.tree > records/dino.xml

clean:
	rm -f records/dino.xml *.mf *.LCK zebrasrv.pid

dist-hook:
	-mkdir $(distdir)/records
