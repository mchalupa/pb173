#include <linux/module.h>

static int my_init(void)
{
	printk(KERN_INFO "Hello world!\n");
	
	void *mem = kmalloc(1000, GFP_KERNEL);
	if (mem) {
		printk(KERN_INFO "Got memory\n");
		strcpy(mem, "Bye");
		printk("mem: %p\n", mem);
		printk("stack: %p\n", &mem);
		printk("str: %s\n", (char *)  mem);
		printk("jiffies: %p\n", &jiffies);
		printk("myfunc: %p\n", &my_init);
		printk("otherfunc: %p\n", &printk);
		printk("%x\n", __builtin_return_address(0));
	}

	kfree(mem);

	return 0;
}

static void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
