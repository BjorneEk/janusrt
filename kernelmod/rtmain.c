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

#include "psci.h"

#define DEVICE_NAME "rtcore"
#define RTCORE_IOCTL_START_CPU _IOW('r', 1, struct rtcore_start_args)

struct rtcore_start_args {
	uint64_t entry_phys;
	uint64_t core_id;
};

static dev_t dev_num;
static struct class *rtcore_class;
static struct cdev rtcore_cdev;
static void __iomem *jrt_mem_virt;

static long rtcore_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct rtcore_start_args args;
	int res;

	if (cmd != RTCORE_IOCTL_START_CPU) {
		pr_info("rtcore: cmd != RTCORE_IOCTL_START_CPU\n");
		return -ENOTTY;
	}

	if (copy_from_user(&args, (void __user *)arg, sizeof(args))) {

		pr_info("rtcore: failed to copy args from user\n");
		return -EFAULT;
	}



//	__flush_dcache_area(jrt_mem_virt, JRT_MEM_SIZE);
//	flush_icache_range((unsigned long)jrt_mem_virt,
//		(unsigned long)jrt_mem_virt + JRT_MEM_SIZE);

	pr_info("rtcore: starting CPU: %llu at 0x%llx\n", args.core_id, args.entry_phys);
	res = psci_cpu_on(args.core_id, args.entry_phys, 0);
	pr_info("rtcore: psci_cpu_on returned: %d\n", res);
	return 0;
}

static int rtcore_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size;
	int res;

	size = vma->vm_end - vma->vm_start;

	pr_info("rtcore: mmap request %lx bytes at user addr %lx\n", size, vma->vm_start);
	pr_info("rtcore: remapping phys %lx\n", (unsigned long)JRT_MEM_PHYS);

	if (size > JRT_MEM_SIZE) {
		pr_err("rtcore: mmap size too large\n");
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	res = remap_pfn_range(
		vma,
		vma->vm_start,
		JRT_MEM_PHYS >> PAGE_SHIFT,
		size,
		vma->vm_page_prot);

	if (res) {
		pr_err("rtcore: remap_pfn_range failed!\n");
		return -EAGAIN;
	}

	pr_info("rtcore: mmap successful\n");

	return 0;
}

static const struct file_operations rtcore_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rtcore_ioctl,
	.mmap = rtcore_mmap,
};

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
	pr_info("rtcore: registered with major %d\n", MAJOR(dev_num));
	pr_info("rtcore: module loaded\n");
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
