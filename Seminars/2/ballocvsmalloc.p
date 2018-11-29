set terminal png
set output "ballocvsmalloc.png"

set title "Time difference between balloc and malloc"

set key right center

set xlabel "#size in bytes"
set ylabel "#time in ms"

plot  "ballocvsmalloc.dat" u 1:2 w linespoints title "balloc",\
      "ballocvsmalloc.dat" u 1:3 w linespoints title "malloc"