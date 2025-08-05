# Author: Gustaf Franzen <gustaffranzen@icloud.com>

FROM debian:bookworm

ENV TARGET=aarch64-none-elf
ENV PREFIX=/opt/$TARGET
ENV PATH=$PREFIX/bin:$PATH
ENV MAKEFLAGS=-j$(nproc)

RUN apt-get update && apt-get install -y \
	python3 \
	python3-pip \
	python3-setuptools \
	python3-wheel \
	python3-distutils \
	python3-lib2to3 \
	python3-dev \
	build-essential \
	bc \
	make \
	git \
	curl \
	wget \
	cpio \
	flex bison \
	libncurses-dev \
	libssl-dev \
	libelf-dev \
	busybox-static \
	gcc-aarch64-linux-gnu \
	binutils-aarch64-linux-gnu \
	crossbuild-essential-arm64 \
	gcc \
	g++ \
	texinfo \
	gawk \
	gperf \
	patch \
	xz-utils \
	bzip2 \
	unzip \
	autoconf \
	automake \
	libtool \
	libgmp-dev \
	libmpfr-dev \
	libmpc-dev \
	zlib1g-dev \
	&& rm -rf /var/lib/apt/lists/*

#RUN curl -LO https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf.tar.xz && \
#	tar -xf arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf.tar.xz &&\
#	mv arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf /opt/arm-toolchain && \
#	ln -s /opt/arm-toolchain/bin/aarch64-none-elf-gcc /usr/bin/aarch64-none-elf-gcc && \
#	ln -s /opt/arm-toolchain/bin/aarch64-none-elf-ld /usr/bin/aarch64-none-elf-ld && \
#	ln -s /opt/arm-toolchain/bin/aarch64-none-elf-objcopy /usr/bin/aarch64-none-elf-objcopy && \
#	rm arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf.tar.xz
# Build binutils
RUN mkdir -p /build/binutils && cd /build && \
	curl -LO https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz && \
	tar -xf binutils-2.42.tar.xz && \
	cd /build/binutils && \
	../binutils-2.42/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror && \
	make && make install
RUN mkdir -p /build/gcc && cd /build && \
	curl -LO https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz && \
	tar -xf gcc-13.2.0.tar.xz && \
	cd gcc-13.2.0 && ./contrib/download_prerequisites && \
	mkdir /build/gcc-build && cd /build/gcc-build && \
	../gcc-13.2.0/configure --target=aarch64-none-elf \
		--prefix=/opt/aarch64-none-elf \
		--disable-nls --enable-languages=c --without-headers \
		--disable-shared --disable-threads \
		--disable-libssp --disable-libmudflap --disable-libgomp \
		--disable-libquadmath --disable-libatomic --disable-libstdcxx && \
	make -j1 all-gcc && make install-gcc
# Build GCC (only C compiler, no libgcc, no stdlib)
#RUN mkdir -p /build/gcc && cd /build && \
#	curl -LO https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz && \
#	tar -xf gcc-13.2.0.tar.xz && \
#	cd gcc-13.2.0 && ./contrib/download_prerequisites && \
#	cd /build/gcc && \
#	../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX \
#	--disable-nls --enable-languages=c --without-headers --disable-shared \
#	--disable-threads --disable-libssp --disable-libmudflap --disable-libgomp \
#	--disable-libquadmath --disable-libatomic --disable-libstdcxx && \
#	make all-gcc && make install-gcc

# Symlink toolchain binaries into /usr/local/bin for easier usage
RUN for bin in /opt/aarch64-none-elf/bin/*; do \
	ln -s "$bin" /usr/local/bin/$(basename "$bin"); \
done

WORKDIR /project

CMD ["/bin/bash"]

