# Author: Gustaf Franzen <gustaffranzen@icloud.com>

FROM debian:bookworm

ENV TARGET=aarch64-none-elf
ENV PREFIX=/opt/$TARGET
ENV PATH=$PREFIX/bin:$PATH
ENV MAKEFLAGS=-j$(nproc)

COPY packages.txt /tmp/
RUN apt-get update && \
	xargs -a /tmp/packages.txt apt-get install -y \
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

