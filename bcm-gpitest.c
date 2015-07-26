#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

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

/*not sure whether this will work as we allow other processes to run
 *so, if anything else tries to access gpi hardware it could be a problem */
/* see http://www.xml.com/ldd/chapter/book/ch06.html */
void sleep_jit(int jiffies)
{
    wait_queue_head_t wait;
    printk(KERN_INFO "JIT delay\n");
    init_waitqueue_head(&wait);
    wait_event_interruptible_timeout(wait, 0, jiffies);
    printk(KERN_INFO "done.\n");
}

void enable_pullup_all(void)
{
    /*set pullup resistors*/
    printk(KERN_INFO "Setting pull-up resistors");
    *gppud = 0x2;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    sleep_jit(150);
    
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

void enable_pullup(int line)
{
    /*set pullup resistors*/
    printk(KERN_INFO "Setting pull-up resistors");
    *gppud = 0x2;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    sleep_jit(150);
    
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

void setup_input(void)
{
    /*see p.92 of BCM2835 ARM Peripherals guide*/
    /*set bottom 3 bits to 0 => pin 0 is an input */
    printk(KERN_INFO "Setting pin 0 to input\n");
    *gpfsel0 = (*gpfsel0) & 0x1FFFFFF8;
    /*set falling edge detect, p.98*/
    printk(KERN_INFO "Setting up falling edge detect\n");
    *gpfen0 = (*gpfen0) | 0x1;
    mb(); //set a memory barrier to ensure all reads/writes committed
    enable_pullup_all();
}

static int __init startup(void)
{
	printk(KERN_INFO "First test accessing bcm hardware\n");
	setup_input();
	return 0;
}

static void __exit finish(void)
{
	printk(KERN_INFO "Unloaded bcm hardware module\n");
}

module_init(startup);
module_exit(finish);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);