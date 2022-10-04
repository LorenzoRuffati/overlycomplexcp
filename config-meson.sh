#!/bin/bash

case "$1" in
	"-d") debug=true;;
	"-r") debug=false;;
esac

if [ "$debug" = true ]; then
	meson configure builddir --buildtype=debug;
else
	meson configure builddir --buildtype=release --strip;
fi
