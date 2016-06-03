#!/bin/bash

if [ "$1" == "--include-dirs" ]; then
		INCS=`pkg-config --cflags glib-2.0`

		for i in $INCS; do
			echo ${i#*-I}
		done
fi
