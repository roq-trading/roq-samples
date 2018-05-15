#!/usr/bin/env bash

if [ "$1" == "debug" ]; then
	shift 1
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

$PREFIX ./example-simple \
	--config-file simple.conf \
	--gateways femas=test:1234@/var/tmp/femasapi.sock $@
#	--gateways femas=trader:secret@/var/tmp/femasapi.sock $@
