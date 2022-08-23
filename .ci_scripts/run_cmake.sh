#!/usr/bin/env bash

case "$BUILD_TARGET" in
"mac")
	mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DSYSTEM_LIBS=OFF ..
	;;
"appimage")
	mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DSYSTEM_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
	;;
"linux")
    if [ ! -z "$MPG123_VERSION" ]
    then
		MPG123_OPT="-DLINK_MPG123=ON"
	fi
	mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DSYSTEM_LIBS=OFF $MPG123_OPT ..
	;;
*)
	mkdir build && cd build && cmake ..
	;;
esac
