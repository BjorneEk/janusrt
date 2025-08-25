/**
 *
 * loader.c - Loads a bare-metal program into reserved memory and starts a core using rtcore ioctl
 *
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "../shared/rtcore.h"

//#define RTCORE_IOCTL_START_CPU _IOW('r', 1, struct rtcore_start_args)

//struct rtcore_start_args {
//	uint64_t entry_phys;
//	uint64_t core_id;
//};

int main(int argc, char *argv[])
{
	int fd, fd_in;
	void *jrt_mem, *prog;
	struct stat st;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <rtprog.elf>\n", argv[0]);
		return 1;
	}

	fd_in = open(argv[1], O_RDONLY);
	if (fd_in < 0) {
		perror("open <program>.bin");
		return 1;
	}

	if (fstat(fd_in, &st) < 0) {
		perror("fstat");
		close(fd_in);
		return 1;
	}

	if (st.st_size > JRT_MEM_SIZE) {
		fprintf(stderr, "<program>.bin too big\n");
		close(fd_in);
		return 1;
	}

	fd = open("/dev/rtcore", O_RDWR);
	if (fd < 0) {
		perror("open /dev/rtcore");
		return 1;
	}

	jrt_mem = mmap(
		NULL,
		st.st_size * 2,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		0);

	if (jrt_mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	prog = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd_in, 0);
	if (prog == MAP_FAILED) {
		perror("mmap file");
		close(fd_in);
		return 1;
	}

	memcpy(jrt_mem, prog, st.st_size);
	munmap(prog, st.st_size);
	close(fd_in);



	struct rtcore_start_args args = {
		.entry_user = (uintptr_t)jrt_mem,
		.core_id = 3
	};

	printf("Starting CPU 3 at user address: 0x%lx...\n", args.entry_user);
	if (ioctl(fd, RTCORE_IOCTL_START_CPU, &args) < 0) {
		perror("ioctl cpu_on");
		return 1;
	}

	printf("Started CPU 3 at user address: 0x%lx\n", args.entry_user);
	struct rtcore_sched_args args2 = {
		.entry_user = (uintptr_t)jrt_mem,
	};
	for (int i = 0; i < 1000; ++i) {
		//sleep(1);
		if (ioctl(fd, RTCORE_IOCTL_SCHED_PROG, &args2) < 0) {
			perror("ioctl sched_prog");
			return 1;
		}
	}
	printf("DONE\n");

	return 0;
}
