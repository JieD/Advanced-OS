
#include <kernel.h>

#include "disptable.c"

/*
 * process that currently owns the CPU.
 */
PROCESS active_proc;


/*
 * Ready queues for all eight priorities.
 * MAX_READY_QUEUES	= 8
 * the same priority processes on the ready queue are organized as a doubly linked list
 */
PCB *ready_queue [MAX_READY_QUEUES];

/*
 * the state (empty or not) of process lists for all priorities (on the ready queue).
 * use bit patterns to indicate state (empty or not)
 */
unsigned ready_lists_state;


/*
 * add_ready_queue
 *----------------------------------------------------------------------------
 * The process pointed to by p is put the ready queue.
 * The appropiate ready queue is determined by p->priority.
 *
 * Steps:
 * 1. Changes the state of process p to ready (p->state = STATE_READY)
 * 2. Process p is added to the ready queue
 * 3. Process p is added at the tail of the double linked list for the appropriate priority level.
 * 4. p->priority determines to which priority level the process is added in the ready queue 
 *    automatically maintains the double-linked list
 */

void add_ready_queue (PROCESS proc)
{
	unsigned short current_priority;
	volatile int saved_if;

	DISABLE_INTR(saved_if);
	assert (proc->magic = MAGIC_PCB);
	current_priority = proc->priority;

	if (ready_queue[current_priority] == NULL) {
		// if empty at this priority
		ready_queue[current_priority] = proc;
    	proc->next = proc;
    	proc->prev = proc;
    	ready_lists_state |= 1 << current_priority; // set the corresponding bit to 1
	} else {
        // add to the tail
    	proc->prev = ready_queue[current_priority]->prev;
        proc->next = ready_queue[current_priority];
        ready_queue[current_priority]->prev->next = proc;
        ready_queue[current_priority]->prev = proc;
    }
    proc->state = STATE_READY;
    ENABLE_INTR(saved_if);
}



/*
 * remove_ready_queue
 *----------------------------------------------------------------------------
 * The process pointed to by p is dequeued from the ready
 * queue.
 *
 * Steps:
 * 1. Process p is removed from the ready queue.
 * 2. After the removal of the process, the ready queue should be a double-linked list 
 *    again with process p removed.
 * NOTE: no need to change process state since not knowing whether it will be processed
 * 		 be CPU, be killed or etc.
 *       proc is not necessarily the tail.
 */

void remove_ready_queue (PROCESS proc)
{
	unsigned short current_priority;
	volatile int saved_if;

	DISABLE_INTR(saved_if);
	assert (proc->magic = MAGIC_PCB);
	current_priority = proc->priority;

	if (proc->next == proc) {
		// the only process at this priority
		ready_queue[current_priority] = NULL;
	    ready_lists_state &= ~(1 << current_priority);
	}
	else {
		if (ready_queue[current_priority] == proc)
			ready_queue[current_priority] = proc->next;
		proc->prev->next = proc->next;
		proc->next->prev = proc->prev;
	}
	ENABLE_INTR(saved_if);

	// cannot have code below since the implementaion is not a real round-robin
	// proc->prev = NULL;
	// proc->next = NULL;
}

/*
 * check whether the bitindex is set for value
 */
BOOL is_bit_set(unsigned value, unsigned bitindex)
{
    return (value & (1 << bitindex)) != 0;
}

int get_highest_priority(unsigned value)
{
    int i;
    for(i = MAX_READY_QUEUES - 1; i >= current_priority; i--)
        if(is_bit_set(value, i))
        	return i;
}

/*
 * dispatcher
 *----------------------------------------------------------------------------
 * Determines a new process to be dispatched. The process
 * with the highest priority is taken. Within one priority
 * level round robin is used.
 *
 * Steps:
 * The next process is selected based on active_proc:
 * If there is a process with a higher priority than active_proc->priority, then 
 * that process will be chosen.
 * Otherwise, active_proc->next will be chosen (Round-Robin within the same priority level).
 */

PROCESS dispatcher()
{
	int current_priority, highest_priority;
	volatile int saved_if;
	PROCESS candidate;

	DISABLE_INTR(saved_if);
	current_priority = active_proc->priority;
	highest_priority = get_highest_priority(ready_lists_state, 0);
	assert((highest_priority >= 0) && (highest_priority <= 7));
	if(highest_priority == current_priority)
		candidate = active_proc->next;
	else
		candidate = ready_queue[highest_priority];
	ENABLE_INTR(saved_if);
	return candidate;
}

/* helper function used in resign()
 * no local variable or function parameters in resign()
 * since we are manipulating stack directly
 */
void check_active() {
	assert(active_proc->magic == MAGIC_PCB);
}

/*
 * resign
 *----------------------------------------------------------------------------
 * The current process gives up the CPU voluntarily. The
 * next running process is determined via dispatcher().
 * The stack of the calling process is setup such that it
 * looks like an interrupt.
 *
 * Steps:
 * 1. save context
 * 2. active_proc->esp = %esp;
 * 3. active_proc = dispatcher();
 * 4. %esp = active_proc->esp;
 * 5. restore context
 */
void resign()
{
	// save context for inter-segment jump
	asm("pushfl");
	asm("cli");
	asm("popl %eax");
	asm("xchgl (%esp),%eax");
    asm("push %cs");
    asm("pushl %eax");

    // save context for intra-segment jump
	asm("pushl %eax");
	asm("pushl %ecx");
	asm("pushl %edx");
	asm("pushl %ebx");
	asm("pushl %ebp");
	asm("pushl %esi");
	asm("pushl %edi");

    // manipulate esp
	asm ("movl %%esp,%0" : "=r" (active_proc->esp) : );
    active_proc = dispatcher();
    check_active(); // helper function 
    asm ("movl %0,%%esp" : : "r" (active_proc->esp));

    // restore context
    asm("popl %edi");
    asm("popl %esi");
    asm("popl %ebp");
    asm("popl %ebx");
    asm("popl %edx");
    asm("popl %ecx");
    asm("popl %eax");

    // asm("ret"); // pop %EIP, resume process to where it was stopped
    asm("iret");
}



/*
 * init_dispatcher
 *----------------------------------------------------------------------------
 * Initializes the necessary data structures.
 *
 * Steps:
 * 1. init the ready queue with null pointers
 * 2. clear ready_lists_state
 * 3. add the first process (active_proc always exists) 
 */

void init_dispatcher()
{
	int i;
	for(i = 0; i < MAX_READY_QUEUES; i++)
		ready_queue[i] = NULL;

	ready_lists_state = 0;
	add_ready_queue(active_proc);
}
