#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  
#include <linux/sched.h>  

//define DEBUG						
#define MYMODNAME "FindOffsets "


static int my_init_module(void);
static void my_cleanup_module(void);


static int my_init_module(void)
{
	printk(KERN_ALERT "Module %s loaded.\n\n", MYMODNAME);

    struct task_struct *p = current;
   

    if ( p != NULL ) {
          
#ifdef DEBUG
           printk(KERN_ALERT "task_struct start = %x.\n" , (unsigned int)p) ;
           printk(KERN_ALERT "task_struct : comm address = %x.\n" , (unsigned int)&(p->comm) ) ;
#endif

           unsigned long commOffset = (unsigned long)(&(p->comm)) - (unsigned long)(p);           
           printk(KERN_ALERT "linux_name = 0x%x\n" , (unsigned int)commOffset ) ;           

#ifdef DEBUG
           printk(KERN_ALERT "task_struct : tasks address = %x.\n" , (unsigned int)&(p->tasks) ) ;
#endif
           
           unsigned long tasksOffset = (unsigned long)(&(p->tasks)) - (unsigned long)(p);           
           printk(KERN_ALERT "linux_tasks = 0x%x\n" , (unsigned int)tasksOffset ) ; 
           
#ifdef DEBUG           
           printk(KERN_ALERT "task_struct : mm address = %x.\n" , (unsigned int)&(p->mm) ) ;
#endif
           unsigned long mmOffset = (unsigned long)(&(p->mm)) - (unsigned long)(p);           
           printk(KERN_ALERT "linux_mm = 0x%x\n" , (unsigned int)mmOffset ) ; 
      
#ifdef DEBUG
           printk(KERN_ALERT "task_struct : pid address = %x.\n" , (unsigned int)&(p->pid) ) ;
#endif
           
           unsigned long pidOffset = (unsigned long)(&(p->pid)) - (unsigned long)(p);           
           printk(KERN_ALERT "linux_pid = 0x%x\n" , (unsigned int)pidOffset ) ; 
                      
#ifdef DEBUG     
           printk(KERN_ALERT "mm_struct  : start address = %x.\n" , (unsigned int)(p->mm) ) ;
           printk(KERN_ALERT "mm_struct : pgd address = %x.\n" , (unsigned int)&((p->mm)->pgd) ) ;
#endif
           
           unsigned long pgdOffset = (unsigned long)( &(p->mm->pgd) ) - (unsigned long)(p->mm);           
           printk(KERN_ALERT "linux_pgd = 0x%x\n" , (unsigned int)pgdOffset ) ; 
                      

#ifdef DEBUG
           printk(KERN_ALERT "mm_struct : start_code address = %x.\n" , (unsigned int)&(p->mm->start_code) ) ;
#endif
           unsigned long addrOffset = (unsigned long)( &(p->mm->start_code) ) - (unsigned long)(p->mm);           
           printk(KERN_ALERT "linux_addr = 0x%x\n" , (unsigned int)addrOffset ) ; 
           
#ifdef DEBUG                   
           printk(KERN_ALERT "task_struct next_task address = %x.\n" , (unsigned int)(next_task(p)) ) ;
#endif
		
		
		

    } else {

            printk(KERN_ALERT "%s: found no process to populate task_struct.\n",           MYMODNAME);
    }

    return 0;
}

static void my_cleanup_module(void)
{
        printk(KERN_ALERT "Module %s unloaded.\n", MYMODNAME);
}



module_init(my_init_module);
module_exit(my_cleanup_module);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nilushan SIlva");
MODULE_DESCRIPTION("task_struct offset Finder");
