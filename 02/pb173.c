#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

int my_open(struct inode *node, struct file *filp)
{
	printk(KERN_INFO "mydevice opend\n");
	return 0;
}

int my_release(struct inode *node, struct file *filp)
{
	printk(KERN_INFO "mydevice closed\n");
	return 0;
}

ssize_t my_read(struct file *filp, char __user *buf, size_t size, loff_t *off)
{
	if (copy_to_user(buf, "Ahoj", 4) == 0)
		return 0;
	else
		return -EFAULT;
}

ssize_t my_write(struct file *filp, const char __user *buf, size_t size, loff_t *off)
{
	return 0;
}

struct file_operations my_ops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release,
};

struct miscdevice my_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydev",
	.fops = &my_ops,
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
