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
	make -j$(nproc) -O all-gcc && make install-gcc

# --------------------------
# Clone LLVM (monorepo)
# --------------------------
WORKDIR /src

RUN git clone --depth=1 https://github.com/llvm/llvm-project.git

# --------------------------
# Build Clang + LLD + LLVM binutils tools
# --------------------------
WORKDIR /build

RUN cmake -G Ninja /src/llvm-project/llvm \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/opt/llvm \
	-DLLVM_ENABLE_PROJECTS="clang;lld" \
	-DLLVM_TARGETS_TO_BUILD="AArch64" \
	-DLLVM_ENABLE_BINUTILS_TOOLS=ON

RUN ninja -j$(nproc) && ninja install

# --------------------------
# Add LLVM tools to PATH
# --------------------------
ENV PATH="/opt/llvm/bin:${PATH}"

# Symlink toolchain binaries into /usr/local/bin for easier usage
RUN for bin in /opt/aarch64-none-elf/bin/*; do \
	ln -s "$bin" /usr/local/bin/$(basename "$bin"); \
done

WORKDIR /project

CMD ["/bin/bash"]

