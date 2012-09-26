#include <linux/module.h>

static int my_init(void)
{
	/* Print initial message */
	printk(KERN_INFO "Hello world!\n");
	
	return 0;
}

static void my_exit(void)
{
	void *mem = kmalloc(1000, GFP_KERNEL);
	
	/* if we got the memory, fill it and print.
	 * Free allocated memory after */
	if (mem) {
		strcpy(mem, "Bye");
		printk(KERN_INFO "%s\n", (char *)  mem);
		kfree(mem);
	}
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
