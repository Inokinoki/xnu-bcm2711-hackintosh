#!/bin/sh
# libunwind, libc++abi and libc++ from upstream LLVM (Apple's are closed or part of the dyld shared cache), built as
# plain arm64 for the Cortex-A72 against the libSystem umbrella in STAGE (build-libsystem.sh), installed into
# OUTROOT/usr/lib with install names /usr/lib/<name>.
#
#   userland/build-llvm-runtimes.sh WORKDIR OUTROOT STAGE
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "${1:?usage: $0 WORKDIR OUTROOT STAGE}" "${2:?}" "${3:?}"
WORK=$(cd "$1" && pwd)
OUT=$(cd "$2" && pwd)
STAGE=$(cd "$3" && pwd)
TAG=${LLVM_TAG:-llvmorg-17.0.6}
SDK=$(xcrun --sdk macosx --show-sdk-path)

if [ ! -d "$WORK/llvm-project/.git" ]; then
	git clone -q --depth 1 --branch "$TAG" --filter=blob:none --sparse https://github.com/llvm/llvm-project "$WORK/llvm-project"
	git -C "$WORK/llvm-project" sparse-checkout set runtimes libcxx libcxxabi libunwind cmake llvm/cmake llvm/utils third-party
fi
git -C "$WORK/llvm-project" log -1 --format="llvm-project %H"

LDF="-nostdlib -L$STAGE -L$OUT/usr/lib/system"
cmake -S "$WORK/llvm-project/runtimes" -B "$WORK/llvm-build" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER="$(xcrun -f clang)" -DCMAKE_CXX_COMPILER="$(xcrun -f clang++)" \
	-DCMAKE_OSX_SYSROOT="$SDK" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
	-DCMAKE_C_FLAGS="-mcpu=cortex-a72" -DCMAKE_CXX_FLAGS="-mcpu=cortex-a72" \
	-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	-DCMAKE_SHARED_LINKER_FLAGS="$LDF -lSystem" -DCMAKE_EXE_LINKER_FLAGS="$LDF -lSystem" \
	-DCMAKE_INSTALL_PREFIX="$WORK/llvm-install" -DCMAKE_INSTALL_NAME_DIR=/usr/lib \
	-DLLVM_ENABLE_RUNTIMES="libunwind;libcxxabi;libcxx" \
	-DLLVM_INCLUDE_TESTS=OFF -DLLVM_ENABLE_ASSERTIONS=OFF \
	-DLIBCXX_CXX_ABI=libcxxabi -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
	-DLIBCXX_ENABLE_STATIC=OFF -DLIBCXXABI_ENABLE_STATIC=OFF -DLIBUNWIND_ENABLE_STATIC=OFF \
	-DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF -DLIBCXX_INCLUDE_DOCS=OFF \
	-DLIBCXXABI_INCLUDE_TESTS=OFF -DLIBUNWIND_INCLUDE_TESTS=OFF
ninja -C "$WORK/llvm-build"
ninja -C "$WORK/llvm-build" install

mkdir -p "$OUT/usr/lib"
cp -a "$WORK/llvm-install/lib/"*.dylib "$OUT/usr/lib/"
ls -l "$OUT/usr/lib"
