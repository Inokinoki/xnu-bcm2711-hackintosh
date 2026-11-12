#!/bin/sh
# Check out the pinned BearSSL (the one the kernel stages, xnu/patches-8796.141.3/0025-stage-bearssl.sh) into DIR,
# reusing a checkout a CI cache restored. bearssl.org answers 502 now and then, hence the retries.
#
#   userland/fetch-bearssl.sh DIR
set -eu
DIR=${1:?usage: $0 DIR}
SHA=7bea48e5e850ab4cafbe68d3765cdaba13a86d6f
if [ -d "$DIR/.git" ] && [ "$(git -C "$DIR" rev-parse HEAD 2>/dev/null)" = "$SHA" ]; then
	git -C "$DIR" checkout -q --force HEAD
	echo "BearSSL $SHA from the cache"
	exit 0
fi
rm -rf "$DIR"
mkdir -p "$DIR"
git init -q "$DIR"
attempt=1
while :; do
	git -C "$DIR" fetch -q --depth 1 https://www.bearssl.org/git/BearSSL "$SHA" && break
	[ "$attempt" -lt 5 ] || exit 1
	sleep $((attempt * 10))
	attempt=$((attempt + 1))
done
git -C "$DIR" checkout -q FETCH_HEAD
[ "$(git -C "$DIR" rev-parse HEAD)" = "$SHA" ]
echo "BearSSL $SHA fetched"
