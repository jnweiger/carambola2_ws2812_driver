/*  
 *  ws2812-draiveris.c - A bitbang kernel module for ws2812
 *
 *  Copyright (C) 2006-2012 OpenWrt.org
 *  Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
 *  Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
 *
 *  This is free software, licensed under the GNU General Public License v2.
 *  See /LICENSE for more information.
 * 
 * parameters: 
 *	gpio_number=20		# set base number. 
 *	inverted=0		# have inverting line drivers at the GPIOs
 */
#include <linux/init.h>
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/moduleparam.h>
#include <linux/stat.h>

//#include <linux/autoconf.h>
#include <linux/types.h>
//#include <linux/stddef.h>
#include <linux/string.h>
//#include <linux/i2c.h>
//#include <linux/i2c-algo-bit.h>
//#include <linux/fs.h>
//#include <asm/uaccess.h> /* for copy_from_user */

//#include <asm/mach-ath79/ar71xx_regs.h>
// cd /home/test/carambola2/build_dir/target-mips_r2_uClibc-0.9.33.2/linux-ar71xx_generic/linux-3.7.9/arch/mips/include/asm/mach-ath79/

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>  /* for put_user */
#include <linux/fs.h>

#define WS2812_MAJOR		152	/* use a LOCAL/EXPERIMENTAL major for now */

#define sysRegRead(phys) \
 (*(volatile u32 *)KSEG1ADDR(phys)) 

#define sysRegWrite(phys, val) \
 ((*(volatile u32 *)KSEG1ADDR(phys)) = (val)) 

// FROM http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf p65, p77
#define SYS_REG_GPIO_OE		0x18040000L
#define SYS_REG_GPIO_IN		0x18040004L
#define SYS_REG_GPIO_OUT	0x18040008L
#define SYS_REG_GPIO_SET	0x1804000CL
#define SYS_REG_GPIO_CLEAR	0x18040010L
#define SYS_REG_GPIO_INT	0x18040014L
#define SYS_REG_GPIO_INT_TYPE	0x18040018L
#define SYS_REG_GPIO_INT_POLARITY	0x1804001CL
#define SYS_REG_GPIO_INT_PENDING	0x18040020L
#define SYS_REG_GPIO_INT_MASK		0x18040024L
#define SYS_REG_GPIO_INT_FUNCTION_1	0x18040028L
#define SYS_REG_GPIO_IN_ETH_SWITCH_KED	0x1804002CL
#define SYS_REG_GPIO_INT_FUNCTION_2	0x18040030L

#define SYS_REG_RST_WATCHDOG_TIMER 0x1806000CL

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saulius Lukse saulius.lukse@gmail.com; Juergen Weigert juewei@fabfolk.com");
MODULE_DESCRIPTION("Bitbang GPIO driver for multiple WS2812 led chains");
MODULE_ALIAS_CHARDEV_MAJOR(WS2812_MAJOR);


static int gpio_number = 20; // default is nr 20
module_param(gpio_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_number, "GPIO number");

static int inverted = 0; // default is 0 == normal. 1 == inverted is good for 74HCT02 line drivers.
module_param(inverted, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(inverted, "drive inverted outputs");

#define SET_GPIOS_H(gpio_bits)	sysRegWrite(SYS_REG_GPIO_SET, gpio_bits)
#define SET_GPIOS_L(gpio_bits)	sysRegWrite(SYS_REG_GPIO_CLEAR, gpio_bits)

void led_bit(int bit)
{
  if (bit==0)
  {
    // high (3.3us)
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);

    // low (7us)
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
  }
  else
  {
    // high (7us)
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);

    // low (3.3us)
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
  }
}


void led_bit_inverted(int bit)
{
  if (bit==0)
  {
    // high (3.3us)
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);

    // low (7us)
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
  }
  else
  {
    // high (7us)
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);
    SET_GPIOS_L( 1<<gpio_number);

    // low (3.3us)
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
    SET_GPIOS_H( 1<<gpio_number);
  }
}



