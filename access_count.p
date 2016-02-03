# Gnuplot script file for plotting data in file "bandwidth.dat"
# This file is called   bw.p


##################### terminal settings ##################
set size ratio 0.5
set term postscript eps enhanced color font "Times-Roman, 15" size 3.5,2.0


unset log                              # remove any log-scaling
unset label                            # remove any previous labels


##################### title, labels ######################
#set title "I/O test flash memory"

set xlabel "Page Address"
#set xrange[0:4000000]

set ylabel "Count"
#set yrange[0:600000]


##################### axis tics & labels ##################
#set xtics ("500" 500000, "1000" 1000000, "1500" 1500000, "2000" 2000000, "2500" 2500000, "3000" 3000000, "3500" 3500000, "4000" 4000000)
set xtics font "Times-Roman, 12"

#set ytics ("50" 50000, "100" 100000, "150" 150000, "200" 200000, "250" 250000, "300" 300000, "350" 350000, "400" 400000, "450" 450000, "500" 500000, "550" 550000,"600" 600000)
set ytics font "Times-Roman, 12"

set grid

set xtics
set ytics
set autoscale

#set decimal locale
#set format x "%'g"
#set format y "%'g"

##################### line ###############################
set style line 1 lt 1 lc rgb "cyan"  lw 1
set style line 2 lt 1 lc rgb "purple" lw 1
set style line 3 lt 1 lc rgb "navy" lw 1
set style line 4 lt 1 lc rgb "blue" lw 1
set style line 5 lt 1 lc rgb "green" lw 1
set style line 6 lt 1 lc rgb "gold" lw 1
set style line 7 lt 1 lc rgb "brown" lw 1
set style line 8 lt 1 lc rgb "red" lw 1
set style line 9 lt 1 lc rgb "salmon" lw 1
set style line 10 lt 1 lc rgb "blueviolet" lw 1
set style line 11 lt 1 lc rgb "skyblue" lw 1
set style line 12 lt 1 lc rgb "magenta" lw 1

set style line 20 lt 1 lc rgb "black" lw 1


##################### legend #############################
#set key 0.01,100
set key outside horizontal top center box ls 20
set key font "Times-Roman, 10" spacing 1.0
#set nokey

##################### user add any shape #################
#set label "4K" at 3630000, 80000 font "Times-Roman,10"
#set label "8K" at 3630000, 150000 font "Times-Roman,10"
#set label "16K" at 3630000, 250000 font "Times-Roman,10"
#set label "32K" at 3630000, 290000 font "Times-Roman,10"
#set label "64K" at 3630000, 360000 font "Times-Roman,10"
#set label "128K" at 3630000, 420000 font "Times-Roman,10"
#set label "256K" at 3630000, 455000 font "Times-Roman,10"
#set label "512K" at 3630000,480000 font "Times-Roman,10"
#set label "-" at 3640000,500000 font "Times-Roman,10"
#set label "8M" at 3630000,515000 font "Times-Roman,10"
#set arrow from 0.0028,250 to 0.003,280
#set xr [.0:0.022]
#set yr [0:325]


##################### plot ###############################
set output "access_count.eps"
plot "access_count.dat" u 1:2 with line ls 8 title "Total-count", "access_count.dat" u 1:3 with line ls 4 title "Write-count"
#plot "stat.dat" u 1:2 with line ls 1 title "dram hit", "stat.dat" u 1:3 with line ls 2 title "pcm hit", "stat.dat" u 1:4 with line ls 3 title "dram miss", "stat.dat" u 1:5 with line ls 4 title "pcm miss", "stat.dat" u 1:6 with line ls 5 title "pcm to dram", "stat.dat" u 1:7 with line ls 6 title "dram to pcm", "stat.dat" u 1:8 with line ls 8 title "write back"
