
// Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include "uart.h"
#include "cpu.h"
#include "syscall.h"

/* forward */
static const char *ec_str(u64 ec);
static const char *dfsc_str(u32 dfsc);
static const char *ifsc_str(u32 ifsc);

static void print_hex64(u64 v)
{
	int i;
	u32 nib;

	for (i = 60; i >= 0; i -= 4) {
		nib = (v >> i) & 0xFULL;
		uart_putc(nib < 10 ? ('0' + nib) : ('a' + (nib - 10)));
	}
}

static void print_u32(u32 v)
{
	char buf[11];
	int i;
	u32 t;

	for (i = 0; i < 10; i++)
		buf[i] = 0;
	buf[10] = 0;

	i = 9;
	do {
		t = v % 10u;
		buf[i--] = '0' + (char)t;
		v = v / 10u;
	} while (v && i >= 0);

	uart_puts(&buf[i + 1]);
}

static void print_ec_line(u64 ec, u64 il)
{
	uart_puts("EC=0x");
	print_hex64(ec);
	uart_puts(" (");
	uart_puts(ec_str((u32)ec));
	uart_puts(") IL=");
	print_u32((u32)il);
	uart_puts("\n");
}

static void decode_abort(u64 esr, u64 far, int instr_abort)
{
	u32 iss;
//	u32 isv;
	u32 wnr;
	u32 s1ptw;
	u32 ea;
	u32 fnv;
	u32 fsc;

	iss = (u32)(esr & 0x01FFFFFFu);
//	isv = (iss >> 24) & 1u;		/* valid for data aborts only */
	wnr = (iss >> 6) & 1u;
	s1ptw = (iss >> 7) & 1u;
	ea = (iss >> 9) & 1u;
	fnv = (iss >> 10) & 1u;

	/* FSC: IFSC for instruction aborts, DFSC for data aborts */
	fsc = iss & 0x3Fu;

	uart_puts(instr_abort ? "IABT " : "DABT ");
	uart_puts("WnR="); print_u32(wnr);
	uart_puts(" S1PTW="); print_u32(s1ptw);
	uart_puts(" EA="); print_u32(ea);
	uart_puts(" FnV="); print_u32(fnv);
	uart_puts(" FSC=0x"); print_hex64(fsc);
	uart_puts(" (");
	uart_puts(instr_abort ? ifsc_str(fsc) : dfsc_str(fsc));
	uart_puts(")\n");

	/* FAR_EL1 meaningful if FnV==0 for aborts (except some cases) */
	uart_puts("FAR=0x"); print_hex64(far);
	uart_puts("\n");
}

const char *ec_str(u64 ec)
{
	switch ((u32)ec) {
	case 0x00: return "Unknown/UNDEF";
	case 0x15: return "SVC (AArch64)";
	case 0x20: return "Instruction Abort, lower EL";
	case 0x21: return "Instruction Abort, same EL";
	case 0x22: return "PC alignment fault";
	case 0x24: return "Data Abort, lower EL";
	case 0x25: return "Data Abort, same EL";
	case 0x26: return "SP alignment fault";
	case 0x28: return "FP/ASIMD";
	case 0x2C: return "SError (system error)";
	case 0x30: return "Breakpoint, lower EL";
	case 0x31: return "Breakpoint, same EL";
	case 0x32: return "Vector catch (AArch32)";
	case 0x33: return "Watchpoint, lower EL";
	case 0x34: return "Watchpoint, same EL";
	case 0x35: return "BKPT (AArch32)";
	case 0x38: return "Software Step, lower EL";
	case 0x39: return "Software Step, same EL";
	case 0x3C: return "BRK (AArch64)";
	default:   return "EC(?)";
	}
}

