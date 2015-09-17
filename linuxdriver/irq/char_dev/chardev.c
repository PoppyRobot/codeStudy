#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/interrupt.h>
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h>  
#include <linux/irq.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  
#include <asm/io.h>  
#include <asm/signal.h>
#include <linux/poll.h>
#include <asm-generic/siginfo.h>
#include <linux/device.h>

static unsigned int g_count = 0;
static int irq_no = 1111;
unsigned int xxx_irq = 19;   /* 1: keyboard interrupt */

static struct class *fifthdrv_class;
static struct class_device	*fifthdrv_class_dev;

//volatile unsigned long *gpfcon;
//volatile unsigned long *gpfdat;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

/* 中断事件标志, 中断服务程序将它置1，fifth_drv_read将它清0 */
static volatile int ev_press = 0;

static struct fasync_struct *button_async;


struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};

/* interrupt process Top half */
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	g_count++;
	
	if (g_count % 100 == 0) {
		//发送信号SIGIO信号给fasync_struct 结构体所描述的PID，触发应用程序的SIGIO信号处理函数
		kill_fasync (&button_async, SIGIO, POLL_IN);
	}

    return IRQ_HANDLED;
}

static int fifth_drv_open(struct inode *inode, struct file *file)
{
	int ret = request_irq(xxx_irq, xxx_interrupt, /*IRQF_DISABLED*/IRQF_SHARED, "xxx", &irq_no);
    printk(KERN_INFO "request_irq return: %d\n", ret);
    if (ret == 0) {
        printk(KERN_INFO "request irq success.\n");
    }
	
	return 0;
}

ssize_t fifth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;
	
	return 1;
}


int fifth_drv_close(struct inode *inode, struct file *file)
{
	free_irq(xxx_irq, &irq_no);
    printk(KERN_INFO "xxx_exit.\n");
	return 0;
}

static int fifth_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: fifth_drv_fasync\n");
	//初始化/释放 fasync_struct 结构体 (fasync_struct->fa_file->f_owner->pid)
	return fasync_helper (fd, filp, on, &button_async);
}


static struct file_operations sencod_drv_fops = {
    .owner   =  THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open    =  fifth_drv_open,     
	.read	 =	fifth_drv_read,	   
	.release =  fifth_drv_close,
//	.poll    =  fifth_drv_poll,
	.fasync	 =  fifth_drv_fasync,
};


int major;
static int fifth_drv_init(void)
{
	major = register_chrdev(0, "fifth_drv", &sencod_drv_fops);

	fifthdrv_class = class_create(THIS_MODULE, "fifth_drv");

	fifthdrv_class_dev = device_create(fifthdrv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */

//	gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16);
//	gpfdat = gpfcon + 1;

	return 0;
}

static void fifth_drv_exit(void)
{
	unregister_chrdev(major, "fifth_drv");
	device_unregister(fifthdrv_class_dev);
	class_destroy(fifthdrv_class);
//	iounmap(gpfcon);
	return 0;
}


module_init(fifth_drv_init);
module_exit(fifth_drv_exit);
MODULE_LICENSE("GPL");

