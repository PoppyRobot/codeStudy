#include <linux/init.h>
#include <linux/module.h>

static int hello_init(void)
{
	printk(KERN_INFO "Hello World enter.\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_INFO "Hello world exit.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("suse <azx12358@126.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple Hello World.");
MODULE_ALIAS("a simplest module");
