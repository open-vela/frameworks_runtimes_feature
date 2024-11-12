#!/bin/sh
find vendor/xiaomi/vela/service_agent/feature/jidl/ -name '*.jidl' | grep -Ev 'utils' | xargs grep -E '^module|@' | \
sed -E 's/.*:module ((.*)@.*)/\2/; s/\./_/g' | sed ':a;N;$!ba;s/\n/;/g' | sed -z 's/\n//g'