#include <kernel.h>

void print(char *s);
void execute_command(char *s);

WINDOW shell_wnd = {0, 9, 61, 16, 0, 0, '_'};
int MAX_LENGTH = 61;


void shell_process(PROCESS self, PARAM param) {
	char ch, line[61]; // input buffer
	int length = 0;
	Keyb_Message msg;
	clear_window(&shell_wnd);
	print("\nType shell commands:\n");

	while (1) {
		while (1) {
			msg.key_buffer = &ch;
			send(keyb_port, &msg);
			// check character
			switch (ch) {
				case 13: // <enter>, execute command
				print("\n");
				//execute_command(line);				
				//print(line);
				//print("\n");
				length = 0; // erase previous input
				break;
				case 8: // <backspace>
				if (length) {
					length--;
					line[length] = ' ';
					remove_cursor(&shell_wnd);	
					move_cursor(&shell_wnd, length, shell_wnd.cursor_y);	
					show_cursor(&shell_wnd);
				}
				break;
				default: // regular character
				if (length <= MAX_LENGTH) {	
					line[length++] = ch;
					print(&ch);
				}
				break;
			}
		}
	}
}

void print(char *s) {
	wprintf(&shell_wnd, s);
	//show_cursor(&shell_wnd);
}

void execute_command(char *s) {
	int start, wl, full_length;
	start = 0;
	wl = 0;
	full_length = 0;

	full_length = strlen(s);
	wprintf(&shell_wnd, "length is %d\n", full_length);

	word_length(s, &start, &wl);
	wprintf(&shell_wnd, "skip %d, word length is %d\n", start, wl);

	word_length(s, &start, &wl);
	wprintf(&shell_wnd, "skip %d, word length is %d\n", start, wl);
	// start = start + wl + 1;
	/** char *method, *args;
	for (; i < MAX_LENGTH; i++) {
		*method++ = *s++;
		if (*s == ' ') { // first space
			s++;
			break;
		}
	}
	//while (*args++ = *s++) ;
	print("\n");
	print(method);
	//print(args); */
}


int strlen(char *s) {
	int i = 0;
	while (*s++) i++;
	return i;
}

int word_length(char *s, int *start, int *length) {
	int i = 0, skip = *start;
	wprintf(&shell_wnd, "first skip %d\n", skip);
	if (skip > 0) { // skip the first few chars
		for (; i < skip; i++) {
			*s++;
		}
		while ((*s == ' ') && !*s) { // skip the white space
			s++;
			skip += 1;
		}
	}
	i = 0;

	while (*s != ' ' && *s) {
		s++;
		i++;
	}
	*length = i;
	wprintf(&shell_wnd, "word length is %d\n", *length);

	*start = *start + i + 1;
	wprintf(&shell_wnd, "next should skip %d\n", *start);
	return i;
}



void init_shell()
{
	create_process(shell_process, 6, 0, "Shell process");
	resign();
}
