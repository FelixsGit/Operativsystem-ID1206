set terminal png
set output "greenVSpthread2.png"

set title "green VS pthread (2 Threads)"

set key left center

set xlabel "#productions"
set ylabel "#time (ms)"

plot  "greenVSpthread2.dat" u 1:2 w linespoints title "green",\
      "greenVSpthread2.dat" u 1:3 w linespoints title "pthread",\
