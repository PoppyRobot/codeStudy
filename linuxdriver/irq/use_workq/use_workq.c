#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

unsigned int xxx_irq = 100;

/*Definei workq  and Bottom half function */

struct work_struct my_wq;
void xxx_do_func(unsigned long);

/* interrupt process Bottom half */
void xxx_do_work(unsigned long arg)
{
	printk(KERN_INFO "xxx_do_work.\n");
}

/* interrupt process Top half */
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	printk(KERN_INFO "work_schedule.\n");
	schedule_work(&my_wq);
}

int __init xxx_init(void)
{
	printk(KERN_INFO "xxx_init.\n");
	
	INIT_WORK(&my_wq, xxx_do_work);
	schedule_work(&my_wq);

	return 0;
#if 0
	/*request irq*/
	int ret = request_irq(xxx_irq, xxx_interrupt, IRQ_DISABLED, "xxx", NULL);
	printk(KERN_INFO "request_irq return: %d\n", ret);	
	if (ret == 0) {
		printk(KERN_INFO "request irq success.\n");	
	}

	return IRQ_HANDLED;
#endif
}

void __exit xxx_exit(void)
{
	/*free irq*/
//	free_irq(xxx_irq, NULL);
	printk(KERN_INFO "xxx_exit.\n");
}

module_init(xxx_init);
module_exit(xxx_exit);

MODULE_AUTHOR("suse <azx12358@126.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("How to use workq.");
MODULE_ALIAS("a simplest module");

