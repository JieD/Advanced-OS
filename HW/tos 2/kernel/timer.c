
#include <kernel.h>


PORT timer_port;

/* helper process which waits for interrupt */
void timer_notifier(PROCESS self, PARAM param)
{
	while (1) {
		wait_for_interrupt(TIMER_IRQ);
		message(timer_port, 0);
	}
}

void timer_process(PROCESS self, PARAM param)
{
	int i, process_number;
	Timer_Message *message;	
	int sleep_table[MAX_PROCS];
	PROCESS sender;

	// init sleep_table
	for (i = 0; i < MAX_PROCS; i++) {
		sleep_table[i] = 0;
	}
	create_process(timer_notifier, 7, (PARAM)timer_port, "timer notifier");
	while (1) {
		message = (Timer_Message*) receive(&sender);
		if (message != NULL) { // from user process
			process_number = sender - pcb;
			assert(&pcb[process_number] == sender);
			sleep_table[process_number] = message->num_of_ticks; // record the sleep time
		} else { // from timer notifier
			for (i = 0; i < MAX_PROCS; i++) {
				if (sleep_table[i]) { // only decrement non-zero
					sleep_table[i]--;
					if (sleep_table[i]) { // time is up
						reply(&pcb[i]);
					}
				}
			}
		}
	}
}

void sleep(int ticks)
{
	Timer_Message message;
	message.num_of_ticks = ticks;
	send(timer_port, &message);
}


void init_timer ()
{
	timer_port = create_process(timer_process, 6, 0, 'timer proecess');
	resign();
}
