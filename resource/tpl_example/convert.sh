#!/bin/bash

DST=/opt/sg/protocols/tpl_example

for p in `ls raw_*.json`
do
    f=${p##raw_}
    cat $p | json_reformat > $f
    cp $f $DST
done

for p in `ls raw_*.json`
do
    f=${p##raw_}
    rm $f
done
