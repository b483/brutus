#!/usr/bin/env bash

set -e

case "$BUILD_TARGET" in
"mac")
	cd build && make -j4 && make test && make install
	echo "Creating disk image"
	hdiutil create -volname Julius -srcfolder julius.app -ov -format UDZO julius.dmg
	;;
"appimage")
	cd build && make -j4 && make test
	make DESTDIR=AppDir install
	cd ..
	./.ci_scripts/package_appimage.sh
	;;
"linux")
	cd build && make -j4 && make test
	zip julius.zip julius
	;;
*)
	cd build && make -j4 && make test
	;;
esac
