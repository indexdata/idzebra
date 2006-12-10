set terminal postscript eps
set output "bench2.eps"
set xlabel "R"
set ylabel "time"
plot "bench2.0.dat" title "0" with linespoints, \
     "bench2.4.dat" title "4" with linespoints, \
     "bench2.8.dat" title "8" with linespoints, \
     "bench2.12.dat" title "12" with linespoints
