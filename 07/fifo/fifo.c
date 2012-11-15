#include <linux/io.h>
#include <linux/module.h>
#include <linux/kfifo.h>

#define SIZE 8
static int my_init(void)
{
	struct kfifo *fifo;
	int i = 0;

	fifo = kfifo_alloc(SIZE*sizeof(int) , GFP_KERNEL, NULL);
	
	while(__kfifo_len(fifo) < SIZE) {
		__kfifo_put(fifo, (void *)  &i, sizeof(int));
		i++;
	}

	kfifo_free(fifo);
	return -EIO;
}

static void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
