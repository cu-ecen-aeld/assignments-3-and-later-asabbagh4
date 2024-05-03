/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#include "aesd-circular-buffer.h"

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct aesd_dev
{
     struct mutex* mutex;
    
    /* Circular buffer */
     struct aesd_circular_buffer* buffer;
   
    /* Partial buffer */    
     struct aesd_buffer_entry* partial_entry;

    /* Partial buffer written bool */
     bool partial;

    /* Seek information */
     loff_t seekto_position;

    /* Seek bool */
     bool seek;

    /* Char device structure */
     struct cdev cdev;    
};

loff_t aesd_llseek(struct file *filp, loff_t off, int whence);

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
