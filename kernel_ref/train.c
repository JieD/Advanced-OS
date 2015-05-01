
#include <kernel.h>

#define SHORT_PAUSE 15
#define LONG_PAUSE 45
#define CR '\015'
#define GREEN 'G'
#define RED 'R'
#define TRAIN_ID "20"
#define OUTER_LOOP 1
#define INNER_LOOP 0
#define CHECK_LOOP 100


void short_pause();
void long_pause();
void clear_buffer();
void change_direction();
void change_speed(int speed);
void set_train_speed(char* speed);
void reset_switches();
void stay_in_track();
void trap_in_outer_loop();
void trap_in_inner_loop();
void reset_switch(char number, char color);
int poll(char *contact_number);
void check_configuration();
void start_1_nz();
void keep_polling(char *contact_number);
void check_zamboni();
void print_zamboni();
void print_configuration(char *cfg);
void start_train();

extern WINDOW* train_wnd;
static WINDOW train_window_def = {0, 0, 80, 8, 0, 0, ' '};
WINDOW* train_wnd = &train_window_def;

COM_Message msg;
char buffer[4];
char FS = '5';
char LS = '4';
char zero = '0';
int configuration, zamboni = 0;


//**************************
//run the train application
//**************************


void short_pause() {
	sleep(SHORT_PAUSE);
}

void long_pause() {
	sleep(LONG_PAUSE);
}

void reset_switches() 
{
	trap_in_outer_loop();
	trap_in_inner_loop();
}


void stay_in_track() 
{
	reset_switch('3', GREEN);
	reset_switch('6', GREEN);
	reset_switch('9', RED);
}

/* reset switches so that trains runs in the outer track
 * set switch 1, 4, 5, 8 to G, 9 to R
 */
void trap_in_outer_loop() {	
	reset_switch('1', GREEN);
	reset_switch('4', GREEN);	
	reset_switch('5', GREEN);	
	reset_switch('8', GREEN);
	reset_switch('9', RED);	
}


/* reset switches so that trains runs in the inner track
 * set switch 2, 3, 6, 7 to G
 */
void trap_in_inner_loop() {
	reset_switch('1', RED);
	reset_switch('2', RED);	
	reset_switch('7', RED);
	reset_switch('8', RED);
	reset_switch('9', RED);	
}

void trap(int flag) 
{
	if (flag == INNER_LOOP) reset_switch('1', RED);
	else reset_switch('1', GREEN);
}


void reset_switch(char switch_number, char color) 
{
	char cmd[5];
	cmd[0] = 'M';
	cmd[1] = switch_number; 
	cmd[2] = color;
	cmd[3] = CR;
	cmd[4] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    short_pause();
}


void change_direction() 
{
	char cmd[6];
	cmd[0] = 'L';
	cmd[1] = TRAIN_ID[0];
	cmd[2] = TRAIN_ID[1];
	cmd[3] = 'D';
	cmd[4] = CR;
	cmd[5] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    short_pause();
}


void change_speed(int speed) 
{
	assert (speed >= 0 && speed <= 5);
	char cmd[7];
	cmd[0] = 'L';
	cmd[1] = TRAIN_ID[0];
	cmd[2] = TRAIN_ID[1];
	cmd[3] = 'S';
	cmd[4] = '0' + speed; // convert to ASCII Character
	cmd[5] = CR;
	cmd[0] = '\0';	
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    short_pause();
}


void set_train_speed(char* speed) 
{	
	char cmd[7];
	cmd[0] = 'L';
	cmd[1] = TRAIN_ID[0];
	cmd[2] = TRAIN_ID[1];
	cmd[3] = 'S';
	cmd[4] = *speed;
	cmd[5] = CR;	
	cmd[6] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    short_pause();
}


int poll(char *contact_number) 
{
	clear_buffer();

	// construct command
	int length = get_full_length(contact_number) + 3;
	char cmd[length + 3];
	cmd[0] = 'C';
	cmd[1] = contact_number[0];	
	if (length == 5) {
		cmd[2] = contact_number[1];
	}
	cmd[length - 2] = CR;
	cmd[length - 1] = '\0';
	
	// send command to com
	msg.output_buffer = cmd;
	msg.input_buffer = buffer;
	msg.len_input_buffer = 3;   
    send(com_port, &msg);
    short_pause();

    // print sent command
    buffer[3] = '\0';
    //wprintf(train_wnd, "poll contact %s: ", contact_number);
    //wprintf(train_wnd, buffer);

    // check polling result
    if (buffer[1] == '1') return 1;
    else return 0;
}


void clear_buffer() 
{
	char cmd[3];
	cmd[0] = 'R';
	cmd[1] = CR;
	cmd[2] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    short_pause();
}


void check_zamboni() 
{
	int i = 0;
	for (; i < CHECK_LOOP; i++)
	{
		if (poll("10")) {
			zamboni = 1;
			break;
		}
	}
	print_zamboni();
}


/* contacts   -   configuration
 *  8 & 2     -      1 & 2
 *  5 & 11    -        3
 *  5 & 16    -        4
 */
void check_configuration() 
{
	char *cfg;
	if (poll("8") && poll("2")) { // configuration 1 and 2
		configuration = 1;
		cfg = "1 or 2";		
	} else if (poll("5")) { // configuration 3
		if (poll("11")) {
			configuration = 3;
			cfg = "3";
		} else if (poll("16")) { // configuration 4
			configuration = 4;
			cfg = "4";			
		}
	}		
	print_configuration(cfg);	
}


void start_train() 
{
	if (configuration == 1) {
		if (zamboni) trap_in_inner_loop(); // track zamboni in the inner loop
		keep_polling("12");
		wprintf(train_wnd, "zamboni is trapped successfully!\n");
		start_1_2();
	} else if (configuration == 3) {
		if (zamboni) {}
	}
}


void print_zamboni() {
	if (zamboni) wprintf(train_wnd, "zamboni is on.\n");
	else wprintf(train_wnd, "zamboni is off.\n");
}


void print_configuration(char *cfg) {
	wprintf(train_wnd, "configuration is %s.\n", cfg);
}


void start_1_2() 
{	
	// start train
	reset_switch('4', RED);
	reset_switch('3', GREEN);
	set_train_speed(&FS);

	// slow down
	keep_polling("6");
	set_train_speed(&LS);

	// rendezvous, stop and change speed
	keep_polling("1");
	set_train_speed(&zero);
	reset_switch('5', RED);
	reset_switch('6', RED);
	change_direction();

	// start and return
	set_train_speed(&FS);
	keep_polling("7");
	set_train_speed(&LS);
	keep_polling("8");
	set_train_speed(&zero);

	wprintf(train_wnd, "Success!\n");
}


void keep_polling(char *contact_number) {
	while (!poll(contact_number))
		short_pause();
}


void train_process(PROCESS self, PARAM param)
{
	stay_in_track();
	trap_in_outer_loop();
	check_zamboni();
	check_configuration();		
	start_train();
	while (1);
}


void init_train(WINDOW* wnd)
{
	create_process(train_process, 6, 0, "Train process");
	resign();
}
