#!/usr/bin/python3

import pandas as pd
from os import listdir
import csv


class Point:
    def extract(self, fname, rowname):
        with open('sparsity/raw_data/'+self.name+'/'+fname) as f:
            csvr = csv.reader(f)
            for r in csvr:
                assert(len(r) == 2)
                if (r[0] == rowname):
                    #setattr(self, r[0], r[1])
                    return r[1]

    def extract_hist(self, fname, statname):
        with open('sparsity/raw_data/'+self.name+'/'+fname) as f:
            csvr = csv.reader(f)
            next(csvr)
            ret = {}
            for r in csvr:
                if (len(r) == 0):
                    continue
                assert(len(r) == 2)
                ret[r[0]] = r[1]
            return ret

    def extract_dim_hist(self, fname, statname):
        with open('sparsity/raw_data/'+self.name+'/'+fname) as f:
            csvr = csv.reader(f)
            next(csvr)
            ret = {}
            for r in csvr:
                if (len(r) == 0):
                    continue
                assert(len(r) == 3)
                if not r[0] in ret:
                    ret[r[0]] = {}
                ret[r[0]][r[1]] = r[2]
            return ret

    def __init__(self, string):
        self.name = string
        fields = string.split('_')
        self.iter = fields[0]
        self.alloc = fields[1]
        self.pipe_depth = fields[2]
        self.alloc_prio = fields[3]
        self.reorder = fields[4]
        self.acc_lat = fields[5]
        self.split_width = fields[6]
        self.max_addr = fields[7]
        self.cycles = self.extract('cycles.csv', 'cycles')
        self.reads = self.extract('reads.csv', 'reads')
        self.writes = self.extract('writes.csv', 'writes')
        self.iss_PMU = self.extract_hist('iss_PMU.csv', 'iss_PMU')
        self.req_PMU = self.extract_hist('req_PMU.csv', 'req_PMU')
        self.iss_lock = self.extract_hist('iss_lock.csv', 'iss_lock')
        self.req_lock = self.extract_hist('req_lock.csv', 'req_lock')
        self.iss_unlock = self.extract_hist('iss_unlock.csv', 'iss_unlock')
        self.req_unlock = self.extract_hist('req_unlock.csv', 'req_unlock')
        self.stage_occ_PMU = self.extract_hist('stage_occ_PMU.csv', 'stage_occ_PMU')
        self.stage_occ_lock = self.extract_hist('stage_occ_lock.csv', 'stage_occ_lock')
        self.stage_occ_PMU_2d = self.extract_dim_hist('stage_occ_PMU_2d.csv', 'stage_occ_PMU_2d')
        self.stage_occ_lock_2d = self.extract_dim_hist('stage_occ_lock_2d.csv', 'stage_occ_lock_2d')
        self.num_splits = self.extract_hist('num_splits.csv', 'num_splits')

    def get(self, stat):
        if (stat == 'cycles'):
            return float(self.cycles)
        elif (stat == 'reads'):
            return float(self.reads)
        elif (stat == 'writes'):
            return float(self.writes)
        elif (stat == 'accesses'):
            return float(self.reads) + float(self.writes)
        elif (stat == 'bandwidth'):
            return (float(self.reads) + float(self.writes))/float(self.cycles)
        else:
            raise NotImplementedError()

    def get_hist(self, stat):
        if (stat == 'iss_PMU'):
            return self.iss_PMU
        elif (stat == 'req_PMU'):
            return self.req_PMU
        elif (stat == 'iss_lock'):
            return self.iss_lock
        elif (stat == 'req_lock'):
            return self.req_lock
        elif (stat == 'iss_unlock'):
            return self.iss_unlock
        elif (stat == 'req_unlock'):
            return self.req_unlock
        elif (stat == 'stage_occ_PMU'):
            return self.stage_occ_PMU
        elif (stat == 'stage_occ_lock'):
            return self.stage_occ_lock
        elif (stat == 'stage_occ_PMU_2d'):
            return self.stage_occ_PMU_2d
        elif (stat == 'stage_occ_lock_2d'):
            return self.stage_occ_lock_2d
        elif (stat == 'num_splits'):
            return self.num_splits
        else:
            raise NotImplementedError()

    def to_df(self):
        return pd.DataFrame(data={
            'iter'        : [self.iter],
            'alloc'       : [self.alloc],
            'pipe_depth'  : [self.pipe_depth],
            'alloc_prio'  : [self.alloc_prio],
            'reorder'     : [self.reorder],
            'acc_lat'     : [self.acc_lat],
            'split_width' : [self.split_width],
            'max_addr'    : [self.max_addr],
            'name'        : [self.name],
            'point'       : [self]})

    def str(self):
        return f'''\titer={self.iter}
\talloc={self.alloc}
\tpipe_depth={self.pipe_depth}
\talloc_prio={self.alloc_prio}
\treorder={self.reorder}
\tacc_lat={self.acc_lat}
\tsplit_width={self.split_width}
\tmax_addr={self.max_addr}

\tcycles={self.cycles}
\treads={self.reads}
\twrites={self.writes}
\tiss_PMU={self.iss_PMU}
\treq_PMU={self.req_PMU}
'''

