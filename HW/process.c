
#include <kernel.h>


PCB pcb[MAX_PROCS];
PCB *next_free_pcb; // PCB* == PROCESS

void push(x)
{
	esp -= 4;
	poke_l(esp, x);
}

/*
 * Steps:
 * Allocates an available PCB entry
 * Initializes the elements of this PCB entry
 * Allocates an available 30KB stack frame for this process
 * Saves the stack pointer to PCB.esp 
 * Adds the new process to the ready queue
 * Returns a NULL pointer
 */
PORT create_process (void (*ptr_to_new_proc) (PROCESS, PARAM),
		     int prio,
		     PARAM param,
		     char *name)
{
	MEM_ADDR esp;
	PROCESS new_proc;

	assert(prio < MAX_READY_QUEUES);
	assert(next_free_pcb != NULL); // pcbs are not all used

	new_proc = next_free_pcb;
	next_free_pcb = new_proc -> next; 

	new_proc->magic = MAGIC_PCB;
	new_proc->used = TRUE;
	new_proc->state = STATE_READY;
	new_proc->priority = prio;
	new_proc->first_port = NULL;
	new_proc->name = name;

	// new_proc->esp = 640 - (new_proc - pcb) * 30;
	/* Compute linear address of new process' system stack */
    esp = (640 - (new_proc - pcb) * 16) * 1024;
    push(param);
    push(new_proc);
    push(0);
    push(ptr_to_new_proc); // EIP
    push(0);
    push(0);
    push(0);
    push(0);
    push(0);
    push(0);
    push(0);
    new_proc->esp = esp;

    add_ready_queue(new_proc);
    return NULL;
}


PROCESS fork()
{
    // Dummy return to make gcc happy
    return (PROCESS) NULL;
}



/*
 * print the details of process proc to window wnd
 * including name, state, priority and whether is active
 */
void print_process(WINDOW* wnd, PROCESS p)
{

}

/*
 * print the details of all processes currently existing in TOS to window wnd
 */
void print_all_processes(WINDOW* wnd)
{
}


/**
 * Create a list of free PCBs
 */
void init_process()
{
	int i;

	// clear states of all PCBs
	for(i = 0; i < MAX_PROCS; i++)
		pcb[i].magic = 0;
		pcb[i].used = FALSE;

	// init the free PCBs (the first pcb will be reserved for boot process)
	for(int i = 1; i < MAX_PROCS - 1; i++)
		pcb[i].next = &pcb[i+1];
		pcb[MAX_PROCS - 1] = NULL;
	next_free_pcb = &pcb[1];

	// init the first PCB as the boot process
	active_proc = pcb;
	pcb[0].magic = MAGIC_PCB;
	pcb[0].used = TRUE;
	pcb[0].state = STATE_READY;
	pcb[0].priority = 1;
	pcb[0].first_port = NULL;
	pcb[0].name = "Boot process";
}
