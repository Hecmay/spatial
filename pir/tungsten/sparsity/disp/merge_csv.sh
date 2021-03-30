#!/bin/bash 

# Merge count histograms
#sh -c 'for prio in rand static; do for i in 1 2 3 64; do for ifof in "if" of; do echo "sparsity/raw_data/${i}_${ifof}_16_${prio}/iss_PMU.csv "; done; done; done' | xargs csvjoin -c1 > sparsity/merged_data/iss_hist.csv
ls sparsity/raw_data/*/iss_PMU.csv | xargs csvjoin -c1 > sparsity/merged_data/iss_hist.csv
sed -i '/^,*$/d' sparsity/merged_data/iss_hist.csv

#for prio in rand static; do 
  #for i in 1 2 3 64; do 
    #for ifof in "if" of; do 
      #pushd "sparsity/raw_data/${i}_${ifof}_16_${prio}/"; 
for dir in `ls sparsity/raw_data`; do
  pushd "sparsity/raw_data/$dir"
    csvstack cycles.csv reads.csv writes.csv > counters_stack.csv
  popd
done
    #done
  #done
#done

#sh -c 'for prio in rand static; do for i in 1 2 3 64; do for ifof in "if" of; do echo "sparsity/raw_data/${i}_${ifof}_16_${prio}/counters_stack.csv "; done; done; done' | xargs csvjoin -c 1 > sparsity/merged_data/counters.csv;
ls sparsity/raw_data/*/counters_stack.csv | xargs csvjoin -c 1 > sparsity/merged_data/counters.csv;

./sparsity/disp/merge.py
./sparsity/disp/gen_tabs.sh
