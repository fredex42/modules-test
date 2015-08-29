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

/*IRQ register pointers.  See p.112 of BCM2835 ARM Peripherals guide*/
uint32_t *irq_basic_pending=0xF200B200;
uint32_t *irq_pending_1=0xF200B204;
uint32_t *irq_pending_2=0xF200B208;
uint32_t *irq_fiq_control=0xF200B20C;
uint32_t *irq_enable_1=0xF200B210;
uint32_t *irq_enable_2=0xF200B214;
uint32_t *irq_enable_basic=0xF200B218;
uint32_t *irq_disable_1=0xF200B21C;
uint32_t *irq_disable_2=0xF200B220;
uint32_t *irq_disable_basic=0xF200B224;

/* GPIO register pointers. See p.90 of BCM2835 ARM Peripherals guide*/
/*function select*/
uint32_t *gpfsel0=0xF2200000;
uint32_t *gpfsel1=0xF2200004;
uint32_t *gpfsel2=0xF2200008;
uint32_t *gpfsel3=0xF220000C;
uint32_t *gpfsel4=0xF2200010;
uint32_t *gpfsel5=0xF2200004;
/*rising edge enable*/
uint32_t *gpren0=0xF220004C;
uint32_t *gpren1=0xF2200050;
/*falling edge enable*/
uint32_t *gpfen0=0xF2200058;
uint32_t *gpfen1=0xF220005C;
/*pullup/pulldown*/
uint32_t *gppud=0xF2200094;
uint32_t *gppudclk0=0xF2200098;
uint32_t *gppudclk1=0xF220009C;

/* work queue */
static struct workqueue_struct *my_workqueue;

static int irq_number=-1;

/* definitions */
#define IRQ_NUMBER	49
#define IRQ_LENGTH	4

#define WORK_QUEUE_NAME "bcm-gpitest2.c"

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

irqreturn_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    static int initialised = 0;
    static struct work_struct task;

    printk(KERN_INFO "Ping!\n");
    if(initialised == 0){
	INIT_WORK(&task,got_gpio);
	initialised = 1;
    } else {
	PREPARE_WORK(&task, got_gpio);
    }
    queue_work(my_workqueue, &task);

    return IRQ_HANDLED;
}

/*not sure whether this will work as we allow other processes to run
 *so, if anything else tries to access gpi hardware it could be a problem */
/* see http://www.xml.com/ldd/chapter/book/ch06.html */
void sleep_jit(int jiffies)
{
    wait_queue_head_t wait;
    printk(KERN_INFO "JIT delay\n");
    init_waitqueue_head(&wait);
    wait_event_interruptible_timeout(wait, 0, jiffies/HZ);
    printk(KERN_INFO "done.\n");
}

void _clock_pullup_all(void)
{    
    mb();
    printk(KERN_INFO "Clocking value to all gpis");
    /* clock to all pins */
    *gppudclk0 = 0xFFFFFFFF;
    *gppudclk1 = 0xFFFFFFFF;
    mb();
    
    sleep_jit(150);
    printk(KERN_INFO "Cleaning up\n");
    *gppud = 0;
    *gppudclk0 = 0;
    *gppudclk1 = 0;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait    
}

void enable_pullup_all(void)
{
    /*set pullup resistors*/
    mb();
    printk(KERN_INFO "Setting pull-up resistors");
    *gppud = 0x2;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    sleep_jit(150);
    /*clock the value to all pins*/
    _clock_pullup_all();
}

void disable_pullup_all(void)
{
    /*disable pullup resistors*/
    mb();
    printk(KERN_INFO "Disabling pull-up resistors");
    *gppud = 0x0;
    mb();
    /* wait 150 cycles */
    sleep_jit(150);
    _clock_pullup_all();
}

void _clock_specific(int line)
{
    /*tell system to clock to specific line*/
    printk(KERN_INFO "Clocking value into hardware");
    if(line<31){
        *gppudclk0 = (*gppudclk0) | (1 << line);
    } else {
        *gppudclk1 = (*gppudclk1) | (1 << (line-32));
    }
    mb();   //set a memory barrier to ensure all reads/writes committed before wait

    /*wait 150 cycles to take effect*/
    sleep_jit(150);
    
    printk(KERN_INFO "Cleaning up\n");
    *gppud = 0;
    if(line<31){
        *gppudclk0 = 0;
    } else {
        *gppudclk1 = 0;
    }
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
}

