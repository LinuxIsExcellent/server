#!/bin/bash

#参数1： 命令
# map
# center 
# front
TYPE=$1

cd bin

#项目主目录
PROJECT_PATH=/home/tom/sg_server

case "$TYPE" in
  ms)
    ./map_server $PROJECT_PATH
    ;;
  cs)
    ./center_server $PROJECT_PATH
    ;;
  fs)
    ./front_server $PROJECT_PATH
    ;;
  test)
    ./test_client $PROJECT_PATH
    ;;
  *)
    echo "type[$TYPE] invalid param, please sure [ms|cs|fs]"
    exit 2
esac

