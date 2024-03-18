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
 * @author Isha Sharma
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>

#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Isha Sharma"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;
    PDEBUG("open");

    // handle open
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; 
    PDEBUG("open complete");
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    // handle release
    filp->private_data = NULL; 
    PDEBUG("released");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    size_t  data_offset = 0;
    struct aesd_buffer_entry *Entry = NULL;
    ssize_t bytes_to_copy = 0;
    struct aesd_dev *aesd_device_ptr=NULL;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    //step1: check args
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read: invalid args\n");
        return -EINVAL;
    }

    //step2:  member of the buffer
    aesd_device_ptr= filp->private_data;

    //step3: mutex
    if(mutex_lock_interruptible(&aesd_device_ptr.lock) != 0)
	{
		PDEBUG("aesd_read: mutex lock failed \n");
		return -ERESTARTSYS; 
	}
    //step4: entry, offset the location = fpos
    Entry = aesd_circular_buffer_find_entry_offset_for_fpos(&aesd_device_ptr->buffer,*f_pos, &data_offset);

    //step5: copy to user space and return the no of bytes that couldnt be copied
    if(Entry)
    {
    	bytes_to_copy = (Entry->size - data_offset);
        //make the bytes_to_copy eual to the given count if its greater
        if (bytes_to_copy > count)
        {
            bytes_to_copy = count;
        }
        
        retval = copy_to_user(buf, (Entry->buffptr + data_offset), bytes_to_copy);
        if (retval != 0)
        {
            return -EFAULT;
        }
        else
        {
            retval = (bytes_to_copy - retval);
            *f_pos += retval;
        }
    }

    //step6: unlock mutex
    mutex_unlock(&aesd_device_ptr.lock);
    PDEBUG("aesd_read: sucess \n");
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM; 
	//initialise variable
    struct aesd_dev *aesd_device_ptr =NULL;  
    char * write_buffer = NULL;
    ssize_t bytes_read = 0;

	aesd_device_ptr = filp->private_data;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    //step1: check args
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read: invalid args\n");
        return -EINVAL;
    }

    //step2: kmalloc
    write_buffer = kmalloc(count, GFP_KERNEL);

    if(write_buffer == NULL)
    {
		PDEBUG("aesd_write: kmalloc failed \n");
        return -ENOMEM;
    }
    
    //step3: mutex lock
    if (mutex_lock_interruptible(&aesd_device_ptr->lock) != 0)
    {
		PDEBUG("aesd_write: mutex lock failed \n");
        kfree(write_buffer);
        return -ERESTARTSYS;
    }

    //step3: copy from user space to kernel
    retval = copy_from_user(write_data, buf, count),
    if (retval != 0)
	{
	    kfree(write_buffer);
        return -EFAULT;
	}

    //step4: find newline in buffer
    // if \n is there, write to buffer. if \n is not there, then continue writing till \n is found and then write to buffer.
    size_t newline_index = 0;
    ssize_t write_buffer_index;
    bool newline_present = FALSE;
    for (write_buffer_index=0; write_buffer_index<count; write_buffer_index++)
    {
        if(write_buffer[write_buffer_index] == '\n') 
        {
            newline_present = TRUE;
            break;
        }
        
    }

    if(newline_present == TRUE)
	{
	   newline_index = write_buffer_index + 1;      
	}
    else
        {
    	   newline_index = count;
        }
	            
    //step 5: reallocate buffer entry mem for the extra data in buffer, if not free and release everything
    aesd_device_ptr->entry.buffptr = krealloc ( aesd_device_ptr->entry.buffptr, aesd_device_ptr->entry.size + newline_index, GFP_KERNEL);
    if(aesd_device_ptr->entry.buffptr == NULL)
    {
        kfree(write_buffer);
        mutex_unlock(&char_dev->lock);
        return -ENOMEM;
    }

    //step6: copy from write buffer into ased char entry buffer
    memcpy(aesd_device_ptr->entry.buffptr + aesd_device_ptr->entry.size, write_buffer, newline_index);
    aesd_device_ptr->entry.size = aesd_device_ptr->entry.size + bytes_to_write;

    //step 8: add to actualy buffer entry 
    if (newline_present)
    {
        struct aesd_buffer_entry Entry;

        Entry.buffptr = aesd_device_ptr->entry.buffptr;
        Entry.size    = aesd_device_ptr->entry.size;

        aesd_circular_buffer_add_entry(&aesd_dev->buffer, &Entry);

        aesd_device_ptr->entry.buffptr = NULL;
        dev->entry.size = 0;
    }    

    //step 9: update the return value 
    retval = (newline_index);
 
    //step10: unlock mutex and clean
    mutex_unlock(&aesd_device_ptr->lock); 
    kfree(write_buffer);
    PDEBUG("aesd_write: sucess \n");  
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
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


    // initialize the AESD specific portion of the device

    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    int8_t ind = 0;
    struct aesd_buffer_entry *entry = NULL;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // cleanup AESD specific poritions here as necessary


    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, ind)
    {
        kfree(entry->buffptr);
        entry->buffptr = NULL;
    }
    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
