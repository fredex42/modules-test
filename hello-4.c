#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define DRIVER_AUTHOR "Andy Gallagher <andy.gallagher@theguardian.com>"
#define DRIVER_DESC "Hello world testing"

static int __init startup(void)
{
	printk(KERN_INFO "Hello world 4.\n");
	return 0;
}

static void __exit finish(void)
{
	printk(KERN_INFO "Goodbye world 4.\n");
}

module_init(startup);
module_exit(finish);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
/*
 * This module uses /dev/testdevice. The MODULE_SUPPORTED_DEVICE macro might
 * be used in the future to help automatic configuration of modules, but is
 * currently unused other than for documentation purposes.
 */
MODULE_SUPPORTED_DEVICE("testdevice");

