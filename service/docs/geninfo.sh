#!/bin/bash

if [[ $# != 1 ]]; then
    echo "Usage: $0 system_name"
    echo
    exit -1
fi
system=$1
outfile=source/signals_${system}.rst
echo "Signals" > ${outfile}
echo "=======" >> ${outfile}
echo >> ${outfile}
for sname in $(geopmread); do
    geopmread --info ${sname} |
        sed -e 's|^    \(.*\)|    - \1:|' -e 's|:$||' >> ${outfile}
done

outfile=source/controls_${system}.rst
echo "Controls" > ${outfile}
echo "========" >> ${outfile}
echo >> ${outfile}

for cname in $(geopmwrite); do
    geopmwrite --info ${cname} |
        sed -e 's|^    \(.*\)|    - \1:|' -e 's|:$||' >> ${outfile}
done