def RestrictDF(df, cons):
    for key, val in cons.items():
        df = df[df[key]==val]
    return df

class Table:
    def __init__(self, glob_df):
        self.df = glob_df
        self.all_cons = {}
        self.row_cons = []
        self.row_names = []
        self.col_cons = []
        self.col_names = []
    def BaseCons(self, cons):
        for key, val in cons.items():
            self.all_cons[key] = val
    def AddBaseCon(self, key, val):
        self.all_cons[key] = val
    def AddRow(self, row, name):
        self.row_cons.append(row)
        self.row_names.append(name)
    def AddCol(self, col, name):
        self.col_cons.append(col)
        self.col_names.append(name)
    def Debug(self):
        print("All: " + str(self.all_cons))
        print("Rows: " + str(self.row_cons))
        print("Cols: " + str(self.col_cons))
        print("Row names: " + str(self.row_names))
        print("Col names: " + str(self.col_names))
    def Arrange(self):
        all_df = RestrictDF(self.df, self.all_cons)
        self.table = []
        for r in self.row_cons:
            row_df = RestrictDF(all_df, r)
            row = []
            for c in self.col_cons:
                col_df = RestrictDF(row_df, c)
                assert(len(col_df) == 1)
                row.append(col_df['point'].item())
            self.table.append(row)
    def GetStat(self, stat, output, normstat=None):
        self.Arrange()
        f = open(output, "w")
        for c in self.col_names:
            f.write(' ,'+c)
        f.write('\n')
        for r, n in zip(self.table, self.row_names):
            f.write(n)
            for c in r:
                if (normstat):
                    f.write(','+str(float(c.get(stat))/float(c.get(normstat))))
                else:
                    f.write(','+str(c.get(stat)))
            f.write('\n')
        f.write('\n')
        f.close()
    def GetHistStat(self, stat, output="/dev/stdout", dim=None, default="0"):
        self.row_cons = [{}]
        self.row_names = [""]
        self.Arrange()
        coltab = self.table[0]
        allkeys = []
        if dim:
            for c in coltab:
                allkeys = allkeys + list(c.get_hist(stat)[dim].keys())
        else:
            for c in coltab:
                allkeys = allkeys + list(c.get_hist(stat).keys())
        allkeys = list(set(allkeys))
        allkeys.sort(key=int)
        f = open(output, "w")
        f.write(stat)
        for c in self.col_names:
            f.write(','+c)
        f.write('\n')
        for k in allkeys:
            f.write(str(k))
            for c in coltab:
                h = c.get_hist(stat)
                if dim:
                    #print(h[dim])
                    val = h[dim].get(k,default)
                else:
                    val = h.get(k,default)
                f.write(','+str(val))
            f.write('\n')
        f.write('\n')
                    


design_pts = listdir('sparsity/raw_data')
pts_df = pd.DataFrame()

for pt in design_pts:
    parsed = Point(pt)
    #print(parsed.str())
    pts_df = pts_df.append(parsed.to_df(), ignore_index=True)

out_dir = 'sparsity/merged_data/'

