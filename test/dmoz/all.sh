#!/bin/sh
# $Id: all.sh,v 1.5 2002-06-20 08:03:30 adam Exp $
while true; do
	./update.sh b
	lpr times-b.ps
	./update.sh c
	lpr times-c.ps
	./update.sh d
	lpr times-d.ps
done
