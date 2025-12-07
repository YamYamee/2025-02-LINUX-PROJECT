#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

static struct workqueue_struct *my_wq;
static struct work_struct work_A, work_B, work_C;

// Import the custom queue-jumping function we embedded in the kernel.
// (We need to declare it with extern since we didn't modify the header file)
extern bool queue_work_head_on(int cpu, struct workqueue_struct *wq, struct work_struct *work);

void task_func_A(struct work_struct *work) {
    printk(KERN_INFO "[TEST] Task A starting (blocking queue... 2 seconds)\n");
    msleep(2000); // Blocking the queue for 2 seconds
    printk(KERN_INFO "[TEST] Task A completed\n");
}

void task_func_B(struct work_struct *work) {
    printk(KERN_INFO "[TEST] Task B executed (I am a normal task)\n");
}

void task_func_C(struct work_struct *work) {
    printk(KERN_INFO "[TEST] Task C executed (I am an urgent task! Queue jump successful?)\n");
}

static int __init my_module_init(void) {
    printk(KERN_INFO "[TEST] === Workqueue Queue-Jump Test Started ===\n");

    // Create a single-threaded queue to make the execution order obvious.
    my_wq = create_singlethread_workqueue("my_test_wq");

    if (!my_wq) return -1;

    INIT_WORK(&work_A, task_func_A);
    INIT_WORK(&work_B, task_func_B);
    INIT_WORK(&work_C, task_func_C);

    // 1. Submit task A (worker will be busy processing it)
    queue_work(my_wq, &work_A);
    
    // 2. Submit task B (joins the queue behind A)
    queue_work(my_wq, &work_B);

    // 3. Submit task C (★using our custom function★)
    // If successful: should be placed at the front of the queue, 
    // so it executes before B
    queue_work_head_on(WORK_CPU_UNBOUND, my_wq, &work_C);

    return 0;
}

static void __exit my_module_exit(void) {
    destroy_workqueue(my_wq);
    printk(KERN_INFO "[TEST] Test completed\n");
}


module_init(my_module_init);
module_exit(my_module_exit);
