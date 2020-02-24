#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/uio_driver.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <linux/sched.h>

#define PAGES 1
//#define NUM_IRQ XXX

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bitchebe");
MODULE_DESCRIPTION("UIO VTF driver");

static struct uio_info* info;
static struct platform_device *pdev;
static struct vtf_info VTF_info = { 0U, 0U, NULL };
void * buff = NULL;

static int uio_vtf_hotplug_open(struct uio_info *info, struct inode *inode)
{
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

static int uio_vtf_hotplug_release(struct uio_info *info, struct inode *inode)
{
    kthread_should_stop();
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

int uio_vtf_hotplug_mmap(struct uio_info *info, struct vm_area_struct *vma){
    unsigned long len = vma->vm_end - vma->vm_start;

    unsigned long pfn = virt_to_phys((void *)buffer)>>PAGE_SHIFT;
    int ret ;

    ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
    if (ret < 0) {
        pr_err("could not map the address area\n");
        free_mmap_pages(buffer,PAGES);
        return -EIO;
    }

    pr_info("memory map called success \n");

    return ret;
}

static irqreturn_t vtf_irq_handler(int irq, void *dev_id)
{
    printk("irq %d\n", irq);
    printk(KERN_INFO "In the handler - pml[0] = %lu\n", VTF_info.pml[0]);
    uio_event_notify(info);
    return IRQ_RETVAL(1);
}

static int __init uio_vtf_hotplug_init(void)
{

    pdev = platform_device_register_simple("uio_vtf_device",
                                            0, NULL, 0);
    if (IS_ERR(pdev)) {
        pr_err("Failed to register platform device.\n");
        return -EINVAL;
    }

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    
    if (!info)
        return -ENOMEM;
    
    /*
     *
     */
    struct xen_add_to_physmap xatp;
	unsigned int page_order = 8;

	buff = (void *) __get_free_pages(GFP_KERNEL | __GFP_ZERO, page_order);
	if (!buff) {
		pr_err("not enough memory\n");
		return -ENOMEM;
	}

	xatp.domid = DOMID_SELF;
	xatp.idx = 0;
	xatp.size = (1u << page_order);
	xatp.space = XENMAPSPACE_pml_shared_info;
	xatp.gpfn = virt_to_gfn(buff);
	if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp))
		BUG();

	VTF_info.pml_page_order = page_order;
	VTF_info.nr_pml_entries = (1u << page_order)* PAGE_SIZE / sizeof(unsigned long) - 1;
	VTF_info.pml = (unsigned long* ) buff;
      /**/
    info->name = "uio_vtf_driver";
    info->version = "0.1";
    info->mem[0].addr = (phys_addr_t) buff;
    if (!info->mem[0].addr)
        goto uiomem;
    info->mem[0].memtype = UIO_MEM_LOGICAL;
    info->mem[0].size = PAGE_SIZE;
    info->irq = NUM_IRQ;
    info->handler = vtf_irq_handler;
    info->mmap = uio_vtf_hotplug_mmap;
    info->open = uio_vtf_hotplug_open;
    info->release = uio_vtf_hotplug_release;
	
    if(uio_register_device(&pdev->dev, info)) {
        pr_err("Unable to register UIO device!\n");
        goto devmem;
    }

    printk(KERN_INFO "pml[0] = %lu\n", VTF_info.pml[0]);

    pr_info("VTF uio hotplug driver loaded\n");
    return 0;

devmem:
    kfree((void *)info->mem[0].addr);
uiomem:
    kfree(info);
    
    return -ENODEV;
}

static void __exit uio_vtf_hotplug_cleanup(void)
{
//    pr_info("Cleaning up module.\n");

    if (pdev)
        platform_device_unregister(pdev);    
    
    free_mmap_pages(buffer, PAGES);

    printk("Cleaning up module.\n");
}

early_initcall(uio_vtf_hotplug_init);
module_init(uio_vtf_hotplug_init);
module_exit(uio_vtf_hotplug_cleanup);
