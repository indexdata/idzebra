set terminal postscript eps
set output "bench1.eps"
set xlabel "R"
set ylabel "time"
plot "1000x1000.dat" title "isam 1000" with linespoints, \
     "100x10000.dat" title "isam 100" with linespoints, \
     "10x100000.dat" title "isam 10" with linespoints, \
     "1x1000000.dat" title "isam 1" with linespoints
