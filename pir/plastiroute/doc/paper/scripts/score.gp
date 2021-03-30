set terminal pdf noenhanced size 8in,3in

set output 'plots/score.pdf'
set grid
set multiplot layout 1, 3

set key off
set log y

set style data histogram

set style histogram cluster gap 1

set style fill solid border rgb "black"
set auto x
set xtics rotate by 45 right
set xlabel "Routing Function"
set ylabel "Score"

bmarks="DotProduct OuterProduct BlackScholes"
plot for [bmark in "DotProduct"] 'data/routing/data_score_vs_routing_DotProduct.dat' using ($2):xtic(1)  title "Score"
plot for [bmark in "OuterProduct"] 'data/routing/data_score_vs_routing_'.bmark.'.dat' using 2:xtic(1)  title "Score"
plot for [bmark in "BlackScholes"] 'data/routing/data_score_vs_routing_'.bmark.'.dat' using 2:xtic(1)  title "Score"

