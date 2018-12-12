set terminal png
set output "greenVSpthread4.png"

set title "green VS pthread (4 Threads)"

set key left center

set xlabel "#productions"
set ylabel "#time (ms)"

plot  "greenVSpthread4.dat" u 1:2 w linespoints title "green",\
      "greenVSpthread4.dat" u 1:3 w linespoints title "pthread",\