#!/usr/local/bin/gnuplot -persist
# set terminal postscript landscape noenhanced monochrome \
#              dashed defaultplex "Helvetica" 14
set term gif
set output "plot.gif"
set xlabel "inter-packet gap" 
set ylabel "count" 
set log y
set title "Inter-packet gap distribution"
#set xrange [ 0 : 2 ] noreverse nowriteback
#set yrange [ 0 : 1 ] noreverse nowriteback
plot "sample_data" using 1:2 #title "samples"
#    EOF
