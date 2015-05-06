#include <kernel.h>

#define MAX_LENGTH 54
#define WELCOME "\nWelcome to TOS:\n"
#define PROMPT "jd@TOS>"
#define PROMPT_LENGTH 7

void print(char *s);
void execute_command(char *s);
int get_full_length(char *s);
int get_next_word(char *s, int *start, int *length, char *word);
int get_word_length(char *s, int *start, int *length);
void s_copy(char *s, char *d, int start, int length);
int s_cmp(char *s, char *d);
void clear_s(char *s);
int atoi(char *p);
void print_help(WINDOW *wnd);

WINDOW shell_wnd = {0, 9, 61, 16, 0, 0, '_'};
WINDOW* train_wnd;


void shell_process(PROCESS self, PARAM param) {
	char ch, line[MAX_LENGTH]; // input buffer
	int length = 0;
	Keyb_Message msg;
	
	clear_window(train_wnd);
	clear_window(&shell_wnd);
	print(WELCOME);
	print(PROMPT);

	while (1) {
		while (1) {
			msg.key_buffer = &ch;
			send(keyb_port, &msg);
			// check character
			switch (ch) {
				case 13: // <enter>, execute command
				line[length] = '\0';
				print("\n");
				execute_command(line);
				clear_s(line);		
				length = 0; // erase previous input
				print("jd@TOS>");
				break;
				case 8: // <backspace>, adjust cursor
				if (length) {
					length--;
					line[length] = ' ';
					remove_cursor(&shell_wnd);	
					move_cursor(&shell_wnd, length + PROMPT_LENGTH, shell_wnd.cursor_y);	
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
}

void printl(char *s) {
	print(s);
	print("\n");
}

// parse the command and then execute
void execute_command(char *s) {
	int start, wl, full_length;
	char method[10], argument[5];
	start = 0;
	wl = 0;
	full_length = 0;

	// wprintf(&shell_wnd, "original s: %s.\n", s);
	full_length = get_full_length(s);
	//wprintf(&shell_wnd, "full length: %d.\n", full_length);

	if (get_next_word(s, &start, &wl, method)) {
		if (!s_cmp(method, "ps")) {
			print_all_processes(&shell_wnd);
		} else if (!s_cmp(method, "clear")) {
			clear_window(&shell_wnd);
		} else if (!s_cmp(method, "help")) {
			print_help(&shell_wnd); 
		} else if (!s_cmp(method, "sleep")) {
			if (get_next_word(s, &start, &wl, argument)) {
				int sleep_time = atoi(argument);
				if (sleep_time) sleep(sleep_time);
			} else {
				wprintf(&shell_wnd, "Argument Error: Need to provide sleep time.\n");
			}
		} else if (!s_cmp(method, "train")) {
			clear_window(train_wnd);
			wprintf(train_wnd, "--------------Start--------------\n");
			init_train(train_wnd);
		} else if (!s_cmp(method, "stop")) {
			stop();
		} else if (!s_cmp(method, "go")) {
			go();
		}
	}
	clear_s(method);
	clear_s(argument);
}

// get the length of the input string
int get_full_length(char *s) {
	int i = 0;
	while (*s++) i++;
	return i;
}


int get_next_word(char *s, int *start, int *length, char *word) {
	if (get_word_length(s, start, length)) {
		s_copy(s, word, *start, *length);
		*start = *start + *length;
		//wprintf(&shell_wnd, "word: %s.\n", word);
		return 1;
	} else {
		return 0;
	}
}


// get the next word length with specified start position
int get_word_length(char *s, int *start, int *length) {
	int i = 0, skip = *start;
	
	// skip the first few chars
	for (; i < skip; i++) {
			*s++;
	}
	i = 0;
	while (*s == ' ') { // skip starting white spaces
		s++;
		skip += 1;
	}
	*start = skip;
	//wprintf(&shell_wnd, "first skip %d, ", *start);
	
	// count the word length
	while ((*s != ' ') && *s) { 
		s++;
		i++;
	}
	*length = i;
	//wprintf(&shell_wnd, "word length is %d, ", *length);

	skip += i;
	//wprintf(&shell_wnd, "next should skip %d\n", skip);
	return i;
}

void s_copy(char *s, char *d, int start, int length) {
	int i = 0;

	for (; i < start; i++) { // skip chars
		*s++;
	}

	i = 0;
	for (; i < length; i++) { // copy characters
		d[i] = *s++;
	}
	d[length] = '\0'; // need to explicitely put '\0' at the end!
	//wprintf(&shell_wnd, "copy %d chars: %s\n", length, d);
}

int s_cmp(char *s, char *d) {
	for(; *s == *d; s++, d++) {
		if (*s == '\0')
			return 0;
	}
	return *s - *d;
}

void clear_s(char *s) {
	while (*s) {
		*s++ = '\0';
	}
}


int atoi(char *p) {
    int var = 0;
    while(*p >= '0' && *p <= '9') {
		char ch = *p++;
		ch -= '0';
		var *= 10;
		var += ch;
    }
    return var;
}


void print_help(WINDOW *wnd) {
	int i = 0;
	char *text[] = {
		"TOS command line guide:                    \n",
		"clear       :  clear screen                \n",
		"help        :  print command line guide    \n",
		"ps          :  print all processes         \n",
		"sleep [TIME]:  sleep for the specified time\n",
		NULL
	};
	while (text[i]) {
		wprintf(wnd, text[i++]);
	}
}

void init_shell()
{
	create_process(shell_process, 6, 0, "Shell process");
	resign();
}
