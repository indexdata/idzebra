#!/bin/sh
while true; do
	./update.sh b
	lpr times-b.ps
	./update.sh c
	lpr times-c.ps
	./update.sh d
	lpr times-d.ps
done
