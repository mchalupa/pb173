#include <linux/module.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/io.h>
#include <asm/page.h>

static int my_init(void)
{
	phys_addr_t phys;
	void *pg;
	void *map;
	struct page *pgstruct;

	pg = (void *)  __get_free_page(GFP_KERNEL);

	if (pg) {
		printk(KERN_INFO "virt: %p\n", pg);
		
		strcpy(pg, "page text");

		phys = virt_to_phys(pg);
		printk(KERN_INFO "phys: %llx\n", phys);

		pgstruct = virt_to_page(pg);
		SetPageReserved(pgstruct);

		printk(KERN_INFO "page: %p\n", pgstruct);

		map = ioremap(phys, PAGE_SIZE);

		printk(KERN_INFO "map: %p\n", map);
		
		printk(KERN_INFO "pfn: %lu\n", page_to_pfn(pgstruct));

		printk(KERN_INFO "%%s virt: %s\n", (char *) pg);

		printk(KERN_INFO "%%s map: %s\n", (char *) map);

		strcpy(map, "Changed map content");

		printk(KERN_INFO "%%s virt: %s\n", (char *) pg);

		iounmap(map);
		ClearPageReserved(pgstruct);
		free_page((unsigned long) pg);
	}

	return 0;
}

static void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

