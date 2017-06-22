#!/bin/sh

rm core
ulimit -c unlimited
../bin/stem 127.0.0.1 0 -s:../configs/symbol_config_onlyC.xml -t:../configs/trading_config_new.xml -a:../configs/admin_config_server.xml -v:TRACE 

