set terminal png
set output "bfreevsfree.png"

set title "Time difference between bfreee and free"

set key right center

set xlabel "#size in bytes"
set ylabel "#time in ms"

plot  "bfreevsfree.dat" u 1:2 w linespoints title "bfree",\
      "bfreevsfree.dat" u 1:3 w linespoints title "free"