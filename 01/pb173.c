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

		/*
		 * HOMEWORK
		 */

		/* print adress of allocated variable */
		printk(KERN_INFO "%pS\n", mem);

		/* print adress of variable on the stack */
		printk(KERN_INFO "%p\n", &mem);

		/* print adress of jiffies */
		printk(KERN_INFO "%p\n", &jiffies);

		/* print adress of some function in this module */
		printk(KERN_INFO "%p", &my_init);

		/* print adress of some func outside of this module */
		printk(KERN_INFO "%p\n", &vprintk);

		/* print name + offset of return adress of this function*/
		printk(KERN_INFO "%pF\n", __builtin_return_address(0));

		kfree(mem);
	}
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

