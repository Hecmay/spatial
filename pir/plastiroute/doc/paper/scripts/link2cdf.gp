set terminal pdf size 3in,2in

set output 'plots/link_cdf.pdf'
set grid
set multiplot

set xlabel "Logical links"
set ylabel "Total tokens"

bmarks="DotProduct OuterProduct GDA LogReg BlackScholes GEMM\_Blocked TPCHQ6"
plot for [bmark in bmarks] 'data/link_cdf/'.bmark.'.dat' using 1:2 with lines title bmark
unset multiplot
unset output
