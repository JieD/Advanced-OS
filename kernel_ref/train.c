
#include <kernel.h>

#define PAUSE_NUMBER 15
#define CR '\015'
#define GREEN 'G'
#define RED 'R'
#define TRAIN_ID "20"


void pause();
void clear_buffer();
void change_direction();
void change_speed(int speed);
void set_train_speed(char* speed);
void reset_switches();
void reset_outer_switches();
void reset_innerer_switches();
void reset_switch(char number, char color);

extern WINDOW* train_wnd;
static WINDOW train_window_def = {0, 0, 80, 8, 0, 0, ' '};
WINDOW* train_window = &train_window_def;

//COM_Message msg;
char buffer[3];
char FS = '5';
char LS = '3';


//**************************
//run the train application
//**************************


void pause() {
	sleep(PAUSE_NUMBER);
}

void reset_switches() 
{
	reset_outer_switches();
	reset_innerer_switches();
}


/* reset outer switches so that no trains runs out of track
 * set switch 1, 4, 5, 8 to G, 9 to R
 */
void reset_outer_switches() {
	reset_switch('1', GREEN);
	reset_switch('4', GREEN);
	reset_switch('5', GREEN);
	reset_switch('8', GREEN);
	reset_switch('9', RED);
}


/* reset inner switches so that no trains runs into deadlock
 * set switch 2, 3, 6, 7 to G
 */
void reset_innerer_switches() {
	reset_switch('2', GREEN);
	reset_switch('3', GREEN);
	reset_switch('6', GREEN);
	reset_switch('7', GREEN);
}

void reset_switch(char number, char color) 
{
	COM_Message msg;
	char cmd[5];
	cmd[0] = 'M';
	cmd[1] = number; 
	cmd[2] = color;
	cmd[3] = CR;
	cmd[4] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    pause();
}


void change_direction() {
	COM_Message msg;
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
    pause();
}


void change_speed(int speed) {
	COM_Message msg;
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
    pause();
}


void set_train_speed(char* speed) 
{
	COM_Message msg;
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
    pause();
}


void clear_buffer() {
	COM_Message msg;
	char cmd[3];
	cmd[0] = 'R';
	cmd[1] = CR;
	cmd[2] = '\0';
	msg.output_buffer = cmd;
	msg.len_input_buffer = 0;   
    send(com_port, &msg);
    pause();
}



void train_process(PROCESS self, PARAM param)
{
	reset_switches();
	et_train_speed(&FS);
	while (1);
}


void init_train(WINDOW* wnd)
{
	create_process(train_process, 6, 0, "Train process");
	resign();
}
