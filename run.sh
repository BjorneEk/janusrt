#!/bin/bash

# Author: Gustaf Franzen <gustaffranzen@icloud.com>

set -e
set -x

ARCH=arm64
QEMU=qemu-system-aarch64
#QEMU=qemu

JRT_MEM_PHYS="$1"
JRT_MEM_SIZE="$2"

JRT_MEM_HEX=$(printf "0x%x" "$JRT_MEM_PHYS")

KERNEL="kernel/arch/$ARCH/boot/Image"
INITRAMFS='initramfs.cpio'
#memmap=16M$0x3f000000 KERNEL_CMDLINE='console=ttyAMA0 isolcpus=3 nohz_full=3 rcu_nocbs=3 rdinit=/init'
KERNEL_CMDLINE="console=ttyAMA0 root=/dev/ram rw isolcpus=3 nohz_full=3 rcu_nocbs=3 memmap=16M\$${JRT_MEM_HEX}"
NCPUS='4'
MEM='2048'
MACH='virt,gic-version=3'
CPU='cortex-a72'

JRT_OUT='jrt.log'

$QEMU \
	-serial mon:stdio \
	-serial "file:$JRT_OUT" \
	-M      "$MACH" \
	-cpu    "$CPU" \
	-smp    "$NCPUS" \
	-m      "$MEM" \
	-kernel "$KERNEL" \
	-initrd "$INITRAMFS" \
	-nographic \
	-append "$KERNEL_CMDLINE"