void enable_pullup(int line)
{
    /*set pullup resistors*/
    printk(KERN_INFO "Setting pull-up resistors");
    mb();
    *gppud = 0x2;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    sleep_jit(150);
    _clock_specific(line);
}

void disable_pullup(int line)
{
    /*set pullup resistors*/
    mb();
    printk(KERN_INFO "Disabling pull-up resistors");
    *gppud = 0x0;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    sleep_jit(150);
    _clock_specific(line);
}

void setup_input(void)
{
    uint32_t mask,val;
    /*see p.92 of BCM2835 ARM Peripherals guide*/
    /*set bottom 3 bits to 0 => pin 0 is an input */
    mb();
    printk(KERN_INFO "Setting pin 4 to input\n");
    mask = (uint32_t)(1 << 14) + (uint32_t)(1 << 13) + (uint32_t)(1 << 12);
    //val = ;
    printk(KERN_INFO "GPFSEL0 value before: 0x%x\n", *gpfsel0);
    (*gpfsel0) = (*gpfsel0) & (uint32_t)~mask;
    printk(KERN_INFO "GPFSEL0 value after: 0x%x\n", *gpfsel0);
    /*set falling edge detect, p.98*/
    printk(KERN_INFO "Setting up pin 4 to falling edge detect\n");
    *gpfen0 = (*gpfen0) | (1 << 4);
    mb(); //set a memory barrier to ensure all reads/writes committed
    enable_pullup(4);
}

void setup_interrupt(unsigned int irqnum)
{
    uint32_t mask,val;
    printk(KERN_INFO "Enabling interrupt in register\n");
    
    mb();
    
    if(irqnum<32){
	printk(KERN_INFO "Interrupt < 32 so using low register\n");
	val = readl(irq_enable_1);
	rmb();
	mask = 1 << irqnum;
	writel(val | mask,irq_enable_1);
	//*irq_enable_1 = (*irq_enable_1) | (1 << irqnum);
    } else if(irqnum<64){
	printk(KERN_INFO "Interrupt > 32 < 64 so using high register\n");
	irqnum = irqnum - 32;
	val = readl(irq_enable_2);
	rmb();
	printk(KERN_INFO "enable register value before 0x%x\n", val);
	mask = 1 << irqnum;
	writel(val|mask,irq_enable_2);
	//*irq_enable_2 = (*irq_enable_2) | (1 << irqnum);
	printk(KERN_INFO "enable register value after 0x%x\n", *irq_enable_2);
    } else {
	printk(KERN_ERR "Invalid interrupt number\n");
    }
    wmb(); //memory barrier to ensure all reads/writes committed
}

void disable_interrupt(unsigned int irqnum)
{
    uint32_t mask,val;
    printk(KERN_INFO "Disabling interrupt in register\n");
    
    mb();
    
    if(irqnum<32){
	printk(KERN_INFO "Interrupt < 32 so using low register\n");
	*irq_disable_1 = (1 << irqnum);
    } else if(irqnum<64){
	printk(KERN_INFO "Interrupt > 32 < 64 so using high register\n");
	irqnum = irqnum - 32;
	*irq_disable_2 = (1 << irqnum);
    } else {
	printk(KERN_ERR "Invalid interrupt number\n");
    }
    mb(); //memory barrier to ensure all reads/writes committed
}

static int __init startup(void)
{
    int n,c;
    /*struct driver_info *i;
    i=(struct driver_info *)kmalloc(sizeof(struct driver_info), GFP_KERNEL);
    */
    
    my_workqueue = create_workqueue(WORK_QUEUE_NAME);
    printk(KERN_INFO "Second test accessing bcm hardware\n");
    /* we have to go through the Linux GPIO driver, which provides dedicated software interrupts */
    irq_number=gpio_to_irq(4);
    printk(KERN_INFO "System HZ is %d",HZ);
    
    //for(c=IRQ_NUMBER;c<IRQ_NUMBER+1;++c){
	    n=request_irq(irq_number, &irq_handler, 0, "test gpio handler",&driver_info_block);
	    //n=1;
	    if(n!=0){
		printk(KERN_ERR "Unable to register interrupt handler: error %d\n",n);
		did_init=0;
	    } else {
		//setup_interrupt(IRQ_NUMBER);
		//enable_irq(irq_number);
		setup_input();
		did_init=1;
	    }
    //}	
    return 0;
}

static void __exit finish(void)
{
    if(did_init){
	//disable_interrupt(IRQ_NUMBER);
	disable_irq(irq_number);
	disable_pullup(4);
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
