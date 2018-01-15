/**
 * @file    capa110dio.c
 * @author  Michael Fraser <michael.fraser@calytrix.com>
 * @date    15 January 2015
 * @version 0.1
 * @brief   Linux kernel module that exposes access to the Digital I/O pins of
 *          the Axiomtek CAPA110 SBC motherboard. This module creates a
 *          character device at /dev/axdio which users can access to read and
 *          write the DIO pin state.
 */

#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define CAPA110_DIO_DEVICE 0x08

// This defines the pin configuration. A hi bit indicates that the pin will
// configured for input, whereas a lo bit indicates that the pin will be
// configured for output.
//
// The mask of 0x07 is the BIOS default DIO0-DIO2 Input, DIO3-DIO7 Output
//
// TODO: Make this a module parameter
#define PIN_MASK           0x07 

#define DEVICE_NAME        "axdio"
#define CLASS_NAME         "axdio"

#define CMD_UNLOCK_SUPERIO 0x87
#define CMD_SELECT_DEVICE  0x07
#define CMD_CONFIG_PINS    0xE0
#define CMD_IO             0xE1

#define PORT_CMD           0x2E
#define PORT_PARAM         0x2F

static int capa110dio_major = 0;
static struct class* capa110dio_class = NULL;
static struct device* capa110dio_device = NULL;

MODULE_AUTHOR( "Michael Fraser <michael.fraser@calytrix.com>" );
MODULE_DESCRIPTION( "Digital I/O access for Axiomtek CAPA110 motherboard" );
MODULE_LICENSE( "GPL" );
MODULE_VERSION( "0.1");

// Prototypes
static int     dev_open( struct inode*, struct file* );
static int     dev_release( struct inode*, struct file* );
static ssize_t dev_read( struct file*, char*, size_t, loff_t* );
static ssize_t dev_write( struct file*, const char*, size_t, loff_t* );

// Kernel Callbacks
static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

///////////////////////////////////////////////////////////////////////////////
//////////////////////////////  Private Methods  //////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void selectDioDevice(void)
{
	// Unlock Super I/O (must be issued twice)
	outb( CMD_UNLOCK_SUPERIO, PORT_CMD );
	outb( CMD_UNLOCK_SUPERIO, PORT_CMD );

	// Select Device
	outb( CMD_SELECT_DEVICE, PORT_CMD );
	outb( CAPA110_DIO_DEVICE, PORT_PARAM );
}

void configureDioPins(void)
{
	// ??
	outb( 0x30, PORT_CMD );
	outb( 0x02, PORT_PARAM );

	// Bits (Hi bits == read, lo bits == write)
	outb( CMD_CONFIG_PINS, PORT_CMD );
	outb( PIN_MASK, PORT_PARAM );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////  Kernel Module Methods  ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int __init capa110dio_init( void )
{
	printk( KERN_INFO "CAPA110DIO: Initializing DIO\n" );

	// Allocate a major number
	capa110dio_major = register_chrdev( 0, DEVICE_NAME, &fops );
	if( capa110dio_major < 0 )
	{
		printk( KERN_ALERT 
		        "CAPA110DIO: failed to register a major number\n" );
		return capa110dio_major;
	}
	printk( KERN_INFO 
	        "CAPA110DIO: registered with major number %d\n", 
	        capa110dio_major );

	// Register the device class
	capa110dio_class = class_create( THIS_MODULE, CLASS_NAME );
	if( IS_ERR(capa110dio_class) )
	{
		unregister_chrdev( capa110dio_major, DEVICE_NAME );
		printk( KERN_ALERT 
		        "CAPA110DIO: failed to register device class\n" );
		return PTR_ERR( capa110dio_class );
	}
	printk( KERN_INFO "CAPA110DIO: registered device class\n" );

	// Register the device driver
	capa110dio_device = device_create( capa110dio_class, 
	                                   NULL, 
	                                   MKDEV(capa110dio_major, 0), 
	                                   NULL, 
	                                   DEVICE_NAME );
	if( IS_ERR(capa110dio_device) )
	{
		class_destroy( capa110dio_class );
		unregister_chrdev( capa110dio_major, DEVICE_NAME );
		printk( KERN_ALERT "CAPA110DIO: Failed to create device\n" );
		return PTR_ERR( capa110dio_device );
	}
	printk( KERN_INFO "CAPA110DIO: created device\n" );

	selectDioDevice();
	configureDioPins();

	return 0;
}

static void __exit capa110dio_exit(void)
{
	device_destroy( capa110dio_class, MKDEV(capa110dio_major, 0) );
	class_unregister( capa110dio_class );
	class_destroy( capa110dio_class );
	unregister_chrdev( capa110dio_major, DEVICE_NAME );
	printk( KERN_INFO "CAPA110DIO: Device destroyed!\n" );
}

static int dev_open( struct inode* inodep, struct file* filep )
{
	try_module_get( THIS_MODULE );
	return 0;
}

static ssize_t dev_read( struct file* filep, 
                         char* buffer, 
                         size_t len, 
                         loff_t* offset )
{
	ssize_t result = 0;
	char val = 0;

	if( len > 0 )
	{
		selectDioDevice();

		// Issue I/O command and read into userspace buffer
		outb( CMD_IO, PORT_CMD );
		val = inb( PORT_PARAM );
		put_user( val, buffer );

		// One byte read
		result = 1;
	}

	return result;
}

static ssize_t dev_write( struct file* filep,
                          const char* buffer,
                          size_t len,
                          loff_t* offset )
{
	ssize_t writeLength = 0;
	selectDioDevice();

	// The user may have provided many bytes to write to DIO so write them
	// sequentially
	for( writeLength = 0 ; writeLength < len ; ++writeLength )
	{
		outb( CMD_IO, PORT_CMD );
		outb( buffer[writeLength], PORT_PARAM );
	}

	return writeLength;
}

static int dev_release( struct inode* inodep, struct file* filep )
{
	module_put( THIS_MODULE );
	return 0;
}

module_init( capa110dio_init );
module_exit( capa110dio_exit );

