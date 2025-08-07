# Author: Gustaf Franzen <gustaffranzen@icloud.com>

FROM debian:bookworm

LABEL maintainer="gustaffranzen@icloud.com"

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=aarch64-none-elf
ENV PREFIX=/opt/$TARGET
ENV LLVM_PREFIX=/opt/llvm
ENV PATH=$PREFIX/bin:$LLVM_PREFIX/bin:$PATH
ENV TOOLCHAIN_CACHE=/opt/toolchain-cache
ENV MAKEFLAGS=-j14

# -------------------------------------------------------
# Install dependencies
# -------------------------------------------------------
COPY packages.txt /tmp/
RUN apt-get update && \
	xargs -a /tmp/packages.txt apt-get install -y && \
	rm -rf /var/lib/apt/lists/*

# -------------------------------------------------------
# Ensure archive directory exists
# -------------------------------------------------------
COPY toolchain-cache $TOOLCHAIN_CACHE

# -------------------------------------------------------
# Build or extract GNU binutils
# -------------------------------------------------------
RUN echo "Checking Binutils archive..." && \
	if [ -f "$TOOLCHAIN_CACHE/binutils.tar.gz" ] && \
		tar -xzf "$TOOLCHAIN_CACHE/binutils.tar.gz" -C /opt; then \
		echo "Binutils extracted successfully."; \
	else \
		echo "Archive missing or corrupt. Building Binutils from source..."; \
		mkdir -p /build/binutils-src && cd /build && \
		aria2c -x 14 https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz && \
		tar -xf binutils-2.42.tar.xz && \
		mkdir binutils && cd binutils && \
		../binutils-2.42/configure \
			--target=$TARGET \
			--prefix=$PREFIX \
			--with-sysroot \
			--disable-nls \
			--disable-werror && \
		make $MAKEFLAGS && make install; \
	fi

RUN tar -czf "$TOOLCHAIN_CACHE/binutils.tar.gz" -C /opt $(basename $PREFIX)

# -------------------------------------------------------
# Build or extract GDB for $TARGET (aarch64-none-elf)
# -------------------------------------------------------
RUN echo "Checking GDB archive..." && \
	if [ -f "$TOOLCHAIN_CACHE/gdb.tar.gz" ] && \
		tar -xzf "$TOOLCHAIN_CACHE/gdb.tar.gz" -C /opt; then \
		echo "GDB extracted successfully."; \
	else \
		echo "Archive missing or corrupt. Building GDB from source..."; \
		mkdir -p /build/gdb-src && cd /build && \
		aria2c -x 14 https://ftp.gnu.org/gnu/gdb/gdb-14.2.tar.xz && \
		tar -xf gdb-14.2.tar.xz && \
		mkdir gdb && cd gdb && \
		../gdb-14.2/configure \
			--target=$TARGET \
			--prefix=$PREFIX \
			--disable-nls && \
		make $MAKEFLAGS && \
		make install; \
	fi

RUN tar -czf "$TOOLCHAIN_CACHE/gdb.tar.gz" -C /opt $(basename $PREFIX)

# -------------------------------------------------------
# Build or extract GCC stage1 (no libc)
# -------------------------------------------------------
RUN echo "Checking GCC stage1 archive..." && \
	if [ -f "$TOOLCHAIN_CACHE/gcc-stage1.tar.gz" ] && \
		tar -xzf "$TOOLCHAIN_CACHE/gcc-stage1.tar.gz" -C /opt; then \
		echo "GCC stage1 extracted successfully."; \
	else \
		echo "Archive missing or corrupt. Building GCC stage1 from source..."; \
		mkdir -p /build/gcc-src && cd /build && \
		aria2c -x 14 https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz && \
		tar -xf gcc-13.2.0.tar.xz && \
		cd gcc-13.2.0 && ./contrib/download_prerequisites && \
		mkdir /build/gcc && cd /build/gcc && \
		../gcc-13.2.0/configure \
			--target=$TARGET \
			--prefix=$PREFIX \
			--disable-nls \
			--enable-languages=c \
			--enable-gdb \
			--without-headers \
			--disable-shared \
			--disable-threads \
			--disable-libssp \
			--disable-libmudflap \
			--disable-libgomp \
			--disable-libquadmath \
			--disable-libatomic \
			--disable-libstdcxx && \
		make $MAKEFLAGS -O all-gcc && make install-gcc; \
	fi

RUN tar -czf "$TOOLCHAIN_CACHE/gcc-stage1.tar.gz" -C /opt $(basename $PREFIX);

# -------------------------------------------------------
# Clone LLVM (shallow)
# -------------------------------------------------------
WORKDIR /src
RUN git clone --depth=1 https://github.com/llvm/llvm-project.git

# -------------------------------------------------------
# Build or extract LLVM + LLD + LLVM binutils
# -------------------------------------------------------
WORKDIR /build
RUN echo "Checking LLVM archive..." && \
	if [ -f "$TOOLCHAIN_CACHE/llvm-toolchain.tar.gz" ] && \
		tar -xzf "$TOOLCHAIN_CACHE/llvm-toolchain.tar.gz" -C /opt; then \
		echo "LLVM archive extracted."; \
	else \
		echo "Archive missing or corrupt. Building LLVM from source..."; \
		cmake -G Ninja /src/llvm-project/llvm \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$LLVM_PREFIX \
			-DLLVM_ENABLE_PROJECTS="clang;lld" \
			-DLLVM_TARGETS_TO_BUILD="AArch64" \
			-DLLVM_ENABLE_BINUTILS_TOOLS=ON \
			-DLLVM_PARALLEL_LINK_JOBS=14 && \
		ninja && ninja install; \
	fi


RUN tar -czf "$TOOLCHAIN_CACHE/llvm-toolchain.tar.gz" -C /opt llvm;

# -------------------------------------------------------
# Symlink all tools into /usr/local/bin
# -------------------------------------------------------
RUN for bin in /opt/$TARGET/bin/* /opt/llvm/bin/*; do \
	[ -x "$bin" ] && ln -sf "$bin" /usr/local/bin/$(basename "$bin"); \
done

# -------------------------------------------------------
# Final setup
# -------------------------------------------------------
WORKDIR /project
CMD ["/bin/bash"]
