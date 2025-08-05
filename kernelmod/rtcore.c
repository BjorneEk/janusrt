/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/ioctl.h>

#include "mailbox.h"

#define DEVICE_NAME "rtcore"
#define CLASS_NAME "rtcore_class"

static dev_t devnum;
static struct class *rtcore_class;
static struct cdev rtcore_cdev;

static unsigned long *pen_release = ((volatile void*)0xFFFF00000900FFF8);

static long rtcore_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct rtcore_boot boot;

	if (cmd != RTCORE_BOOT)
		return -EINVAL;

	if (copy_from_user(&boot, (void __user*)arg, sizeof(boot)))
		return -EINVAL;

	if (boot.core_id >= nr_cpu_ids)
		return -EINVAL;

	pr_info("rtcore: booting core %u to 0x%llx\n", boot.core_id, boot.entrypoint);
	*pen_release = boot.entrypoint;
	smp_wmb();

	if (cpu_up(boot.core_id))
		return -EIO;

	return 0;
}

static const struct file_operations rtcore_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rtcore_ioctl,
};

static int __init rtcore_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&devnum, 0, 1, DEVICE_NAME);

	if (ret)
		return ret;

	cdev_init(&rtcore_cdev, &rtcore_fops);
	rtcore_cdev.owner = THIS_MODULE;
	ret = cdev_add(&rtcore_cdev, devnum, 1);

	if (ret)
		goto unreg;
	rtcore_class = class_create(CLASS_NAME);
//	rtcore_class = class_create(THIS_MODULE, CLASS_NAME);

	if (IS_ERR(rtcore_class)) {
		ret = PTR_ERR(rtcore_class);
		goto del;
	}

	device_create(rtcore_class, NULL, devnum, NULL, DEVICE_NAME);
	pr_info("rtcore: module loaded, major=%d\n", MAJOR(devnum));
	return 0;
del:
	cdev_del(&rtcore_cdev);
unreg:
	unregister_chrdev_region(devnum, 1);
	return ret;
}

static void __exit rtcore_exit(void)
{
	device_destroy(rtcore_class, devnum);
	class_destroy(rtcore_class);
	cdev_del(&rtcore_cdev);
	unregister_chdev_region(devnum, 1);
	pr_info("rtcore: module unloaded\n");
}

module_init(rtcore_init);
module_exit(rtcore_exit);
