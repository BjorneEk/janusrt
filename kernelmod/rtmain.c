/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */
#if defined(__clang__)

#endif

#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <asm/cacheflush.h>
#include <asm/sysreg.h>      /* for CTR_EL0 */
#include <linux/types.h>
#include <linux/atomic.h>

#include "psci.h"
#include "rtcore.h"

typedef struct rtcore_ctx {
	unsigned long user_base;	/* VMA start we mapped for this fd */
	size_t user_len;		/* bytes mapped for this fd */
	phys_addr_t phys_base;		/* page-aligned physical base */
	unsigned long first_off;	/* phys_start_unaligned & (PAGE_SIZE-1) */
	bool mapped;
} rtcore_ctx_t;

static int rtcore_open(struct inode *ino, struct file *filp);
static int rtcore_release(struct inode *ino, struct file *filp);
static long rtcore_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int rtcore_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations rtcore_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = rtcore_ioctl,
	.mmap           = rtcore_mmap,
	.open           = rtcore_open,
	.release        = rtcore_release,
};

static dev_t dev_num;
static struct class *rtcore_class;
static struct cdev rtcore_cdev;

static rtcore_mem_t __iomem *rtcore_mem;
static size_t G_MEM_OFF = 0;

static void __iomem *jrt_mem_virt;

static int rtcore_open(struct inode *ino, struct file *filp)
{
	rtcore_ctx_t *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	filp->private_data = ctx;

	return 0;
}

static int rtcore_release(struct inode *ino, struct file *filp)
{	kfree(filp->private_data);
	return 0;
}


static inline void clean_dcache_pou_range(unsigned long start, unsigned long end)
{
	u64 ctr;
	unsigned int dminline;
	unsigned long line;
	unsigned long addr;

	ctr = read_sysreg(CTR_EL0);
	dminline = (ctr >> 16) & 0xF;
	line = 4UL << dminline;

	start &= ~(line - 1);
	for (addr = start; addr < end; addr += line)
		asm volatile("dc cvau, %0" :: "r"(addr) : "memory");

	asm volatile("dsb ish" ::: "memory");
}

static inline void inval_icache_pou_range(unsigned long start, unsigned long end)
{
	u64 ctr;
	unsigned int iminline;
	unsigned long line;
	unsigned long addr;

	ctr = read_sysreg(CTR_EL0);
	iminline = ctr & 0xF;
	line = 4UL << iminline;

	start &= ~(line - 1);
	for (addr = start; addr < end; addr += line)
		asm volatile("ic ivau, %0" :: "r"(addr) : "memory");

	asm volatile("dsb ish" ::: "memory");
	asm volatile("isb");
}

static void rtcore_icache_sync_phys_range(phys_addr_t phys, size_t len)
{
	size_t off;
	unsigned long start, end;

	if (!len)
		return;

	off   = phys - JRT_MEM_PHYS;
	start = (unsigned long)((void *)((uintptr_t)jrt_mem_virt + off));
	end   = start + len;

	pr_info("rtcore: clear %lu bytes of icache from 0x%lx [0x%lx]\n",
		len,
		start,
		(unsigned long)phys);

//	dcache_clean_poc(start, len);
	clean_dcache_pou_range(start, end);
	inval_icache_pou_range(start, end);
//	flush_icache_range(start, end);
}

