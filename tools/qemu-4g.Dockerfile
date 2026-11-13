# QEMU with a 4 GiB raspi4b, for testing the 4G memory layout (upstream raspi4b is fixed at 2 GiB by its board revision).
#
#   docker build -t pi4-qemu-4g -f tools/qemu-4g.Dockerfile tools
#   docker run --rm -u $(id -u):$(id -g) -v $PWD:/w:ro -v $PWD/build/qemu-4g:/out -w /w \
#     -e CC=aarch64-none-elf-gcc -e OUT=/out -e RAMDISK=/w/build/root.img -e MEM=4G \
#     pi4-qemu-4g tools/pi4-qemu-boot.sh KERNEL 90
FROM alpine:edge AS build
RUN apk add --no-cache build-base git python3 py3-pip py3-setuptools ninja meson glib-dev pixman-dev bash zlib-dev perl flex bison linux-headers curl xz dtc-dev
RUN curl -fL https://download.qemu.org/qemu-10.1.0.tar.xz | tar xJ -C /
WORKDIR /qemu-10.1.0
# raspi4b: board revision c03115 = Pi 4B rev 1.5 with 4 GiB (upstream only has the 2 GiB b03115)
RUN grep -n "0xb03115" hw/arm/raspi4b.c && sed -i 's/0xb03115/0xc03115/' hw/arm/raspi4b.c
RUN ./configure --target-list=aarch64-softmmu --disable-docs --disable-tools --disable-guest-agent --disable-werror --prefix=/opt/q4 \
 && make -j"$(nproc)" && make install
FROM alpine:edge
RUN apk add --no-cache gcc-aarch64-none-elf binutils-aarch64-none-elf python3 coreutils glib pixman zlib libgcc libstdc++ dtc
COPY --from=build /opt/q4 /opt/q4
ENV PATH=/opt/q4/bin:$PATH
