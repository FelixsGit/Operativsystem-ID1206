//printk -prints tot he kernel log
//ioctl -Sends a request to a kernel module
//less -Terminal text viewer
  //'shift + 'g' -Goes to the end of text
  //':' + 'q' -Exits less

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dr Chandra");
MODULE_DESCRIPTION("Heuristically programmed Algorithmic computer");
static int __init hal_init(void){
  here:
  		printk(KERN_INFO "I'm here %p) \n", &&here);
  return 0;
}
static void __exit hal_cleanup(void){
  printk(KERN_INFO "Whar areyou doing, Dave?\n");
}
module_init(hal_init);
module_exit(hal_cleanup);