static long rtcore_start_cpu(struct file *file, unsigned long arg)
{
	rtcore_ctx_t *ctx;
	start_cpu_args_t args;
	phys_addr_t entry_phys;

	ctx = file->private_data;

	if (copy_from_user(&args, (void __user *)arg, sizeof(args)))
		return -EFAULT;

	if (!ctx || !ctx->mapped) {
		pr_err("rtcore: no mmap registered on this fd\n");
		return -EINVAL;
	}

	/* Bounds check: entry_user must lie inside the VMA we created */
	if (
		args.entry_user < ctx->user_base ||
		args.entry_user >= ctx->user_base + ctx->user_len) {
		pr_err(
			"rtcore: entry_user 0x%llx not "
			"within mapping [0x%lx..0x%lx)\n",
			args.entry_user,
			ctx->user_base,
			ctx->user_base + ctx->user_len);
		return -EFAULT;
	}

	/* VA -> PA: phys_base + first_off + (delta from user_base) */
	entry_phys = ctx->phys_base +
		(phys_addr_t)(args.entry_user - ctx->user_base);

	/* Optional: enforce 4-byte alignment */
	if (entry_phys & 0x3) {
		pr_err("rtcore: entry phys 0x%pa not 4-byte aligned\n", &entry_phys);
		return -EINVAL;
	}

	/* Optional: clamp to reserved window */
	if (	entry_phys < JRT_MEM_PHYS ||
		entry_phys >= JRT_MEM_PHYS + JRT_MEM_SIZE) {
		pr_err("rtcore: entry phys 0x%pa outside JRT region\n", &entry_phys);
		return -EINVAL;
	}

	pr_info("rtcore: starting CPU %llu at phys 0x%pa (VA 0x%llx)\n",
		args.core_id, &entry_phys, args.entry_user);

	/* If you’ve just copied code into this mapping, consider cache maintenance:
	 * __flush_dcache_area(kva, len); flush_icache_range(kva, kva+len);
	 * (You can compute 'kva' from jrt_mem_virt + (entry_phys - JRT_MEM_PHYS))
	 */
	rtcore_icache_sync_phys_range(
		entry_phys,
		ctx->user_len - (args.entry_user - ctx->user_base));
	return psci_cpu_on(args.core_id, entry_phys, 0);
}
static u32 mpsc_fetch_add_head_relaxed(u32 *p)
{
	u32		old;

	old = (u32)atomic_fetch_add_relaxed(1, (atomic_t *)p);

	return old;
}

static int mpsc_push(struct mpsc_ring *r,
	const struct tojrt_rec *rec,
	int try_only)	/* 0 = spin, 1 = return -EAGAIN */
{
	u32		t;
	u32		idx;
	u8		flag;

	t = mpsc_fetch_add_head_relaxed(&r->head);
	idx = t & TOJRT_MASK;

	/* wait until consumer cleared this slot (flags[idx] == 0) */
	if (try_only) {
		flag = smp_load_acquire(&r->flags[idx]);
		if (flag)
			return -EAGAIN;
	} else {
		for (;;) {
			flag = smp_load_acquire(&r->flags[idx]);
			if (!flag)
				break;
			cpu_relax();
		}
	}

	/* write payload first */
	memcpy(&r->data[idx], rec, sizeof(*rec));

	/* publish: set flag with release so consumer sees data */
	smp_store_release(&r->flags[idx], 1);

	return 0;
}

static long rtcore_sched_prog(struct file *file, unsigned long arg)
{
	rtcore_ctx_t *ctx;
	start_cpu_args_t args;
	phys_addr_t entry_phys;

	ctx = file->private_data;

	if (copy_from_user(&args, (void __user *)arg, sizeof(args)))
		return -EFAULT;

	if (!ctx || !ctx->mapped) {
		pr_err("rtcore: no mmap registered on this fd\n");
		return -EINVAL;
	}

	/* Bounds check: entry_user must lie inside the VMA we created */
	if (
		args.entry_user < ctx->user_base ||
		args.entry_user >= ctx->user_base + ctx->user_len) {
		pr_err(
			"rtcore: entry_user 0x%llx not "
			"within mapping [0x%lx..0x%lx)\n",
			args.entry_user,
			ctx->user_base,
			ctx->user_base + ctx->user_len);
		return -EFAULT;
	}

	/* VA -> PA: phys_base + first_off + (delta from user_base) */
	entry_phys = ctx->phys_base +
		(phys_addr_t)(args.entry_user - ctx->user_base);

	/* Optional: enforce 4-byte alignment */
	if (entry_phys & 0x3) {
		pr_err("rtcore: entry phys 0x%pa not 4-byte aligned\n", &entry_phys);
		return -EINVAL;
	}

	/* Optional: clamp to reserved window */
	if (	entry_phys < JRT_MEM_PHYS ||
		entry_phys >= JRT_MEM_PHYS + JRT_MEM_SIZE) {
		pr_err("rtcore: entry phys 0x%pa outside JRT region\n", &entry_phys);
		return -EINVAL;
	}

	//pr_info("rtcore: starting CPU %llu at phys 0x%pa (VA 0x%llx)\n",
	//	args.core_id, &entry_phys, args.entry_user);

	char msg[16];
	snprintf(msg, sizeof(msg), "ep:%llx", entry_phys);
	int r = mpsc_push(&rtcore_mem->ring, (struct tojrt_rec*)msg, 0);
	pr_info("rtcore: shed %i\n", r);
	return 0;
}

