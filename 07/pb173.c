#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mm.h>

static void* iomem;

static int mprobe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	printk("Probing mpcidriver...\n");
	printk("%.2x:%.2x:%.2x\n", pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	printk("vendor %.2x, device %.2x\n", pdev->vendor, pdev->device);

	pci_enable_device(pdev);

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

	__u32 dword;

	/*
	struct pci_dev *pdev = NULL;
	while((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev)))
		pci_dev_get(..)  -- zvyseni reference
		nezxapomenout v release zavolat pci_dev_put()
		printk("[bus:vendor:dev] %.2x:%.2x:%.2x\n", 
				pdev->bus->number, pdev->vendor, pdev->device);
	*/

	pci_register_driver(&driver);

	dword = readl(iomem);
	printk("major rev: %x, minor rev: %x\n", (dword  >> 8) & 0xff, dword & 0xff);

	dword = readl(iomem + 4);

	printk("20%02d-%02d-%02d %d:%d\n", dword >> 7*4, (dword  >> 6*4) & 0xf,
					(dword  >> 4*4) & 0xff, (dword  >> 2*4) & 0xff, (dword & 0xff));

	return 0;
}

static void my_exit(void)
{
	pci_unregister_driver(&driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
