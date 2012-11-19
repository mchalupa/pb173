#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/list.h>

struct mdata {
	struct list_head list;
	int domain;
	int bus;
	int slot;
	int func;
};

static void* iomem;

LIST_HEAD(l);

static int mprobe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	printk("Probing mpcidriver...\n");
	printk("%.2x:%.2x:%.2x\n", pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	printk("vendor %.2x, device %.2x\n", pdev->vendor, pdev->device);

	if (! pci_enable_device(pdev))
		return -EBUSY;

	if (pci_request_region(pdev, 0, "Only MAAAINNN region") != 0) {
		pci_disable_device(pdev);
		return -EBUSY;
	}

	iomem =	pci_ioremap_bar(pdev, 0);

	return 0;
}

static void mremove(struct pci_dev *pdev)
{
	printk("Removing mpcidriver\n");
	printk("%.2x:%.2x:%.2x\n", pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	iounmap(iomem);
	pci_release_region(pdev, 0);
	pci_disable_device(pdev);
}

static struct pci_device_id table[] = {
	{PCI_DEVICE(0x18ec, 0xc058)},
	{0}
};
MODULE_DEVICE_TABLE(pci, table);

static struct pci_driver driver = {
	.probe = mprobe,
	.remove = mremove,
	.name = "mpcidriver",
	.id_table = table,
};

static int my_init(void)
{
	__u32 dword;	/* for encoding COMBO info area */
	struct pci_dev *pdev = NULL;
	struct mdata *tmp;
	struct list_head *pos, *n;

	while((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev))) {
		/*
		printk(KERN_DEBUG "[bus:vendor:dev] %.2x:%.2x:%.2x\n",
				pdev->bus->number, pdev->vendor, pdev->device);
		*/

		pci_dev_get(pdev);

		tmp = kmalloc(sizeof(struct mdata), GFP_KERNEL);

		if (! tmp) {
			/* free already allocated memory upon error*/
			list_for_each_safe(pos, n, &l) {
				tmp = list_entry(pos, struct mdata, list);
				list_del(pos);
				kfree(tmp);
			}
			return -ENOMEM;
		} else {
			tmp->domain = pci_domain_nr(pdev->bus);
			tmp->bus = pdev->bus->number;
			tmp->slot = PCI_SLOT(pdev->devfn);
			tmp->func = PCI_FUNC(pdev->devfn);

			list_add(&(tmp->list), &l);
		}
	}

	/********************************/
	/* COMBO part			*/
	/********************************/

	/* register COMBO driver */
	if (! pci_register_driver(&driver))
		return -EBUSY;

	/* revisions */
	dword = readl(iomem);
	printk("major rev: %x, minor rev: %x\n", (dword  >> 8) & 0xff, dword & 0xff);

	/* date */
	dword = readl(iomem + 4);
	printk("20%02d-%02d-%02d %d:%d\n", dword >> 7*4, (dword  >> 6*4) & 0xf,
					(dword  >> 4*4) & 0xff, (dword  >> 2*4) & 0xff, (dword & 0xff));

	return 0;
}

static void my_exit(void)
{
	struct mdata *tmp;
	struct list_head *pos, *n;
	struct pci_dev *pdev = NULL;
	int present = 0; /* bool for comparing list of devices */

	while((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev))) {
		list_for_each_entry(tmp, &l, list) {
			if (tmp->domain == pci_domain_nr(pdev->bus) &&
				tmp->bus == pdev->bus->number &&
				tmp->slot == PCI_SLOT(pdev->devfn) &&
				tmp->func == PCI_FUNC(pdev->devfn))
				present = 1;
		}
		if (! present)
			/* print new devices */
			printk(KERN_INFO "%.2x:%.2x\n", pdev->vendor, pdev->device);
	}

	list_for_each_safe(pos, n, &l) {
		tmp = list_entry(pos, struct mdata, list);

		pci_dev_put(pci_get_bus_and_slot(tmp->bus, PCI_DEVFN(tmp->slot, tmp->func)));

		list_del(pos);
		kfree(tmp);
	}

	/* unregister COMBO driver */
	pci_unregister_driver(&driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
