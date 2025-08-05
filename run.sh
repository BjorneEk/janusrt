#!/bin/bash

# Author: Gustaf Franzen <gustaffranzen@icloud.com>

set -e
set -x

ARCH=arm64
QEMU=qemu-system-aarch64
#QEMU=qemu

KERNEL="kernel/arch/$ARCH/boot/Image"
INITRAMFS='initramfs.cpio'
#KERNEL_CMDLINE='console=ttyAMA0 isolcpus=3 nohz_full=3 rcu_nocbs=3 rdinit=/init'
KERNEL_CMDLINE='console=ttyAMA0 root=/dev/ram rw isolcpus=3 nohz_full=3 rcu_nocbs=3'
NCPUS='4'
MEM='2048'
MACH='virt,gic-version=3'
CPU='cortex-a72'

JRT_OUT='jrt-log'
JRT_MEM="$1"

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

