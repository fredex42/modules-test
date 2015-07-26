#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

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
uint32_t *gpren0=0xF220004C;
/*falling edge enable*/
uint32_t *gpfen0=0xF2200058;
uint32_t *gpfen1=0xF220005C;
/*pullup/pulldown*/
uint32_t *gppud=0xF2200094;
uint32_t *gppudclk0=0xF2200098;
uint32_t *gppudclk1=0xF220009C;

void enable_pullup(int line)
{
    /*set pullup resistors*/
    *gppud = 0x2;
    mb();   //set a memory barrier to ensure all reads/writes committed before wait
    
    /*wait 150 cycles to stabilise*/
    
    /*tell system to clock to specific line*/
    if(line<31){
        *gppudclk0 = (*gppudclk0) & (1 << line);
    } else {
        *gppudclk1 = (*gppudclk1) & (1 << (line-32));
    }
    mb();   //set a memory barrier to ensure all reads/writes committed before wait

    /*wait 150 cycles to take effect*/
    
    *gppud = 0;
    if(line<31){
        *gppudclk0 = 0;
    } else {
        *gppudclk1 = 0;
    }
    mb();   //set a memory barrier to ensure all reads/writes committed before wait

}
void setup_input()
{
    /*see p.92 of BCM2835 ARM Peripherals guide*/
    /*set bottom 3 bits to 0 => pin 0 is an input */
    *gpfsel0 = (*gpfsel0) & 0x1FFFFFF8;
    /*set falling edge detect, p.98*/
    *gpfen0 = (*gpfen0) | 0x1;
    mb(); //set a memory barrier to ensure all reads/writes committed
    enable_pullup();
}

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