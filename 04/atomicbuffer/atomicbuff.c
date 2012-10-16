#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
#define BUFFER_SIZE 128
#define WRITE_MAX 5

static char buffer[BUFFER_SIZE];

DEFINE_MUTEX(buffer_busy);

/** reading device **/
ssize_t read(struct file *filp, char __user *ubuff, size_t count, loff_t *off)
{
	size_t act;
	
	if (*off >= BUFFER_SIZE || !count)
		return 0;	/* EOF */
	
	act = BUFFER_SIZE > count ? count : BUFFER_SIZE;

	mutex_lock(&buffer_busy);

	if(copy_to_user(ubuff, buffer + *off, act) != 0) {
		mutex_unlock(&buffer_busy);
		return -EIO;
		}
	
	*off += act;

	mutex_unlock(&buffer_busy);

	return act;
}

static struct file_operations r_ops = {
	.owner = THIS_MODULE,
	.read = read,
};

static struct miscdevice r_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "atomicbuff_r",
	.fops = &r_ops
};

/** writing device **/
ssize_t write(struct file *filp, const char __user *ubuff, size_t count, loff_t *off)
{
	size_t act;
	int i;

	if(*off >= BUFFER_SIZE)
		*off = 0; /* cycle buffer */

	act = WRITE_MAX > count ? count : WRITE_MAX;
	if (act + *off > BUFFER_SIZE) /* overflow */
		act = BUFFER_SIZE - *off;

	mutex_lock(&buffer_busy);

	for(i = 0; i < act; i++) {
		if (get_user(buffer[*off], ubuff + i)) {
			mutex_unlock(&buffer_busy);
			return i;
		}

	msleep(20);
	
	(*off)++;
	}

	mutex_unlock(&buffer_busy);
	
	return act;
}

static struct file_operations w_ops = {
	.owner = THIS_MODULE,
	.write = write,
};

static struct miscdevice w_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "atomicbuff_w",
	.fops = &w_ops
};

/** module init/exit **/
static int minit(void)
{
	int error;
	
	error =	misc_register(&r_misc);
	if (error < 0)
		return error;
	
	error = misc_register(&w_misc);
	if (error < 0) {
		misc_deregister(&r_misc);
		return error;
	}

	return 0;
}

static void mexit(void)
{
	misc_deregister(&r_misc);
	misc_deregister(&w_misc);
}

module_init(minit);
module_exit(mexit);

