/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include "psci.h"

/**
 * defined in psci_arm64.S
 */
extern
void psci_cpu_on_arm64(u64 core_id, u64 entry, u64 context);

void psci_cpu_on(u64 core_id, u64 entry, u64 context)
{
	psci_cpu_on_arm64(core_id, entry, context);
}
EXPORT_SYMBOL(psci_cpu_on);
