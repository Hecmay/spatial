set terminal pdf noenhanced size 8in,3in

set output 'plots/iter.pdf'
set grid
set multiplot layout 1, 3

set yrange [0:]
set style data histogram

set style histogram cluster gap 1

set style fill solid border rgb "black"
set auto x
set xtics rotate by 45 right
set xlabel "Iteration"
set ylabel "Score"

set title "DotProduct"
plot for [route in "bdor directed_valiant dor min_local min_valiant valiant"] 'data/iteration/route_'.route.'_DotProduct.dat' using 1:2 with lines title route

set title "OuterProduct"
plot for [route in "bdor directed_valiant dor min_local min_valiant valiant"] 'data/iteration/route_'.route.'_OuterProduct.dat' using 1:2 with lines title route

set title "BlackScholes"
plot for [route in "bdor directed_valiant dor min_local min_valiant valiant"] 'data/iteration/route_'.route.'_BlackScholes.dat' using 1:2 with lines title route

