/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include "psci.h"
#include <linux/export.h>
#include <linux/module.h>

/**
 * defined in psci_arm64.S
 */
extern
u64 psci_cpu_on_arm64(u64 core_id, u64 entry, u64 context);

u64 psci_cpu_on(u64 core_id, u64 entry, u64 context)
{
	return psci_cpu_on_arm64(core_id, entry, context);
}
EXPORT_SYMBOL(psci_cpu_on);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gustaf Franzen");
MODULE_DESCRIPTION("RTCore CPU manager and memory sharer");
