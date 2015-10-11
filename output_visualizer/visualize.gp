set title "Simple visualization of WDD output"
set autoscale
set style data points
set xlabel "Time"
set ylabel "X"
set zlabel "Y"
set key box
splot "output_xyz.csv" u 3:1:2, "output_clusters.csv" u 3:1:2 w lines
pause -1 "Hit return to continue."