#!/usr/bin/python3

import pandas as pd
from os import listdir
import csv

def extract(name, fname, rowname):
    with open('sparsity/raw_data/'+name+'/'+fname) as f:
        csvr = csv.reader(f)
        for r in csvr:
            assert(len(r) == 2)
            if (r[0] == rowname):
                return float(r[1])

print('BankedPct,Throughput,Idx')
idx=0
for banked_pct in [0, 70, 90, 97, 99, 100]:
    reads = extract(f'PMU_RO_{banked_pct}', 'reads.csv', 'reads')
    cycles = extract(f'PMU_RO_{banked_pct}', 'cycles.csv', 'cycles')
    #print(reads)
    #print(f'Reads: {reads} Cycles: {cycles}')
    #print(f'Throughput: {reads/cycles}')
    print(f'{banked_pct},{reads/cycles},{idx}')
    idx += 1
