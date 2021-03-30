set terminal pdf noenhanced size 8in,4in

set output 'plots/routefunc.pdf'
set grid
set multiplot layout 1, 3

set key bmargin

set log y
set style data histogram

set style histogram cluster gap 1

set style fill solid border rgb "black"
set boxwidth 0.9
set auto x
set xtics rotate by 45 right
set xlabel "Routing Function"
set ylabel "Score"

bmarks="DotProduct OuterProduct BlackScholes"
set title "DotProduct"
plot for [bmark in "DotProduct"] 'data/routing/data_score_vs_routing_DotProduct.dat' using 3:xtic(1)  title "Congestion", \
      '' using 4  title "Longest Route", \
      '' using 5  title "Total Hops"
set title "OuterProduct"
plot for [bmark in "OuterProduct"] 'data/routing/data_score_vs_routing_'.bmark.'.dat' using 3:xtic(1)  title "Congestion", \
      '' using 4:xtic(1)  title "Longest Route", \
      '' using 5:xtic(1)  title "Total Hops"
set title "BlackScholes"
plot for [bmark in "BlackScholes"] 'data/routing/data_score_vs_routing_'.bmark.'.dat' using 3:xtic(1)  title "Congestion", \
      '' using 4:xtic(1)  title "Longest Route", \
      '' using 5:xtic(1)  title "Total Hops"

