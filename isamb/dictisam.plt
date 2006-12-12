# $Id: dictisam.plt,v 1.1 2006-12-12 13:54:33 adam Exp $
# Plot isam + isamb times from output of benchindex1
set terminal x11
#set terminal postscript eps
#set output "dictisam.eps"
set xlabel "Run"
set ylabel "time (seconds)"
plot "4.jfs.dat" title "jfs 4" with lines, \
     "4.jfs.dat" using 1:3 title  "jfs 4 dict" with lines, \
     "4.jfs.dat" using 1:6 title  "jfs 4 isam" with lines
pause 60
