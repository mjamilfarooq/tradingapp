#!/bin/sh
kill `ps -e|grep "stem"|cut -f1 -d' '`
