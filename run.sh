#!/bin/bash

# Author: Gustaf Franzen <gustaffranzen@icloud.com>

set -e
set -x

ARCH=arm64
QEMU=qemu-system-aarch64
#QEMU=qemu

KERNEL="kernel/arch/$ARCH/boot/Image"
INITRAMFS='initramfs.cpio'

$QEMU \
	-M virt -cpu cortex-a57 -smp 4 -m 1024 \
	-kernel $KERNEL \
	-initrd $INITRAMFS \
	-nographic \
	-append "console=ttyAMA0,115200 rdinit=/init"

