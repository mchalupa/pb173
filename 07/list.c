#include <linux/io.h>
#include <linux/module.h>
#include <linux/list.h>

struct mydata {
	void *p;
	struct list_head list;
}

static int my_init(void)
{
	int count;
	struct mydata *temp;

	LIST_HEAD(l);



/*
	temp = kmalloc(sizeof(*temp), GFP_KERNEL);
	list_add(temp->lh, &l);
*/
















	return -EIO;
}

static void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
