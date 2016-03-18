#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <asm/io.h> //readl, writel etc.
#define DRIVER_AUTHOR "Andy Gallagher <andy.gallagher@theguardian.com>"
#define DRIVER_DESC "BCM gpio testing"
/* work queue */
static struct workqueue_struct *my_workqueue;
static int irq_number=-1;

/* definitions */
#define IRQ_NUMBER	49
#define IRQ_LENGTH	4

#define WORK_QUEUE_NAME "bcm-gpitest3.c"


char did_init=0;

struct driver_info {
    int junk;
};

static struct driver_info driver_info_block;

//SEE http://www.linuxforums.org/forum/kernel/183688-init_work-two-arguments.html for how to get data from work_struct param
static void got_gpio(struct work_struct *taskp)
{
    printk(KERN_INFO "Ping 2!\n");
}

static irqreturn_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    static int initialised = 0;
    static struct work_struct task;

    printk(KERN_INFO "Ping!\n");
/*    if(initialised == 0){
	INIT_WORK(&task,got_gpio);
	initialised = 1;
    } else {
	PREPARE_WORK(&task, got_gpio);
    }
    queue_work(my_workqueue, &task);
*/
    return IRQ_WAKE_THREAD;

 //   return IRQ_NONE;
}


static int __init startup(void)
{
    int n,c;
    /*struct driver_info *i;
    i=(struct driver_info *)kmalloc(sizeof(struct driver_info), GFP_KERNEL);
    */
    
    my_workqueue = create_workqueue(WORK_QUEUE_NAME);
    printk(KERN_INFO "Second test accessing bcm hardware\n");
    /* the gpio uses irqs 49->52 (decimal), p.113 */
    
    irq_number=gpio_to_irq(4);
    printk(KERN_INFO "Requesting irq %d",irq_number);
    //for(c=IRQ_NUMBER;c<IRQ_NUMBER+1;++c){
	    n=request_irq(irq_number, &irq_handler, 0, "test gpio handler",&driver_info_block);
	    //n=1;
	    if(n!=0){
		printk(KERN_ERR "Unable to register interrupt handler: error %d\n",n);
		did_init=0;
	    } else {
		//setup_interrupt(IRQ_NUMBER);
		enable_irq(irq_number);
		//setup_input();
		did_init=1;
	    }
    //}	
    return 0;
}

static void __exit finish(void)
{
    if(did_init){
	free_irq(irq_number, &driver_info_block);
	printk(KERN_INFO "Unloaded bcm hardware module\n");
    } else {
	printk(KERN_INFO "Driver did not initialise properly, so not unloading.\n");
    }
}

module_init(startup);
module_exit(finish);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
