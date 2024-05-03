/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("asabbagh4"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;
struct mutex aesd_mutex;


int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    struct aesd_dev *device = NULL;
    device = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = device;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    
    ssize_t retval = 0;
    size_t read_bytes = 0;
    size_t offset_in_entry = 0;
    struct aesd_buffer_entry *buffer_entry = NULL;
    struct aesd_dev *device = NULL;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    if(filp == NULL || buf == NULL)
    {
        PDEBUG("Received invalid arguments");
        return -EINVAL;
    }

    device = filp->private_data;

    if(device == NULL)
    {
        PDEBUG("No device found");
        return -ENODEV;
    }

    if(mutex_lock_interruptible(&device->mutex_lock))
    {
        PDEBUG("Failed to lock mutex");
        return -ERESTARTSYS;
    }

    buffer_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&device->circular_buf, *f_pos, &offset_in_entry);

    if(buffer_entry == NULL)
    {
        retval = read_bytes;
        goto exit_read;
    }

    read_bytes = buffer_entry->size - offset_in_entry;

    if(read_bytes > count)
    {
        read_bytes = count;
    }

    retval = copy_to_user(buf, buffer_entry->buffptr + offset_in_entry, read_bytes);

    if(retval != 0)
    {
        PDEBUG("Failed to copy to user space");
        retval = -EFAULT;
        goto exit_read;
    }

    *f_pos += read_bytes;
    retval = read_bytes;

    exit_read:
    mutex_unlock(&device->mutex_lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *device = NULL;
    const char *buffer_to_free = NULL;
    size_t updated_entry_size = 0;

    if(filp == NULL || buf == NULL)
    {
        PDEBUG("Received invalid arguments");
        return -EINVAL;
    }
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    device = filp->private_data;

    if(device == NULL)
    {
        PDEBUG("No device found");
        return -ENODEV;
    }

    if(mutex_lock_interruptible(&device->mutex_lock))
    {
        PDEBUG("Failed to lock mutex");
        return -ERESTARTSYS;
    }

    updated_entry_size = (device->buf_entry.size + count);

    device->buf_entry.buffptr = krealloc(device->buf_entry.buffptr, updated_entry_size, GFP_KERNEL);
    if(device->buf_entry.buffptr == NULL)
    {
        PDEBUG("Failed to allocate %lu bytes of memory", updated_entry_size);
        retval = -ENOMEM;
        goto exit_write;
    }

    retval = copy_from_user((void *)(device->buf_entry.buffptr + device->buf_entry.size), buf, count);
    if(retval != 0)
    {
        PDEBUG("Failed to copy from user space");
        retval = -EFAULT;
        goto exit_write;
    }

    device->buf_entry.size += count;
    retval = count;

    if(device->buf_entry.buffptr[device->buf_entry.size - 1] == '\n')
    {
        buffer_to_free = aesd_circular_buffer_add_entry(&device->circular_buf, &device->buf_entry);

        if(buffer_to_free != NULL)
        {
            kfree(buffer_to_free);
            buffer_to_free = NULL;
        }

        device->buf_entry.buffptr = NULL;
        device->buf_entry.size = 0;
    }

    exit_write:
    mutex_unlock(&device->mutex_lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.mutex_lock);
    aesd_circular_buffer_init(&aesd_device.circular_buf);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    uint8_t i = 0;
    struct aesd_buffer_entry *entry = NULL;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);
    AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.circular_buf,i) {
        if(entry->buffptr != NULL)
        {
            kfree(entry->buffptr);
            entry->buffptr = NULL;
        }
    }
    mutex_destroy(&aesd_device.mutex_lock);
    unregister_chrdev_region(devno, 1);
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;
    printk(KERN_INFO "whence %d\n", whence);
    switch(whence) {
        case SEEK_SET: // Start from the beginning of the file
            newpos = off;
            break;

        case SEEK_CUR: // Start from the current file position
            newpos = filp->f_pos + off;
            break;

        case SEEK_END: // Start from the end of the file
            newpos = (aesd_device->buffer->total_size) + off;
            break;

        default: // Invalid whence
            return -EINVAL;
    }

    if (newpos < 0) return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    int command, offset;
    int retval = 0;
    loff_t newpos;

    switch(cmd) {
        case AESDCHAR_IOCSEEKTO:
            // Copy the command and offset from user space
            if (copy_from_user(&command, (int *)arg, sizeof(int)) ||
                copy_from_user(&offset, (int *)(arg + sizeof(int)), sizeof(int))) {
                retval = -EFAULT;
                break;
            }

            // Validate the command and offset
            if (command < 0 || command >= dev->circular_buf.count ||
                offset < 0 || offset >= dev->circular_buf.entries[command].size) {
                retval = -EINVAL;
                break;
            }

            // Calculate the new file position
            newpos = dev->circular_buf.entries[command].offset + offset;

            // Update the file position
            if (newpos < 0 || newpos >= (aesd_device->buffer->total_size)) {
                retval = -EINVAL;
            } else {
                filp->f_pos = newpos;
            }
            break;

        default:
            retval = -EINVAL;
    }

    return retval;
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);