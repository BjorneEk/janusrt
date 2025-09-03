
// Author: Gustaf Franzen <gustaffranzen@icloud.com>

// FILE: mmu.h
//typedef struct {
//	u64 *l0;
//	u64  l0_pa;
//} pt_root_t;
	.equ PT_ROOT_L0,	0
	.equ PT_ROOT_L0_PA,	(PT_ROOT_L0 + 8)
	.equ PT_ROOT_SIZE,	(PT_ROOT_L0_PA + 8) //16

// FILE: mmu.h
//typedef struct {
//	pt_root_t root;
//	u16  asid;
//} mmu_map_t;
	.equ MMAP_ROOT,		0
	.equ MMAP_ASID, 	(MMAP_ROOT + PT_ROOT_SIZE)	//16
	.equ MMAP_PAD,		(MMAP_ASID + 2)			//18
	.equ MMAP_SIZE,		(MMAP_PAD + 6)			//24

// FILE: sched.h
//typedef JRT_PACKED struct ctx {
//	u64	x[31];
//	u64	sp;
//	u64	pc;
//	u64	pstate;
//
//	u128 vregs[32];
//	u32 fpsr, fpcr;
//	mmu_map_t mmap;
//} ctx_t;
	.equ CTX_X,		0
	.equ CTX_SP,		(31*8)			//248
	.equ CTX_PC,		(CTX_SP + 8)		//256
	.equ CTX_PSTATE,	(CTX_PC + 8)		//264
	.equ CTX_VREGS,		(CTX_PSTATE + 8)	//272
	.equ CTX_FPSR,		(CTX_VREGS + 32*16)	//784
	.equ CTX_FPCR,		(CTX_FPSR + 4)		//788
	.equ CTX_MMAP,		(CTX_FPCR + 4)		//792
	.equ CTX_SIZE,		(CTX_MMAP + MMAP_SIZE)	//816

// FILE: sched.h
//typedef struct process {
//	u32	pid;
//	state_t	state;
//	int first;
//
//	/* scheduling */
//	u64	wait_until;      /* how long to wait */
//	u64	abs_deadline;   /* base requested deadline */
//	u64	eff_deadline;   /* effective (after inheritance) */
//	u64	wait_start_ns;  /* for fairness */
//	u64	wait_seq;       /* tiebreaker */
//
//	ctx_t ctx;
//
//	void *mem;
//	size_t mem_size;
//
//} proc_t;
	.equ PROC_CTX,		0
	.equ PROC_SIZE,		(PROC_CTX + CTX_SIZE + 80)

// FILE: sched.h
//typedef struct sched {
//	proc_t p0; //kernel proc
//	proc_t *curr;
//	proc_t pb[MAX_PROC];
//	proc_t *free_proc[MAX_PROC];
//	size_t nfree_proc;
//
//	heap_node_t rn[READY_MAX];
//	minheap_t ready;
//	u32 pid;
//} sched_t;
	.equ SCHED_P0,		0
	.equ SCHED_CURR,	(SCHED_P0 + PROC_SIZE)
	.equ SCHED_PID,		(SCHED_CURR + 8)

	.equ SAVE_FP, 0

	.extern G_SCHED
	.extern G_KERNEL_CTX


