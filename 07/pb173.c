#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#define BYTE(ptr, n) ((*((__u8 *) ptr) + n) & 0xff)

#define COMBO_INT_NO 	0x100

#define RAISED_INT 	0x40
#define ENABLED_INT 	0x44
#define RAISE_INT	0x60
#define ACKNW_INT	0x64

struct mdata {
	struct list_head list;
	struct pci_dev *pdev;
};

static void* iomem;

dma_addr_t dma_phys;
void *dma_virt;

LIST_HEAD(l);

static irqreturn_t irqhandler(int irq, void *data, struct pt_regs *ptregs)
{
	__u32 dword = readl(data + RAISED_INT);

	if (dword) {
		printk(KERN_INFO "IRQ %x handled\n", dword);
		writel(COMBO_INT_NO, data + ACKNW_INT);
		writel(0x1 << 31, data + 0x8c);
		return IRQ_HANDLED;
	} else {
		return IRQ_NONE;
	}
}

static void run_dma(__u32 from, __u32 to, __u8 bus1, __u8 bus2, __u32 size, __u8 interrupt)
{
	/* set DMA registry */
	writel(from, iomem + 0x80);
	writel(to, iomem + 0x84);
	writel(size, iomem + 0x88);

	writel((1 | bus1 << 1 | bus2 << 4 | interrupt << 7), iomem + 0x8c);
}

static int mprobe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	__u32 dword;	/* for decoding COMBO info area */
	int irqret;

	printk("Probing mpcidriver...\n");
	printk("%.2x:%.2x:%.2x\n", pdev->bus->number, PCI_SLOT(pdev->devfn),
				PCI_FUNC(pdev->devfn));
	printk("vendor %.2x, device %.2x\n", pdev->vendor, pdev->device);

	if (pci_enable_device(pdev) < 0)
		return -EBUSY;

	pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	pci_set_master(pdev);

	if (pci_request_region(pdev, 0, "Only MAAAINNN region") < 0) {
		pci_disable_device(pdev);
		return -EFAULT;
	}

	iomem =	pci_ioremap_bar(pdev, 0);

	if(! iomem) {
		pci_disable_device(pdev);
		pci_release_region(pdev, 0);
		return -ENOMEM;
	}

	irqret = request_irq(pdev->irq, irqhandler, IRQF_SHARED, "Combo IRQ", iomem);

	if (irqret) {
		printk(KERN_ERR "Combo: Couldn't register IRQ %d\n", pdev->irq);

		iounmap(iomem);
		pci_release_region(pdev, 0);
		pci_disable_device(pdev);
		return -EIO;
	}

	/* enable interrupt in Combo */
	writel(COMBO_INT_NO, iomem + ENABLED_INT);

	/* revisions */
	dword = readl(iomem);
	printk("major rev: %x, minor rev: %x\n", BYTE(&dword, 1),
						BYTE(&dword, 0));

	/* date */
	dword = readl(iomem + 4);
	/* 0xYMDDhhmm */
	printk("20%02d-%02d-%02d %d:%d\n", BYTE(&dword, 3) >> 4,
					BYTE(&dword, 3) & 0xf,
					BYTE(&dword, 2), BYTE(&dword, 1),
					BYTE(&dword, 0));

	
	dma_virt = dma_alloc_coherent(NULL, PAGE_SIZE, &dma_phys, GFP_KERNEL);

	if (! dma_virt) {
		iounmap(iomem);
		pci_release_region(pdev, 0);
		pci_disable_device(pdev);
		return -ENOMEM;
	}

	strcpy(dma_virt, "1234567890");

	run_dma(dma_phys, 0x40000, 2, 4, 10, 1);

	while(readl(iomem + 0x8c) & 0x1)
		msleep(100);

	printk(KERN_INFO "DMA transfer (-> ward)  completed\n");

	run_dma(0x40000, dma_phys + 10, 4, 2, 10, 0);

	while(readl(iomem + 0x8c) & 0x1)
		msleep(100);

	printk(KERN_INFO "DMA transfer (<- ward)  completed\n");
	printk(KERN_INFO "mem: %20s\n", (char *) dma_virt);

	/**********
	 HW
	**********/	
	run_dma(0x40000, dma_phys + 20, 4, 2, 10, 0);

	printk(KERN_INFO "mem: %30s\n", (char *) dma_virt);

	return 0;
}

static void mremove(struct pci_dev *pdev)
{
	printk("Removing mpcidriver\n");
	printk("%.2x:%.2x:%.2x\n", pdev->bus->number, PCI_SLOT(pdev->devfn),
						PCI_FUNC(pdev->devfn));

	dma_free_coherent(NULL, PAGE_SIZE, dma_virt, dma_phys);

	free_irq(pdev->irq, iomem);

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
	.name = "combodriver",
	.id_table = table,
};

static void raise_interrupt(unsigned long data);
DEFINE_TIMER(int_timer, raise_interrupt, 0, 0);

static void raise_interrupt(unsigned long data)
{
	/* raise interrupt */
	/*writel(COMBO_INT_NO, iomem + RAISE_INT);

	mod_timer(&int_timer, jiffies + msecs_to_jiffies(100));*/
}


static int my_init(void)
{
	struct pci_dev *pdev = NULL;
	struct mdata *tmp;
	struct list_head *pos, *n;


	while((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev))) {
		tmp = kmalloc(sizeof(struct mdata), GFP_KERNEL);

		if (! tmp) {
			/* free already allocated memory upon error*/
			list_for_each_safe(pos, n, &l) {
				tmp = list_entry(pos, struct mdata, list);
				list_del(pos);
				pci_dev_put(tmp->pdev);
				kfree(tmp);
			}
			return -ENOMEM;
		} else {
			pci_dev_get(pdev);
			tmp->pdev = pdev;
			list_add(&(tmp->list), &l);
		}
	}

	/* register COMBO driver */
	if (pci_register_driver(&driver) < 0)
		return -EBUSY;

	/* start interrupt timer */
	mod_timer(&int_timer, jiffies + msecs_to_jiffies(100));

	return 0;
}

static void my_exit(void)
{
	struct mdata *tmp;
	struct list_head *pos, *n;
	struct pci_dev *pdev = NULL;
	int present = 0;

	/* delete timer */
	del_timer_sync(&int_timer);

	/* print new devices and free the old one */
	while((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev))) {
		list_for_each_safe(pos, n, &l) {
			tmp = list_entry(pos, struct mdata, list);

			if (pci_domain_nr(tmp->pdev->bus) == pci_domain_nr(pdev->bus) &&
				tmp->pdev->bus->number == pdev->bus->number &&
				PCI_SLOT(tmp->pdev->devfn) == PCI_SLOT(pdev->devfn) &&
				PCI_FUNC(tmp->pdev->devfn) == PCI_FUNC(pdev->devfn)) {
				
				pci_dev_put(tmp->pdev);
				kfree(tmp);
				list_del(pos);

				present = 1;
				break;	
			}
		}
		/* not in the list - new device */
		if (! present)
			printk(KERN_INFO "New: %.2x:%.2x\n", pdev->vendor, pdev->device);
		else
			present = 0;
	}

	/* rest of list are removed devices
	   print them and free */
	list_for_each_safe(pos, n, &l) {
		tmp = list_entry(pos, struct mdata, list);

		printk(KERN_INFO "Rem: %.2x:%.2x\n", tmp->pdev->vendor, tmp->pdev->device);

		pci_dev_put(tmp->pdev);
		kfree(tmp);
		list_del(pos);
	}

	/* unregister COMBO driver */
	pci_unregister_driver(&driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
