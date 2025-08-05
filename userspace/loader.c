/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "mailbox.h"

#define RT_ADDR	(0x90000000UL)
#define RT_SIZE	(0x10000)
#define RTCORE_DEV	"/dev/rtcore"

int main(int argc, char *argv[])
{
	int fd_bin;
	void *mem;
	size_t n;
	struct rtcore_boot boot;
	int fd_rtcore;
	struct mailbox *mbox;

	if (argc < 2) {
		printf("usage %s <prog.bin>\n", argv[0]);
		return -1;
	}

	fd_bin = open(argv[1], O_RDONLY);

	if (fd_bin < 0) {
		perror("opening file");
		return 1;
	}

	mem = mmap(
		(void*)RT_ADDR,
		RT_SIZE,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
		-1,
		0);

	if (mem == MAP_FAILED) {
		perror("allocating memory");
		return -1;
	}

	if (mlock(mem, RT_SIZE) != 0) {
		perror("failed to lock memory region");
		return -1;
	}

	n = read(fd_bin, mem, RT_SIZE);

	if (n < 0) {
		perror("reading file");
		return -1;
	}
	close(fd_bin);

	boot = (struct rtcore_boot){
		.core_id = 3,
		.entrypoint = RT_ADDR,
	};

	fd_rtcore = open(RTCORE_DEV, O_RDWR);

	if (fd_rtcore < 0) {
		perror("open /dev/rtcore");
		return -1;
	}

	if (ioctl(fd_rtcore, RTCORE_BOOT, &boot) != 0) {
		perror("ioctl RTCORE_BOOT");
		return -1;
	}

	printf("Core %u started at 0x%lx press char to release memory\n", boot.core_id, boot.entrypoint);
	getchar();
	return 0;
	//mbox = (struct mailbox*)(RT_ADDR + RT_SIZE - sizeof(struct mailbox));
}
