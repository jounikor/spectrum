#!/bin/sh

# Usage:
#  sh prep.sh zxpac2_output_debug literals_output matches_output consequtive_literals
#


if [[ $# -eq 4 ]]; then
    
    # literal dostributions
    echo "Literals" > $2
    grep RAW $1 | sed -E 's/.*RAW ([0-9]{0,3}) .*/\1/' >> $2

    # match offset and length distributions
    echo "Offset,Length" > $3
    grep MTC $1 | sed -E 's/.*MTC ([0-9]{0,5}),([0-9]{0,3}) .*/\1,\2/' >> $3

    # consequtive literal runs
    raws=0
    echo "Literal_runs" > $4
    while read -r line; do
        if [[ "$line" =~ .*RAW.* ]]; then
            raws=$((raws + 1))
        else
            echo "$raws" >> $4
            raws=0
        fi



    done < $1



fi
