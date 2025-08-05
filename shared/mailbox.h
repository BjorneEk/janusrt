/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#define RTCORE_MAGIC (0xB0)
#define RTCORE_BOOT _IOW(RTCORE_MAGIC, 0x01, struct rtcore_boot)

struct rtcore_boot {
	uint32_t core_id;
	uint32_t entrypoint;
};

struct mailbox {
	volatile uint32_t counter;
	volatile uint32_t last_value;
};

#define MAILBOX_PHYS_ADDR 0xA0000000

#endif
