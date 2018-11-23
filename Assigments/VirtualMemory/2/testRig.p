set terminal png
set output "testRig.png"

set title "TLB benchmark, 4 KiByte pages, 10 Gi operations"

set key right center

set xlabel "number of pages"

set ylabel "time in s"

#use log scale if we use doubling of number of pages
set logscale x 2

plot "testRig.dat" u 1:2 w linespoints title "page refs"
plot "testRig4K.dat" u 1:2 w linespoints title "page size 4K bytes", \
     "testRig64.dat" u 1:2 w linespoints titel "page size 64 bytes" ,\
     "dummy.dat" u 1:2 w linespoints title "dummy"
