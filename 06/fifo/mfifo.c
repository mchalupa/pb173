#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define NBUF 8192
#define BUFFER_SIZE (NBUF * sizeof(char))

DEFINE_KFIFO(fifo, char, NBUF);

/** reading device **/
ssize_t read(struct file *filp, char __user *ubuff, size_t count, loff_t *off)
{
	int err;
	char val;

	if (! count)
		return 0;

	if (! kfifo_get(&fifo, &val))
		return 0;	/* fifo is empty */

	err = put_user(val, ubuff);

	if (err != 0)
		return err;

	*off += 1;

	return 1;
}


/** writing device **/
ssize_t write(struct file *filp, const char __user *ubuff, size_t count, loff_t *off)
{
	int err;
	char val;

	if (! count)
		return 0;

	err = get_user(val, ubuff);
	
	if (err != 0)
		return err;

	if (! kfifo_put(&fifo, &val))
		return 0; /* fifo is full */

	(*off)++;

	return 1;
}

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.read = read,
	.write = write
};

static struct miscdevice f_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mfifo",
	.fops = &f_ops
};


/** module init/exit **/
static int minit(void)
{
	int error;

	error =	misc_register(&f_misc);
	if (error < 0)
		return error;

	return 0;
}

static void mexit(void)
{
	misc_deregister(&f_misc);
	kfifo_free(&fifo);
}

module_init(minit);
module_exit(mexit);

