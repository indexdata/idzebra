EXTRA_DIST = zebra.cfg Zthes.abs zthes.att tree2xml.pl dino.tree records/dino.xml

# optional: tagsetg-zthes.tag tagsetm-zthes.tag zthes.tag 
dino: reci-0.mf

reci-0.mf: records/dino.xml
	../../index/zebraidx init
	../../index/zebraidx update records

clean:
	rm -f *.mf *.LCK zebrasrv.pid
