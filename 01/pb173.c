#include <linux/module.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/io.h>

static int my_init(void)
{
	/* Print initial message */
/*	printk(KERN_INFO "Hello world!\n");*/
/*
	u32 *addr = ioremap(0x000000007ff64828, 8);
	
	printk(KERN_INFO "text: %4s\n", (char *) addr[0]);
	printk(KERN_INFO "size: %u\n", addr[1]);

	iounmap(addr);
*/	
	phys_addr_t phys;
	void *pg;
	struct page *pgstruct;
	void *map;

	pg = (void *)  __get_free_page(GFP_KERNEL);

	if (pg) {
		printk(KERN_INFO "virt: %p\n", pg);
		
		strcpy(pg, "page text");
		printk(KERN_INFO "text: %s\n", (char *)  pg);


		phys = virt_to_phys(pg);
		printk(KERN_INFO "phys: %llx\n", phys);

		pgstruct = virt_to_page(pg);
		SetPageReserved(pgstruct);

		printk(KERN_INFO "page: %p\n", pgstruct);

		map = ioremap(phys, PAGE_SIZE);

		printk(KERN_INFO "%p\n", map);
		
		printk("%d\n", page_to_pnf(pgstruct));

		iounmap(map);
	}

	return 0;
}

static void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

