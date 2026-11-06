#!/bin/sh
# libz, libbz2, liblzma: the three decompressors text_cmds' grep links (-lbz2 -llzma -lz, for -Z/-J/-M).
# zlib-83 and bzip2-45 are Apple's sources, compiled directly; liblzma is upstream xz (Apple's xz is not in the
# open-source manifest). Headers go to the fakeroot's usr/local/include so the Xcode builds find them.
#
#   userland/build-compress-libs.sh DXBDIR SRCDIR OUTROOT WORKDIR
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
DXB=$(cd "${1:?usage: $0 DXBDIR SRCDIR OUTROOT WORKDIR}" && pwd)
SRC=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}" "${4:?}"
OUT=$(cd "$3" && pwd)
WORK=$(cd "$4" && pwd)
SDK=$(xcrun --sdk macosx --show-sdk-path)
INC=$DXB/fakeroot/usr/local/include
mkdir -p "$OUT/usr/lib" "$INC"
CC="xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot $SDK -Wno-error"
dylib() {	# NAME VERSION-LIST...: link the objects in $WORK/NAME/*.o
	name=$1 install=$2 compat=$3 cur=$4
	# shellcheck disable=SC2046
	$CC -dynamiclib -nostdlib -install_name "$install" -compatibility_version "$compat" -current_version "$cur" \
		-L"$OUT/usr/lib" -lSystem "$WORK/$name"/*.o "$(xcrun clang -print-resource-dir)/lib/darwin/libclang_rt.osx.a" \
		-o "$OUT$install"
}

# zlib
Z=$SRC/zlib/zlib
mkdir -p "$WORK/z"
for f in adler32 compress crc32 deflate gzclose gzlib gzread gzwrite infback inffast inflate inftrees trees uncompr zutil; do
	$CC -include stdio.h -include unistd.h -DHAVE_UNISTD_H -DHAVE_STDARG_H -I"$Z" -c "$Z/$f.c" -o "$WORK/z/$f.o"
done
dylib z /usr/lib/libz.1.dylib 1.0.0 1.2.12
ln -sf libz.1.dylib "$OUT/usr/lib/libz.dylib"
cp "$Z/zlib.h" "$Z/zconf.h" "$INC/"

# bzip2
B=$SRC/bzip2/bzip2
mkdir -p "$WORK/bz2"
for f in blocksort bzlib compress crctable decompress huffman randtable; do
	$CC -D_FILE_OFFSET_BITS=64 -I"$B" -c "$B/$f.c" -o "$WORK/bz2/$f.o"
done
$CC -I"$B" -c "$HERE/src/bz2_crc32_vec.c" -o "$WORK/bz2/crc32_vec.o"
dylib bz2 /usr/lib/libbz2.1.0.dylib 1.0.0 1.0.8
ln -sf libbz2.1.0.dylib "$OUT/usr/lib/libbz2.dylib"
cp "$B/bzlib.h" "$INC/"

# liblzma (xz 5.4.6, the last release before the 5.6 backdoor; the tarball is pinned by hash)
XZ=xz-5.4.6
XZ_SHA256=aeba3e03bf8140ddedf62a0a367158340520f6b384f75ca6045ccc6c0d43fd5c
if [ ! -f "$WORK/$XZ/configure" ]; then
	curl -fsSL --retry 4 -o "$WORK/$XZ.tar.gz" "https://github.com/tukaani-project/xz/releases/download/v5.4.6/$XZ.tar.gz"
	echo "$XZ_SHA256  $WORK/$XZ.tar.gz" | shasum -a 256 -c -
	tar -xzf "$WORK/$XZ.tar.gz" -C "$WORK"
fi
mkdir -p "$WORK/xz-build"
(
	cd "$WORK/xz-build"
	"$WORK/$XZ/configure" CC="xcrun clang" CFLAGS="-target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot $SDK" \
		LDFLAGS="-target arm64-apple-macos13.0 -isysroot $SDK" --prefix=/usr \
		--disable-xz --disable-xzdec --disable-lzmadec --disable-lzmainfo --disable-lzma-links --disable-scripts \
		--disable-doc --disable-nls --disable-threads --disable-static --enable-shared >configure.log 2>&1 || { tail -30 configure.log; exit 1; }
	make -C src/liblzma -j4 >make.log 2>&1 || { tail -30 make.log; exit 1; }
	make -C src/liblzma install DESTDIR="$WORK/xz-install" >install.log 2>&1 || { tail -30 install.log; exit 1; }
)
cp -P "$WORK"/xz-install/usr/lib/liblzma*.dylib "$OUT/usr/lib/"
cp "$WORK/xz-install/usr/include/lzma.h" "$INC/"
cp -R "$WORK/xz-install/usr/include/lzma" "$INC/"
ls -l "$OUT"/usr/lib/libz* "$OUT"/usr/lib/libbz2* "$OUT"/usr/lib/liblzma*