cyc_tab = Table(pts_df)
cyc_tab.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'reorder' : '4096', 'split_width' : '0', 'max_addr' : '65536'})
for a in ['if', 'of']:
    for it in ['1', '2', '64']:
        cyc_tab.AddRow({'alloc' : a, 'iter' : it}, f"{a}-{it}")
cyc_tab.AddRow({'alloc' : 'augmenting', 'iter' : '1'}, f"augmenting")
for d in [1,2,4,8,16,64]:
    cyc_tab.AddCol({'pipe_depth' : str(d)}, f"{d}-stage")
cyc_tab.GetStat('cycles', out_dir+'cycles.csv')
cyc_tab.GetStat('accesses', out_dir+'throughput.csv', 'cycles')

iss_tab = Table(pts_df)
iss_tab.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'reorder' : '4096', 'alloc' : 'if', 'iter' : '2', 'split_width' : '0', 'max_addr' : '65536'})
for d in [1,2,4,8,16,64]:
    iss_tab.AddCol({'pipe_depth' : str(d)}, f"{d}-stage")
iss_tab.GetHistStat('iss_PMU', out_dir+'iss_PMU.csv')
iss_tab.GetHistStat('req_PMU', out_dir+'req_PMU.csv')
iss_tab.GetHistStat('iss_lock', out_dir+'iss_lock.csv')
iss_tab.GetHistStat('req_lock', out_dir+'req_lock.csv')

iss_req_depth = Table(pts_df)
iss_req_depth.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'pipe_depth' : '16', 'alloc' : 'if', 'iter' : '2', 'split_width' : '0', 'max_addr' : '65536'})
for d in [4,8,16,32,64,128,256,512,1024,2048,1024*1024]:
    iss_req_depth.AddCol({'reorder' : str(d)}, f"{d}-subbanks")
iss_req_depth.GetHistStat('iss_PMU', out_dir+'iss_PMU_reqs.csv')
iss_req_depth.GetHistStat('req_PMU', out_dir+'req_PMU_reqs.csv')
iss_req_depth.GetHistStat('iss_lock', out_dir+'iss_lock_reqs.csv')
iss_req_depth.GetHistStat('req_lock', out_dir+'req_lock_reqs.csv')

pmu_occ = Table(pts_df)
pmu_occ.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'reorder' : '4096', 'alloc' : 'if', 'iter' : '2', 'split_width' : '0', 'max_addr' : '65536'})
for d in [4, 8, 16, 64]:
    pmu_occ.AddCol({'pipe_depth' : str(d)}, f"{d}-pipedepth")
pmu_occ.GetHistStat('stage_occ_PMU', out_dir+'stage_occ_PMU_pipedepth.csv')
pmu_occ.GetHistStat('stage_occ_lock', out_dir+'stage_occ_lock_pipedepth.csv')

pmu_occ.GetHistStat('stage_occ_PMU_2d', out_dir+'stage_occ_PMU_pipedepth_read.csv', "0")
pmu_occ.GetHistStat('stage_occ_PMU_2d', out_dir+'stage_occ_PMU_pipedepth_write.csv', "1")

split_tab = Table(pts_df)
split_tab.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'reorder' : '4096', 'alloc' : 'if', 'iter' : '2', 'pipe_depth' : '16', 'max_addr' : '65536'})
for d in [0,4,16,64,128,256,512,1024]:
    split_tab.AddCol({'split_width' : str(d)}, f"{d}-split_width")
split_tab.GetHistStat('num_splits', out_dir+'num_splits.csv')

no_lock_hists = Table(pts_df)
no_lock_hists.BaseCons({'alloc_prio': 'rand', 'acc_lat' :'1', 'reorder' : '4096', 'alloc' : 'if', 'iter' : '2', 'split_width' : '1024'})
for d in [4, 8, 16, 64]:
    no_lock_hists.AddCol({'pipe_depth' : str(d), 'max_addr' : '65536'}, f"{d}-stage-lock")
    no_lock_hists.AddCol({'pipe_depth' : str(d), 'max_addr' : 'none'}, f"{d}-stage-nolock")
no_lock_hists.GetHistStat('stage_occ_PMU', out_dir+'stage_occ_PMU_conflict.csv')
no_lock_hists.GetHistStat('iss_PMU', out_dir+'iss_PMU_conflict.csv')
