#!/bin/bash

SRC=/opt/app/sg_upload
DST=/opt/app/sg

mkdir -p $DST/lib
mkdir -p $DST/ms
mkdir -p $DST/cs
mkdir -p $DST/fs-1
mkdir -p $DST/fs-2
mkdir -p $DST/fs-3
mkdir -p $DST/fs-4
mkdir -p $DST/fs-5
mkdir -p $DST/fs-6
mkdir -p $DST/fs-7
mkdir -p $DST/fs-8
mkdir -p $DST/resource

cp -u $SRC/lib/* $DST/lib
cp -u $SRC/ms/map_server $DST/ms
cp -u $SRC/cs/center_server $DST/cs
cp -u $SRC/fs-1/front_server $DST/fs-1
cp -u $SRC/fs-2/front_server $DST/fs-2
cp -u $SRC/fs-3/front_server $DST/fs-3
cp -u $SRC/fs-4/front_server $DST/fs-4
cp -u $SRC/fs-5/front_server $DST/fs-5
cp -u $SRC/fs-6/front_server $DST/fs-6
cp -u $SRC/fs-7/front_server $DST/fs-7
cp -u $SRC/fs-8/front_server $DST/fs-8

rm -rf $DST/resource/script
rm -rf $DST/resource/cs-script
rm -rf $DST/resource/fs-script
cp -u -r $SRC/resource/* $DST/resource

echo 'copy done ...'
