set terminal pdf noenhanced size 8in,3in

set output 'plots/vc.pdf'
set grid
set multiplot layout 1, 3

set key off

set style data histogram

set style histogram cluster gap 1

set style fill solid border rgb "black"
set xtics rotate by 45 right
set xlabel "Routing Function"
set ylabel "VCs Required"
set yrange [0:12]

bmarks="DotProduct OuterProduct BlackScholes"
set title "DotProduct"
plot for [bmark in "DotProduct"] 'data/vc/vc_DotProduct.dat' using ($2):xtic(1)  title "VCs"
set title "OuterProduct"
plot for [bmark in "OuterProduct"] 'data/vc/vc_'.bmark.'.dat' using 2:xtic(1)  title "VCs"
set title "BlackScholes"
plot for [bmark in "BlackScholes"] 'data/vc/vc_'.bmark.'.dat' using 2:xtic(1)  title "VCs"

