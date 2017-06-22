#!/bin/sh
scripts/positiontoredis.sh -c -a
rm core
ulimit -c unlimited
local/bin/stem 127.0.0.1 0 -s:configs/symbol_config_new.xml -t:configs/trading_config_new.xml -a:configs/admin_config.xml -v:TRACE
