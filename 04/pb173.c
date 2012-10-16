#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#define MODULE_NAME "mydev"

static struct dentry *dbgdir;
static struct dentry *dbgcount;
static struct dentry *dbgbin;

static __u8 count;

int my_open(struct inode *node, struct file *filp)
{
	/* default my_read returning string length = strlen("Ahoj") = 4 */
	filp->private_data = (void *) 4;
	
	count++;

	printk(KERN_INFO "module %s opened\n", MODULE_NAME);
	return 0;
}

int my_release(struct inode *node, struct file *filp)
{
	count--;

	printk(KERN_INFO "module %s closed\n", MODULE_NAME);
	return 0;
}

ssize_t my_read(struct file *filp, char __user *buff, size_t size, loff_t *off)
{
	/* filp->private data is in range from 1 to 4, therefore
	 * there is no need to check if size > strlen("Ahoj"). It 
	 * will be provided implicitly */
	size_t length = ( ((size_t) filp->private_data) > size ) ?
				size : (size_t) filp->private_data;

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
	
	actsize = SINGLE_WRITE_LENGTH > size ? size : SINGLE_WRITE_LENGTH;

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
				filp->private_data = (void *) arg;
			else
				return -EINVAL;
			break;
		case GET_CURRENT_LENGTH:
			if(put_user((unsigned long) filp->private_data, (unsigned long *) arg))
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
	
	dbgcount = debugfs_create_u8("count", 444, dbgdir, &count);
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

