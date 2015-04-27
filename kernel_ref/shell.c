#include <kernel.h>


WINDOW shell_wnd = {0, 9, 61, 16, 0, 0, 0xDC};


void shell_process(PROECSS self, PARAM param) {
	while (1) {

	}
}


void init_shell()
{
	create_process(shell_process, 6, (PARAM)keyb_port, "Shell process");
	resign();
}
