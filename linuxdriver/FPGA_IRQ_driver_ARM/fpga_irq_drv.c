/*
 * 监测FPGA产生的中断信号，中断处理函数产生异步通知
*/
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
#include<mach/gpio.h>
#include<mach/irqs.h>

static int irq_no = 1111;
unsigned int fpga_irq = 0;

static struct class *fpgadrv_class;
static struct class_device	*fpgadrv_class_dev;
static struct fasync_struct *fpga_async;

/* interrupt process Top half */
irqreturn_t fpga_interrupt(int irq, void *dev_id)
{
//	static unsigned int g_count = 0;
//	g_count++;
	
//	if (g_count % 50 == 0) {
		printk(KERN_INFO "fpga_interrupt.\n");
		//static unsigned long suspend_test_start_time;
		//suspend_test_start_time = jiffies;
		
		//printk("kill_fasync. %d\n", g_count);
		//发送信号SIGIO信号给fasync_struct 结构体所描述的PID，触发应用程序的SIGIO信号处理函数
		kill_fasync (&fpga_async, SIGIO, POLL_IN);
		//long nj = jiffies - suspend_test_start_time;
		//unsigned int msec = jiffies_to_msecs(abs(nj));
		//printk("Runing:nj=%ld %d.%03d seconds\n", nj, msec / 1000, msec % 1000); 
//	}
    return IRQ_HANDLED;
}

static int fpga_irq_drv_open(struct inode *inode, struct file *file)
{
	fpga_irq = gpio_to_irq(147);
	printk("fpga_irq= %d\n", fpga_irq);	
	//set_irq_type(fpga_irq, IRQF_TRIGGER_LOW);	
	int ret = request_irq(fpga_irq, fpga_interrupt,  IRQF_TRIGGER_RISING     /* IRQF_SHARED*/, "FPGA_SPI_IRQ", NULL/*&irq_no*/);
    
	printk(KERN_INFO "request_irq return: %d\n", ret);
    if (ret == 0) {
        printk(KERN_INFO "request irq success.\n");
    }
	return 0;
}

ssize_t fpga_irq_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;
	
	return 1;
}


int fpga_irq_drv_close(struct inode *inode, struct file *file)
{
	free_irq(fpga_irq, NULL/* &irq_no*/);
    printk(KERN_INFO "fpga_exit.\n");
	return 0;
}

static int fpga_irq_drv_fasync (int fd, struct file *filp, int on)
{
//	printk("driver: fpga_irq_drv_fasync\n");
	//初始化/释放 fasync_struct 结构体 (fasync_struct->fa_file->f_owner->pid)
	return fasync_helper (fd, filp, on, &fpga_async);
}


static struct file_operations sencod_drv_fops = {
    .owner   =  THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open    =  fpga_irq_drv_open,     
	.read	 =	fpga_irq_drv_read,	   
	.release =  fpga_irq_drv_close,
//	.poll    =  fpga_irq_drv_poll,
	.fasync	 =  fpga_irq_drv_fasync,
};


int major;
static int fpga_irq_drv_init(void)
{
	major = register_chrdev(0, "fpga_irq_drv", &sencod_drv_fops);
	fpgadrv_class = class_create(THIS_MODULE, "fpga_irq_drv");
	fpgadrv_class_dev = device_create(fpgadrv_class, NULL, MKDEV(major, 0), NULL, "fpgaIRQ"); /* /dev/fpgaIRQ */

#if 0	
	fpga_irq = gpio_to_irq(147);
	int ret = request_irq(fpga_irq, fpga_interrupt, IRQF_DISABLED/*IRQF_SHARED*/, "FPGA_SPI_IRQ", NULL/*&irq_no*/);
    
	printk("fpga_irq= %d\n", fpga_irq);	
	printk(KERN_INFO "request_irq return: %d\n", ret);
    if (ret == 0) {
        printk(KERN_INFO "request irq success.\n");
    }
#endif
	return 0;
}

static void fpga_irq_drv_exit(void)
{
	unregister_chrdev(major, "fpga_irq_drv");
	device_unregister(fpgadrv_class_dev);
	class_destroy(fpgadrv_class);
	return 0;
}

module_init(fpga_irq_drv_init);
module_exit(fpga_irq_drv_exit);
MODULE_LICENSE("GPL");

