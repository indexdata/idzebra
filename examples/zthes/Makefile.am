EXTRA_DIST = zebra.cfg Zthes.abs zthes.att zthes.tag \
  tagsetg-zthes.tag tagsetm-zthes.tag tree2xml.pl dino.tree

dist-hook:
	-mkdir $(distdir)/records

recdA-0.mf: records/dino.xml
	zebraidx init
	zebraidx update records

records/dino.xml: tree2xml.pl dino.tree
	./tree2xml.pl dino.tree > records/dino.xml

clean:
	rm -f records/dino.xml *.mf *.LCK zebrasrv.pid