const char *dfsc_str(u32 fsc)
{
	/* Data Fault Status Code (bits[5:0]) */
	switch (fsc) {
	case 0x00: return "Addr size fault L0";
	case 0x01: return "Addr size fault L1";
	case 0x02: return "Addr size fault L2";
	case 0x03: return "Addr size fault L3";
	case 0x04: return "Translation fault L0";
	case 0x05: return "Translation fault L1";
	case 0x06: return "Translation fault L2";
	case 0x07: return "Translation fault L3";
	case 0x08: return "Access flag fault L0";
	case 0x09: return "Access flag fault L1";
	case 0x0A: return "Access flag fault L2";
	case 0x0B: return "Access flag fault L3";
	case 0x0C: return "Permission fault L0";
	case 0x0D: return "Permission fault L1";
	case 0x0E: return "Permission fault L2";
	case 0x0F: return "Permission fault L3";
	case 0x10: return "Sync ext abort (nTWB)";
	case 0x11: return "Async ext abort";
	case 0x12: return "Sync ext abort (TWB)";
	case 0x14: return "Sync parity/ECC";
	case 0x15: return "Async parity/ECC";
	case 0x18: return "Sync parity (TWB)";
	case 0x1C: return "TF check";
	case 0x21: return "Alignment fault";
	case 0x22: return "Debug event";
	default:   return "DFSC(?)";
	}
}

const char *ifsc_str(u32 fsc)
{
	switch (fsc) {
	case 0x00: return "Addr size fault L0";
	case 0x01: return "Addr size fault L1";
	case 0x02: return "Addr size fault L2";
	case 0x03: return "Addr size fault L3";
	case 0x04: return "Translation fault L0";
	case 0x05: return "Translation fault L1";
	case 0x06: return "Translation fault L2";
	case 0x07: return "Translation fault L3";
	case 0x0C: return "Permission fault L0";
	case 0x0D: return "Permission fault L1";
	case 0x0E: return "Permission fault L2";
	case 0x0F: return "Permission fault L3";
	case 0x11: return "Async ext abort";
	case 0x14: return "Sync parity/ECC";
	case 0x1C: return "TF check";
	default:   return "IFSC(?)";
	}
}
static void norecover(enum panic_reason r)
{
	halt();
}

sync_recover_t g_sync_recover = norecover;

void sync_set_recover_handler(sync_recover_t recover)
{
	g_sync_recover = recover;
}
#define ESR_EC(esr)        (((esr) >> 26) & 0x3F)
#define ESR_IL(esr)        (((esr) >> 25) & 0x1)
#define ESR_ISS(esr)       ((esr) & 0x01FFFFFF)
#define SVC_IMM16(esr)     ((u16)ESR_ISS(esr))
/* C entry called from EL1h Sync vector */
void sync_exception_entry(u64 esr, u64 elr, u64 far)
{
	u64 ec;
	u64 il;
	u64 iss;
	u32 imm16;

	ec = ESR_EC(esr);
	il = ESR_IL(esr);
	iss =  esr & 0x01ffffff;

	if ((u32)ec != 0x15) {
		uart_puts("[SYNC] ESR=0x"); print_hex64(esr);
		uart_puts(" ELR=0x"); print_hex64(elr);
		uart_puts(" FAR=0x"); print_hex64(far);
		uart_puts("\n");

		print_ec_line(ec, il);
	}

	switch ((u32)ec) {
	case 0x15:
		uart_puts("syscall\n");
		take_syscall(SVC_IMM16(esr));
		return;
	case 0x20:	/* IABT, lower EL */
	case 0x21:	/* IABT, same EL */
		decode_abort(esr, far, 1);
		break;
	case 0x24:	/* DABT, lower EL */
	case 0x25:	/* DABT, same EL */
		decode_abort(esr, far, 0);
		break;
	case 0x2C:	/* SError reported as sync (rare) */
		uart_puts("SError reported synchronously\n");
		break;
	case 0x3C:
		imm16 = iss & 0xFFFF;
		uart_puts("[BRK] type=");
		uart_puts(panic_str(imm16));
		uart_puts(" ELR=0x"); print_hex64(elr);
		uart_puts("\n");
		g_sync_recover(imm16);
		return;
	default:
		uart_puts("Unhandled SYNC class\n");
		break;
	}

	uart_puts("Halting.\n");
	halt();
}

