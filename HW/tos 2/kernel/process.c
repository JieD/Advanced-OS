
#include <kernel.h>


PCB pcb[MAX_PROCS];
PCB *next_free_pcb; // PCB* == PROCESS

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
	PORT new_port;
	volatile int saved_if;

	DISABLE_INTR(saved_if);
	assert(prio < MAX_READY_QUEUES);
	assert(next_free_pcb != NULL); // pcbs are not all used

	new_proc = next_free_pcb;
	next_free_pcb = new_proc -> next; 
	ENABLE_INTR(saved_if);

	new_proc->magic = MAGIC_PCB;
	new_proc->used = TRUE;
	new_proc->state = STATE_READY;
	new_proc->priority = prio;
	new_proc->first_port = NULL;
	new_proc->name = name;

	new_port = create_new_port(new_proc);

	// new_proc->esp = 640 - (new_proc - pcb) * 30;
	/* Compute linear address of new process' system stack */
    esp = (640 - (new_proc - pcb) * 16) * 1024;

// define macro, '\' is line splicing for preprocessing
#define PUSH(x)    esp -= 4; \
                   poke_l (esp, (LONG) x);

    /* Initialize the stack for the new process */
    PUSH (param);		/* First data */
    PUSH (new_proc);		/* Self */
    PUSH (0);			/* Dummy return address */
    if (interrupts_initialized) {
    	PUSH (512); /* Avoid enable interrupts when they are not initialized */
    } else {
    	PUSH(0);
    }
    PUSH (CODE_SELECTOR);
    PUSH (ptr_to_new_proc);	/* EIP -> Entry point of new process */
    PUSH (0);			/* EAX */
    PUSH (0);			/* ECX */
    PUSH (0);			/* EDX */
    PUSH (0);			/* EBX */
    PUSH (0);			/* EBP */
    PUSH (0);			/* ESI */
    PUSH (0);			/* EDI */
    
#undef PUSH

    new_proc->esp = esp;
    add_ready_queue(new_proc);
    return new_port;
}


PROCESS fork()
{
    // Dummy return to make gcc happy
    return (PROCESS) NULL;
}

void print_process_head(WINDOW* wnd)
{
	wprintf(wnd, "                        TOS Process Table        \n");
	wprintf(wnd, "Name                     State                 Prio Active\n");
	wprintf(wnd, "---------------------------------------------------------------------\n");
}

void print_process_info(WINDOW* wnd, PROCESS p)
{
	static const char *state[] = 
	{ "READY          ",
	  "SEND_BLOCKED   ",
	  "REPLY_BLOCKED  ",
	  "RECEIVE_BLOCKED",
	  "MESSAGE_BLOCKED",
	  "INTR_BLOCKED   "
	};
	
	wprintf(wnd, "%-25s", p->name);
	wprintf(wnd, "%-22s", state[p->state]);
	wprintf(wnd, "%-5d", p->priority);
	/* Check for active_proc */
    if (p == active_proc) wprintf(wnd, "  *  ");
	wprintf(wnd, "\n");
}


/*
 * print the details of process proc to window wnd
 * including name, state, priority and whether is active
 */
void print_process(WINDOW* wnd, PROCESS p)
{
	print_process_head(wnd);
	print_process_info(wnd, p);
}

/*
 * print the details of all processes currently existing in TOS to window wnd
 */
void print_all_processes(WINDOW* wnd)
{
	/* int i;
	PCB process;
	print_process_head(wnd);
	for(i = 0; i < MAX_PROCS; i++) {
		process = pcb[i];
		if(process.used) {
			print_process_info(wnd, &process);
		}
	} */
	int i;
    PCB* p = pcb;
    print_process_head(wnd);
    for (i = 0; i < MAX_PROCS; i++, p++) {
		if (p->used) print_process_info(wnd, p);
    }
}


/**
 * Create a list of free PCBs
 * The first entry is the boot process.
 */
void init_process()
{
	int i;

	// clear states of all PCBs
	for(i = 0; i < MAX_PROCS; i++)
		pcb[i].magic = 0;
		pcb[i].used = FALSE;

	// init the free PCBs (the first pcb will be reserved for boot process)
	for(i = 1; i < MAX_PROCS - 1; i++)
		pcb[i].next = &pcb[i+1];
	pcb[MAX_PROCS - 1].next = NULL; // special value signify no PCB available
	next_free_pcb = &pcb[1];

	// init the first PCB as the boot process
	active_proc = pcb;
	pcb[0].magic = MAGIC_PCB;
	pcb[0].used = TRUE;
	pcb[0].state = STATE_READY;
	pcb[0].priority = 1;
	pcb[0].first_port = NULL; // why NULL?
	pcb[0].name = "Boot process";
}
