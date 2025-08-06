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
#KERNEL_CMDLINE="console=ttyAMA0 root=/dev/ram rw isolcpus=3 nohz_full=3 rcu_nocbs=3 memmap=16M\$${JRT_MEM_HEX}"
#KERNEL_CMDLINE="console=ttyAMA0 root=/dev/ram rw isolcpus=3 nohz_full=3 rcu_nocbs=3 mem=1280M maxcpus=3"
KERNEL_CMDLINE="root=/dev/ram rw isolcpus=3 nohz_full=3 rcu_nocbs=3 mem=1280M maxcpus=3"
NCPUS='4'
MEM='4096'
MACH='virt,gic-version=3,secure=on'
CPU='cortex-a72'
DTB="$3"
JRT_OUT='jrt.log'
JRT_OUT2='jrt2.log'
#-icount shift=7,align=on,sleep=on \
$QEMU \
	-serial mon:stdio \
	-serial "file:$JRT_OUT" \
	-serial "file:$JRT_OUT2" \
	-dtb	"$DTB" \
	-M      "$MACH" \
	-cpu    "$CPU" \
	-smp    "$NCPUS",threads="$NCPUS" \
	-m      "$MEM" \
	-kernel "$KERNEL" \
	-initrd "$INITRAMFS" \
	-append "$KERNEL_CMDLINE" \
	-nographic
#-icount shift=7,align=on,sleep=on \
#-d guest_errors,unimp,exec \
