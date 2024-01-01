
#include <linux/kernel.h>
#include <linux/syscalls.h>

//asmlinkage long sys_hello(void) {
SYSCALL_DEFINE3(hello, int*, result, int, size, int, count) {

    int histo[65];
    long cnt = count;
    long foo;
    int num;

    printk("Hello from thrasher\n");
    memset(histo, 0, sizeof(histo));
 
    asm volatile (
        "movq %1, %%r8\n"            // load the address of the histogram
        "movq %2, %%r9\n"            // count iterations
        "loop:\n"
        "  cli\n"
        "  rdtsc\n"                  // read the clock
        "  movl %%eax, %%esi\n"      // store the values in registers 
        "  movl %%edx, %%edi\n"      // so they don't get clobbered
        "  rdtsc\n"                  // read the clock again
        "  sti\n"                    
        "  shlq $32,%%rdx\n"         // shift the hi part of the tsc left by 32 bits
        "  movl %%eax, %%eax\n"      // clear the high bits of rax
        "  orq %%rdx, %%rax\n"       // OR them together so that rax contains end time
        "  shlq $32,%%rdi\n"         // shift the hi part of the start time
        "  orq %%rdi, %%rsi\n"       // OR the start time parts together 
        "  subq %%rsi, %%rax\n"      // subtract start time from end time
        "  bsrq %%rax, %%rax\n"      // find the highest bit set in the elapsed time
        "  incl (%%r8, %%rax, 4)\n"  // increment the bucket in the histogram
        "  decq %%r9\n"              // decrement the loop counter
        "  jnz loop\n"               // and loop until the counter is zero
        : "=r"(foo)
        : "r"(histo), "r"(cnt)
        : "%rax", "%rcx", "%rdx", "%rdi", "%rsi", "r8", "r9"
    );  

    num = 65 * sizeof(unsigned int);
    if(size < num) num = size;
    if (copy_to_user(result, histo, num))
        return -EFAULT;

    return 0;
}