static long rtcore_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case RTCORE_IOCTL_START_CPU:
		return rtcore_start_cpu(file, arg);
	case RTCORE_IOCTL_SCHED_PROG:
		return rtcore_sched_prog(file, arg);
	default:
		return -ENOTTY;
	}
}

static int rtcore_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct rtcore_ctx *ctx;
	unsigned long size;
	unsigned long first_off, pfn;
	phys_addr_t phys_start_unaligned;
	phys_addr_t phys_base;
	size_t sz;

	ctx = filp->private_data;
	size = vma->vm_end - vma->vm_start;

	sz = G_MEM_OFF + size + sizeof(rtcore_mem_t);
	/* Sanity vs your reserved window usage */
	if (sz > JRT_MEM_SIZE) {
		pr_err("rtcore: mmap size too large (%lu bytes)\n", sz);
		return -EINVAL;
	}

	/* Compute the *unaligned* physical start we want to expose next */
	phys_start_unaligned = JRT_MEM_PHYS + sizeof(rtcore_mem_t) + G_MEM_OFF;

	/* Split it into page-aligned base + first-page offset */
	first_off = phys_start_unaligned & (PAGE_SIZE - 1);
	phys_base = phys_start_unaligned & PAGE_MASK;
	pfn       = phys_base >> PAGE_SHIFT;

	/* Map with requested protections
	 * (WB is preferable for code; drop noncached)
	 * If you truly need noncached,
	 * keep pgprot_noncached() — but exec will be slow.
	 * vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); */

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		pr_err("rtcore: remap_pfn_range failed!\n");
		return -EAGAIN;
	}

	/* Record mapping info for this fd so ioctl can translate VAs to PAs */
	ctx->user_base = vma->vm_start;
	ctx->user_len  = size;
	ctx->phys_base = phys_base;
	ctx->first_off = first_off;
	ctx->mapped    = true;

	/* Advance “allocation” cursor in your shared
	 * window by the exact bytes the user sees */
	G_MEM_OFF += size;

	pr_info(
		"rtcore: mmap %lx bytes at user %lx "
		"-> phys base 0x%pa (+off 0x%lx)\n",
		size,
		vma->vm_start,
		&phys_base,
		first_off);

	return 0;
}
static inline void ipc_init(struct mpsc_ring *r)
{
	smp_store_release((u32*)(&r->head), 0);
	smp_store_release((u32*)(&r->tail), 0);
	memset(r->flags, 0, TOJRT_SIZE);
}

static int __init rtcore_init(void)
{
	alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	cdev_init(&rtcore_cdev, &rtcore_fops);
	cdev_add(&rtcore_cdev, dev_num, 1);

	rtcore_class = class_create(DEVICE_NAME);
	device_create(rtcore_class, NULL, dev_num, NULL, DEVICE_NAME);

	jrt_mem_virt = memremap(JRT_MEM_PHYS, JRT_MEM_SIZE, MEMREMAP_WB);
	if (!jrt_mem_virt) {
		pr_err("rtcore: failed to map JRT memory\n");
		return -ENOMEM;
	}

	rtcore_mem = jrt_mem_virt;
	pr_info("rtcore: registered with major %d\n", MAJOR(dev_num));
	pr_info("rtcore: module loaded\n");
	ipc_init(&rtcore_mem->ring);
	pr_info("rtcore: initialized linux -> jrt ipc\n");
	return 0;
}

static void __exit rtcore_exit(void)
{
	memunmap(jrt_mem_virt);
	device_destroy(rtcore_class, dev_num);
	class_destroy(rtcore_class);
	cdev_del(&rtcore_cdev);
	unregister_chrdev_region(dev_num, 1);
	pr_info("rtcore: module unloaded\n");
}

module_init(rtcore_init);
module_exit(rtcore_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gustaf Franzen");
MODULE_DESCRIPTION("RTCore CPU manager and memory sharer");
