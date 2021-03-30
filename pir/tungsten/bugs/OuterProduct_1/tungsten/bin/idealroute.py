import argparse
import csv

def main():
    parser = argparse.ArgumentParser(description='Generate ideal placement')
    parser.add_argument('-l', '--link', type=str, help='input path to link.csv')
    parser.add_argument('-p', '--place', type=str, help='output path to placement file')
    parser.add_argument('-i', '--idealnet', type=str, default='/idealnet', help='full path of idealnet module')

    (opts, args) = parser.parse_known_args()
    with open(opts.link, 'r') as f:
        with open(opts.place, 'w') as wf:
            csv_reader = csv.DictReader(f)
            srcid = 0
            dstid = 0
            for row in csv_reader:
                wf.write('set {} name {}\n'.format(opts.idealnet, row['link'].split('/')[-1]))
                wf.write('set {} src {}\n'.format(opts.idealnet, srcid))
                wf.write('set {} addr {}\n'.format(row['link'], srcid))
                wf.write('set {} flow {}\n'.format(row['link'], 0))
                wf.write('set {} net {}\n'.format(row['link'], "ideal"))
                srcid += 1
                for key in row:
                    if 'out' in key and row[key] != '' and row[key] is not None:
                        wf.write('set {} name {}\n'.format(opts.idealnet, row[key].split('/')[-1]))
                        wf.write('set {} dst {}\n'.format(opts.idealnet, dstid))
                        wf.write('set {} addr {}\n'.format(row[key], dstid))
                        wf.write('set {} flow {}\n'.format(row[key], 0))
                        wf.write('set {} net {}\n'.format(row[key], "ideal"))
                        dstid += 1
            wf.write("build\n")
            wf.write("disable net statnet\n")

main()
