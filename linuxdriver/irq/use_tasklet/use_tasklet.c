#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>

static unsigned int g_count = 0;
static int irq_no = 1111;
unsigned int xxx_irq = 19;   /* 1: keyboard interrupt */

/*Define tasklet and Bottom half function */
struct tasklet_struct my_tasklet;
void xxx_do_tasklet(unsigned long);

DECLARE_TASKLET(my_tasklet, xxx_do_tasklet, 0);

/* interrupt process Bottom half */
void xxx_do_tasklet(unsigned long arg)
{
	if (g_count % 100 == 0) 
		printk(KERN_INFO "xxx_do_tasklet. g_count= %d\n", g_count);
}

/* interrupt process Top half */
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	g_count++;
//	printk(KERN_INFO "interrupt tasklet_schedule.\n");
//	tasklet_init(&my_tasklet, xxx_do_tasklet, 0);
	tasklet_schedule(&my_tasklet);
	
	return IRQ_HANDLED;
}

int __init xxx_init(void)
{
	int ret = 0;
	printk(KERN_INFO "xxx_init.\n");
	//tasklet_schedule(&my_tasklet);

	//return 0;
#if 1
	/*request irq*/
	ret = request_irq(xxx_irq, xxx_interrupt, /*IRQF_DISABLED*/IRQF_SHARED, "xxx", &irq_no);
	printk(KERN_INFO "request_irq return: %d\n", ret);	
	if (ret == 0) {
		printk(KERN_INFO "request irq success.\n");	
	}

	return 0;
#endif
}

void __exit xxx_exit(void)
{
	/*free irq*/
	free_irq(xxx_irq, &irq_no);
	printk(KERN_INFO "xxx_exit.\n");
}

module_init(xxx_init);
module_exit(xxx_exit);

MODULE_AUTHOR("suse <azx12358@126.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple Hello World.");
MODULE_ALIAS("a simplest module");