void update_leds(const char *buff, size_t len)
{

  unsigned long flags;
  long int i = 0;
  int b = 0;
  static int grb_increment[3] = { -1, 2, 2 };		// swap red and green: r g b -> g r b
  int grb_idx = 0;					// counting: 1 0 2  4 3 5  7 6 8  ...

  static DEFINE_SPINLOCK(critical);

  sysRegWrite(SYS_REG_RST_WATCHDOG_TIMER, 1<<31); // Just in case set watchdog to timeout some time later
  i = sysRegRead(SYS_REG_GPIO_OE);
  sysRegWrite(SYS_REG_GPIO_OE, i|1<<gpio_number); 	// output enable to PIN20

  if (inverted)
    {
      // wait 65uS
      for(i=0;i<1000;i++)
      {
	volatile int j = i;
	SET_GPIOS_H( 1<<gpio_number);
      }

      spin_lock_irqsave(&critical, flags);
      for (i = 1; i<len; i += grb_increment[grb_idx++])
      {
	for(b = 7; b>=0; b--)
	  if (buff[i] & (1 << b))
	    led_bit_inverted(1);
	  else
	    led_bit_inverted(0);
        if (grb_idx >= 3) grb_idx = 0;
      }
      spin_unlock_irqrestore(&critical, flags);
    }
  else	// not inverted
    {
      // wait 65uS
      for(i=0;i<1000;i++)
      {
	volatile int j = i;
	SET_GPIOS_L( 1<<gpio_number);
      }


      spin_lock_irqsave(&critical, flags);
      for (i = 1; i<len; i += grb_increment[grb_idx++])
      {
	for(b = 7; b>=0; b--)
	  if (buff[i] & (1 << b))
	    led_bit(1);
	  else
	    led_bit(0);
        if (grb_idx >= 3) grb_idx = 0;
      }
      spin_unlock_irqrestore(&critical, flags);
    }
}


/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "ws2812" /* Dev name as it appears in /proc/ws2812   */
#define BUF_LEN 900    /* Max length of the message from the device = 300 LED'S*/

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Device_Open = 0; /* Is device open?  
         * Used to prevent multiple access to device */
static char msg[BUF_LEN]; /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

// we must create *and remove* device classes properly, otherwise we get horrible 
// kernel stack backtraces.
static struct class *ws2812_class;

int init_module(void)
{ 
  int r;
  if ((r = register_chrdev(WS2812_MAJOR, DEVICE_NAME, &fops))) {
    printk(KERN_ALERT DEVICE_NAME ": Registering char device major=%d failed with %d\n", WS2812_MAJOR, r);
    return r;
  }

  printk(KERN_INFO DEVICE_NAME ": Build: %s %s\n", __DATE__, __TIME__);
  printk(KERN_INFO DEVICE_NAME ": Major=%d  device=/dev/%s\n", WS2812_MAJOR, DEVICE_NAME);
  printk(KERN_INFO DEVICE_NAME ": GPIO number: %d\n", gpio_number);


  ws2812_class = class_create(THIS_MODULE, DEVICE_NAME);
  if (!device_create(ws2812_class, NULL, MKDEV(WS2812_MAJOR, 0), NULL, DEVICE_NAME ))
    {
      printk(KERN_ALERT DEVICE_NAME": device_create failed. Try 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, WS2812_MAJOR);
      // return -ENXIO;		// better continue anyway...
    }

  return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{  
  device_destroy(ws2812_class, MKDEV(WS2812_MAJOR,0));
  class_destroy(ws2812_class);
  unregister_chrdev(WS2812_MAJOR, DEVICE_NAME);
  printk(KERN_ALERT DEVICE_NAME": Bye, that's all.\n");
}






/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
  static int counter = 0;

  if (Device_Open)
    return -EBUSY;

  Device_Open++;
  sprintf(msg, "I already told you %d times Hello world!\n", counter++);
  msg_Ptr = msg;
  try_module_get(THIS_MODULE);

  return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;    /* We're now ready for our next caller */

  /* 
   * Decrement the usage count, or else once you opened the file, you'll
   * never get get rid of the module. 
   */
  module_put(THIS_MODULE);

  return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
         char *buffer,  /* buffer to fill with data */
         size_t length, /* length of the buffer     */
         loff_t * offset)
{
  /*
   * Number of bytes actually written to the buffer 
   */
  int bytes_read = 0;

  /*
   * If we're at the end of the message, 
   * return 0 signifying end of file 
   */
  if (*msg_Ptr == 0)
    return 0;

  /* 
   * Actually put the data into the buffer 
   */
  while (length && *msg_Ptr) {

    /* 
     * The buffer is in the user data segment, not the kernel 
     * segment so "*" assignment won't work.  We have to use 
     * put_user which copies data from the kernel data segment to
     * the user data segment. 
     */
    put_user(*(msg_Ptr++), buffer++);

    length--;
    bytes_read++;
  }

  /* 
   * Most read functions return the number of bytes put into the buffer
   */
  return bytes_read;
}


static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
  update_leds(buff, len);
  //printk(KERN_INFO DEVICE_NAME": GOT: %s, %d\n", buff, len);
  return len;
}
