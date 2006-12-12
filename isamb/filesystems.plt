# $Id: filesystems.plt,v 1.1 2006-12-12 13:54:33 adam Exp $
# Inspect benchindex1 times with different filesystems
set terminal x11
# set terminal postscript eps
# set output "filesystems.eps"
set xlabel "Run"
set ylabel "time (seconds)"
plot "ext3.dat" title "ext3" smooth acsplines,\
     "jfs.dat" title "jfs" smooth acsplines, \
     "reiser.dat" title "reiser" smooth acsplines

pause 60
