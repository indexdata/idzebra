set terminal postscript eps
set output "bench1.eps"
set xlabel "R"
set ylabel "time"
plot "bench1.1000.dat" title "isam 1000" with linespoints, \
     "bench1.100.dat" title "isam 100" with linespoints, \
     "bench1.10.dat" title "isam 10" with linespoints, \
     "bench1.1.dat" title "isam 1" with linespoints
