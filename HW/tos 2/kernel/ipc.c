
#include <kernel.h>

PORT_DEF port[MAX_PORTS];
PORT next_free_port;

void add_to_block_list(PORT port, PROCESS sender);
void remove_from_block_list(PORT port);
void check_valid_port(PORT port);
void check_valid_process(PROCESS process);


/**
 * Creates a new port. The owner of the new port will be the calling process (active_proc). 
 * The return value of create_port() is the newly created port. The port is initially open.
 */
PORT create_port()
{

	return create_new_port(active_proc);
}

/**
 * Similar to create_port() except that the owner of the new port will be the process 
 * identified by owner.
 */
PORT create_new_port (PROCESS owner)
{
	PORT new_port;
	check_valid_process(owner);
	if(next_free_port != NULL) {
		new_port = next_free_port;
		next_free_port = new_port->next;

		new_port->used = TRUE;
		new_port->open = TRUE;
		new_port->owner = owner;

		if(owner->first_port == NULL) {
			new_port->next = NULL;			
		} else {
			new_port->next = owner->first_port;
		}
		owner->first_port = new_port; // Save the pointer to this first port in PCB.first_port
	}		
	return new_port;
}


/**
 * Opens a port. Only messages sent to an open port are delivered to the receiver.
 */
void open_port (PORT port)
{
	check_valid_port(port);
	port->open = TRUE;
}


/**
 * Closes a port. Messages can still be sent to a closed port, but they are not delivered 
 * to the receiver. If a port is closed, all incoming messages are queued.
 */
void close_port (PORT port)
{
	check_valid_port(port);
	port->open = FALSE;
}

/**
 * Sends a synchronous message to the port dest_port. The receiver will be passed the 
 * void-pointer data. The sender is blocked until the receiver replies to the sender.
 * Pseudo Code:
 * if (receiver is received blocked and port is open) {
 *     Change receiver to STATE_READY;
 *     Change to STATE_REPLY_BLOCKED;
 *  } else {
 *     Get on the send blocked list of the port;
 *     Change to STATE_SEND_BLOCKED;
 *  }
 */
void send (PORT dest_port, void* data)
{
	PROCESS receiver;
	check_valid_port(dest_port);
	receiver = dest_port->owner;
	check_valid_process(receiver);

	if ((receiver->state == STATE_RECEIVE_BLOCKED) && (dest_port->open == TRUE)) {
		// receiver is ready - received blocked. Message is delivered immediately
		receiver->param_proc = active_proc; 
		receiver->param_data = data; // pass the data
		add_ready_queue(receiver);
		active_proc->state = STATE_REPLY_BLOCKED;
	} else { // receiver is not ready. get on to the send block list of the port
		active_proc->param_data = data; // save the data
		add_to_block_list(dest_port, active_proc);
		active_proc->state = STATE_SEND_BLOCKED;
	}	

	//active_proc->param_data = data;
	remove_ready_queue(active_proc);
	resign();	 
}

/**
 * Sends a synchronous message to the port dest_port. The receiver will be passed the 
 * void-pointer data. The sender is unblocked after the receiver has received the message.
 * Pseudo Code:
 * if (receiver is receive blocked and port is open) {
 *     Change receiver to STATE_READY;
 * } else {
 *     Get on the send blocked list of the port;
 *     Change to STATE_MESSAGE_BLOCKED;
 * }
 */
void message (PORT dest_port, void* data)
{
	PROCESS receiver;
	check_valid_port(dest_port);
	receiver = dest_port->owner;
	check_valid_process(receiver);

	if ((receiver->state == STATE_RECEIVE_BLOCKED) && (dest_port->open == TRUE)) {
		receiver->param_proc = active_proc;
		receiver->param_data = data;
		add_ready_queue(receiver);
	} else { // receiver is not ready
		add_to_block_list(dest_port, active_proc);
		active_proc->state = STATE_MESSAGE_BLOCKED;		
		active_proc->param_data = data;
		remove_ready_queue(active_proc);
	}

	resign(); 
}

/**
 * Receives a message. If no message is pending for this process, the process becomes 
 * received blocked. This function returns the void-pointer passed by the sender and 
 * modifies argument sender to point to the PCB-entry of the sender.
 * Pseudo Code:
 * For every port:
 * 	    if (send blocked list is not empty) {
 *         sender = first process on the send blocked list;
 *         if (sender is STATE_MESSAGE_BLOCKED)
 *             Change state of sender to STATE_READY;
 * 	       if (sender is STATE_SEND_BLOCKED)
 *	           Change state of sender to STATE_REPLY_BLOCKED;
 *	    } else {
 *	       Change to STATE_RECEIVED_BLOCKED;
 *	    }
 */
void* receive (PROCESS* sender)
{
	PORT port;
	PROCESS source;
	void *data;
	port = active_proc->first_port;
	data = NULL;

	// get the first available port
	while(port != NULL) {
		check_valid_port(port);
		if(port->open && (port->blocked_list_head != NULL)) {
			break;
		}
		port = port->next;
	}

	if(port != NULL) {	// message pending
		source = port->blocked_list_head;
		check_valid_process(source);

		sender = &source; // or *sender = source;
		data = source->param_data;
		remove_from_block_list(port);

		if(source->state == STATE_MESSAGE_BLOCKED) {
			add_ready_queue(source);
			return data;
		} else if (source->state == STATE_SEND_BLOCKED) {
			source->state = STATE_REPLY_BLOCKED; 
			return data;			
		}					
	} else {   // no message pending
		active_proc->param_data = data;
		active_proc->state = STATE_RECEIVE_BLOCKED;
		remove_ready_queue(active_proc);
		resign();
	    *sender = active_proc->param_proc; // data has already passed to receiver
	    data = active_proc->param_data;
	    return data;
	}
}

/**
 * The receiver replies to a sender. The receiver must have previously received a 
 * message from the sender and the sender must be reply blocked.
 */
void reply (PROCESS sender)
{
	if (sender->state == STATE_REPLY_BLOCKED)
		add_ready_queue(sender);
		resign();
}


void init_ipc()
{
	int i;
	next_free_port = port;

	for(i = 0; i < MAX_PORTS - 1; i++)
		port[i].magic = MAGIC_PORT;
		port[i].used = FALSE;
		// port[i].open = FALSE; ?
		// no need since default is NULL
		// port[i].owner = NULL;
		// port[i].blocked_list_head = NULL;
		// port[i].blocked_list_tail = NULL;
		port[i].next = &port[i + 1];

	port[MAX_PORTS - 1].magic = MAGIC_PORT;
	port[MAX_PORTS - 1].used  = FALSE;   
    port[MAX_PORTS - 1].next  = NULL;
}

void add_to_block_list(PORT port, PROCESS sender)
{
	check_valid_port(port);
	check_valid_process(sender);
	if(port->blocked_list_head == NULL) // empty list
		port->blocked_list_head = sender;
	else // list is not empty, add to tail
		port->blocked_list_tail->next_blocked = sender;
	port->blocked_list_tail = sender;
	sender->next_blocked = NULL;
}

void remove_from_block_list(PORT port)
{
	check_valid_port(port);
	port->blocked_list_head = port->blocked_list_head->next_blocked;
	if(port->blocked_list_head == NULL)
		port->blocked_list_tail = NULL;
}

void check_valid_port(PORT port)
{
	assert(port->magic == MAGIC_PORT);
}

void check_valid_process(PROCESS process)
{
	assert(process->magic == MAGIC_PCB);
}