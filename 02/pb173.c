#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

int my_open(struct inode *node, struct file *filp)
{
	/* default my_read returning string length = strlen("Ahoj") = 4 */
	filp->private_data = (void *) 4;

	printk(KERN_INFO "/dev/mydev opened\n");
	return 0;
}

int my_release(struct inode *node, struct file *filp)
{
	printk(KERN_INFO "Mydevice closed\n");
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
		return -EIO;
}

#define BUFFER_SIZE 1024
ssize_t my_write(struct file *filp, const char __user *buff, size_t size, loff_t *off)
{	
	char buffer[BUFFER_SIZE + 1];
	size_t len = 0;
	int i;

	for(i = 0; i < size; i += BUFFER_SIZE) {
		len = (BUFFER_SIZE < size - i) ? BUFFER_SIZE : (size - i);
		if (copy_from_user(buffer, buff, len) != 0)
			return -EIO;
		buffer[len] = 0;
		printk(KERN_INFO "%s", buffer);
		}
	return size;
}

#define SET_LENGTH 1
/* 2 is prolly in conflict, didnt work before */
#define GET_CURRENT_LENGTH 0 

int my_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case SET_LENGTH:
			if (arg > 0 && arg <= 4)
				filp->private_data = (void *) arg;
			else
				return -EINVAL;
			break;
		case GET_CURRENT_LENGTH:
			if(put_user((size_t) filp->private_data, (int *) arg))
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
	.ioctl = my_ioctl
};

struct miscdevice my_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydev",
	.fops = &my_ops
};

static int my_init(void)
{
	misc_register(&my_misc);
	return 0;
}

static void my_exit(void)
{
	misc_deregister(&my_misc);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
