#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

static int __init mkit_init(void)
{
        printk(KERN_ALERT "Hello world from kernel!! \n");
        return 0;
}

static void __exit mkit_exit(void)
{
        printk(KERN_ALERT "Exiting hello world module from kernel !!!\n");
}

module_init(mkit_init);
module_exit(mkit_exit);