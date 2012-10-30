#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#define READ_MAX_SIZE 1000
#define WRITE_MAX_SIZE 1000
#define MEM_SIZE 0xAp21 /* = 10 * 2^21 = 20MB */

/* buffer */
static void *mem;

static void memory_init(void)
{
	int page_number = MEM_SIZE / PAGE_SIZE;
	int i;
	void *page = mem;	
	
	char addr[30];

	for(i = 0; i < page_number; i++, page += PAGE_SIZE) {
		sprintf(addr, "%p:0x%lx\n", page, vmalloc_to_pfn(page) * PAGE_SIZE);
		strcpy(page, addr);
	}
}

/** read from device **/
ssize_t read(struct file *filp, char __user *ubuff, size_t count, loff_t *off)
{
	size_t read_size;
	size_t unred;

	if (*off >= MEM_SIZE || !count)
		return 0;

	read_size = READ_MAX_SIZE > count ? count : READ_MAX_SIZE;
	
	if (*off + read_size >= MEM_SIZE)
		read_size = MEM_SIZE - *off;

	unred = copy_to_user(ubuff, mem + *off, read_size);

	if (unred == 0)	{/* ok */
		*off += read_size;
		return read_size;
	}
	else if (unred) { /* untransfered chars */
		*off += (read_size - unred);
		return (read_size - unred);
		}
	else	/* error */
		return unred;
}


/** write to device **/
ssize_t write(struct file *filp, const char __user *ubuff, size_t count, loff_t *off)
{
	size_t write_size;
	size_t unwritten;

	if (*off >= MEM_SIZE || ! count)
		return 0;
	
	write_size = count > WRITE_MAX_SIZE ? WRITE_MAX_SIZE : count;

	/* check overflow */
	if (*off + write_size >= MEM_SIZE)
		write_size = MEM_SIZE - *off;

	unwritten = copy_from_user(mem + *off, ubuff, write_size);

	if (unwritten == 0) /* all ok */
		return write_size;
	else if (unwritten) /* not all bytes were written */
		return (write_size - unwritten);
	else		/* error */
		return unwritten;
}


static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = read,
	.write = write
};

static struct miscdevice miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "buf",
	.fops = &fops
};

/** module init/exit **/
static int minit(void)
{
	int error;

	error =	misc_register(&miscdev);
	if (error < 0)
		return error;

	if (! (mem = vzalloc(MEM_SIZE))) {
		misc_deregister(&miscdev);
		return -ENOMEM;
	}
	
	memory_init();

	return 0;
}

static void mexit(void)
{
	misc_deregister(&miscdev);
	vfree(mem);
}

module_init(minit);
module_exit(mexit);

