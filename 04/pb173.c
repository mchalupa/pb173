#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/atomic.h>
#include <linux/mutex.h>

#define MODULE_NAME "mydev"

static struct dentry *dbgdir;
static struct dentry *dbgcount;
static struct dentry *dbgbin;

static atomic_t count;
static int wopened = 0;

int my_open(struct inode *node, struct file *filp)
{
	DEFINE_MUTEX(open_mutex);
	atomic_t *rstrlen = NULL;
	
	mutex_lock(&open_mutex);
	
	if((filp->f_mode & FMODE_WRITE) != 0) {
		if(wopened) {
			mutex_unlock(&open_mutex);
			return -EBUSY;
		}
		else
			wopened = 1;
	}

	mutex_unlock(&open_mutex);

	rstrlen = kmalloc(sizeof(atomic_t), GFP_KERNEL);
	if (! rstrlen)
		return -ENOMEM;
	
	atomic_set(rstrlen, 4);

	filp->private_data = rstrlen;
	
	atomic_inc(&count);

	printk(KERN_INFO "module %s opened\n", MODULE_NAME);
	return 0;
}

int my_release(struct inode *node, struct file *filp)
{
	DEFINE_MUTEX(release_mutex);

	mutex_lock(&release_mutex);
	
	if((filp->f_mode & FMODE_WRITE) != 0) {
		if(wopened)
			wopened = 0;
		else
			printk(KERN_WARNING "WARNING: fmod_write with no wopened set in release!");
	}

	mutex_unlock(&release_mutex);
	
	atomic_dec(&count);

	kfree(filp->private_data);

	printk(KERN_INFO "module %s closed\n", MODULE_NAME);
	
	return 0;
}

ssize_t my_read(struct file *filp, char __user *buff, size_t size, loff_t *off)
{
	/* filp->private data is in range from 1 to 4, therefore
	 * there is no need to check if size > strlen("Ahoj"). It 
	 * will be provided implicitly */
	int set_length = atomic_read(filp->private_data);
	size_t length = (set_length > size) ?
				size : set_length;

	if (copy_to_user(buff, "Ahoj", length) == 0) {
		return length;
	}
	else
		return -EFAULT;
}

#define SINGLE_WRITE_LENGTH 1024
ssize_t my_write(struct file *filp, const char __user *buff, size_t size, loff_t *off)
{
	size_t actsize;
	char *buffer;

	if (*off >= size)
		return 0;
	
	actsize = SINGLE_WRITE_LENGTH > size ?
			size : SINGLE_WRITE_LENGTH;

	buffer = (char *) kmalloc((actsize + 1) * sizeof(char), GFP_KERNEL);

	if (buffer) {
		if(copy_from_user(buffer, buff + *off, actsize) != 0)
			return -EIO;
		
		/* ending zero */
		*(buffer + actsize) = 0;
		
		printk(KERN_INFO "%s", buffer);
		
		kfree(buffer);
		*off += actsize;

		return actsize;
	}
	else
		return -ENOMEM;
}

#define SET_LENGTH 1
/* 2 is prolly in conflict, didnt work before */
#define GET_CURRENT_LENGTH 0 

long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case SET_LENGTH:
			if (arg > 0 && arg <= 4)
				atomic_set(filp->private_data, arg);
			else
				return -EINVAL;
			break;
		case GET_CURRENT_LENGTH:
			if(put_user(atomic_read(filp->private_data), (unsigned long *) arg))
				return -EFAULT;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

struct file_operations my_ops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release,
	.unlocked_ioctl = my_ioctl
};

#define READ_SIZE 1024
ssize_t dbgbin_read(struct file *filp, char __user *buff, size_t size, loff_t *off)
{
	size_t actsize;

	if (*off >= THIS_MODULE->core_text_size)
		return 0;

	actsize = READ_SIZE > size ? size : READ_SIZE;

	if (copy_to_user(buff, THIS_MODULE->module_core + *off, actsize) != 0)
		return -EIO;
	
	*off += actsize;

	return actsize;
}

struct file_operations dbgfile = {
	.owner = THIS_MODULE,
	.read = dbgbin_read
};

struct miscdevice my_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODULE_NAME,
	.fops = &my_ops
};

static int my_init(void)
{
	misc_register(&my_misc);

	dbgdir = debugfs_create_dir(MODULE_NAME, NULL);
	if (! dbgdir)
		return -ENODEV;
	
	/* here it's kind of hardcore, but i have no other idea how to do it */
	dbgcount = debugfs_create_u32("count", 444, dbgdir, &(count.counter));
	if (! dbgcount) {
		debugfs_remove(dbgdir);
		return -ENODEV;
	}

	dbgbin = debugfs_create_file("text", 444, dbgdir, NULL, &dbgfile);
	if (! dbgbin) {
		debugfs_remove(dbgcount);
		debugfs_remove(dbgdir);
		return -ENODEV;
	}
	
	print_hex_dump_bytes("offset: ", DUMP_PREFIX_OFFSET, THIS_MODULE->module_core, THIS_MODULE->core_text_size);

	return 0;

}

static void my_exit(void)
{
	misc_deregister(&my_misc);

	debugfs_remove(dbgcount);
	debugfs_remove(dbgbin);
	debugfs_remove(dbgdir);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

