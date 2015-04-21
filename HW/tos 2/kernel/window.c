
#include <kernel.h>
#define MEMORY_START 0xb8000
#define LINE_MEMORY_CAPACITY 160
#define SCREEN_WIDTH 80

MEM_ADDR get_addr(int x, int y)
{
	return MEMORY_START + (y * SCREEN_WIDTH + x) * 2;
}

/* calculate the starting address of the window */
MEM_ADDR get_window_start_addr(WINDOW* wnd)
{
    return get_addr(wnd->x, wnd->y);
}

/* calculate the end address of the window */
MEM_ADDR get_window_end_addr(WINDOW* wnd)
{
    return get_addr(wnd->x + wnd->width, wnd->y + wnd->height); 
}

/* claculate the memory address for the current cursor location */
MEM_ADDR get_cursor_addr(WINDOW* wnd)
{
    return get_addr(wnd->x + wnd->cursor_x, wnd->y + wnd->cursor_y);
}


void poke_w_char(MEM_ADDR addr, char c)
{
	static const WORD BRIGHT_WHITE = 0x0f00;
    poke_w(addr, c | BRIGHT_WHITE);
}

void clear_word(MEM_ADDR addr)
{
    poke_w_char(addr, ' ');
}

void clear_screen(WINDOW* wnd)
{
    MEM_ADDR i; 
    for(i = get_window_start_addr(wnd); i < get_window_end_addr(wnd); clear_word(i), i += 2) 
	;
}

void move_cursor(WINDOW* wnd, int x, int y)
{
    assert(x >= 0 && x < wnd->width && y >= 0 && y < wnd->height);
	wnd->cursor_x = x;
	wnd->cursor_y = y;
}

/* remove the cursor by displaying a blank character at its location */
void remove_cursor(WINDOW* wnd)
{
    poke_w_char(get_cursor_addr(wnd), ' ');
}


/* display the cursor character at current location */
void show_cursor(WINDOW* wnd)
{
    poke_w_char(get_cursor_addr(wnd), wnd->cursor_char);   
}

/* clear the window content and move the cursor to the top left corner (0,0) */
void clear_window(WINDOW* wnd)
{
	volatile int saved_if;

	DISABLE_INTR(saved_if);
    clear_screen(wnd);
    wnd->cursor_x = 0;
    wnd->cursor_y = 0;
    show_cursor(wnd);
    ENABLE_INTR(saved_if);
}

void copy_w(MEM_ADDR src, MEM_ADDR des) 
{
    WORD value = peek_w(src);
    poke_w(des, value);
}

/* copy line
   Note: the sequence is from line start to line end */
void copy_line(MEM_ADDR src, MEM_ADDR des, int length)
{
    int i;
    for(i = 0; i < length; i++) {
    	copy_w(src, des);
		src += 2;
		des += 2;		
	}
}

/* scroll down the window by coping memory, discard the first line of the window */
void scroll_down(WINDOW* wnd)
{
	int x, y;
	for (y = wnd->y; y < wnd->y + wnd->height - 1; y++) {
		for (x = wnd->x; x < wnd->x + wnd->width; x++) {
			poke_w(get_addr(x, y), peek_w(get_addr(x, y + 1)));
		}
	}
	// y = wnd->y + wnd->height - 1;
	for (x = wnd->x; x < wnd->x + wnd->width; x++) {
		poke_w_char(get_addr(x, y), ' ');
	}
    
    // move cursor accordingly
	wnd->cursor_x = 0;
    wnd->cursor_y = wnd->height - 1;
}

/* move the cursor to the next line
   if at the bottom of the window, scroll the window down by one line */
void move_to_next_line(WINDOW* wnd)
{
    if(wnd->cursor_y == wnd->height)
    	// at the bottom of the window
        scroll_down(wnd);
    else
        wnd->cursor_y++;
    wnd->cursor_x = 0;
}
 
/* check whether the cursor need to move to the next line */
int should_move_to_next_line(WINDOW* wnd, unsigned char c)
{
    return (c == '\n') || (wnd->cursor_x == wnd->width);
}
 
/* display the character c at the current cursor location, and advances the cursor to the next location 
   Note: need to check the boundary of the window, support wrap-around and scroll */
void output_char(WINDOW* wnd, unsigned char c)
{
	remove_cursor(wnd);
	switch(c) {
		case '\n':
		case 13: // carriage return
			wnd->cursor_x = 0;
			wnd->cursor_y++;
			break;
		case '\b': // backspace
			if(wnd->cursor_x != 0) {
				wnd->cursor_x--;
			} else {
				wnd->cursor_x = wnd->width - 1;
				wnd->cursor_y--;
			}
			break;
		default:
			poke_w_char(get_cursor_addr(wnd), c);
			wnd->cursor_x++;
			if(wnd->cursor_x == wnd->width) {
				wnd->cursor_x = 0;
				wnd->cursor_y++;
			}
			break;
	}
	if(wnd->cursor_y == wnd->height) {
		scroll_down(wnd);
		show_cursor(wnd);
	}
}

void output_string(WINDOW* wnd, const char *str)
{
    char c;
    volatile int saved_if;

    DISABLE_INTR(saved_if);
    while(c = *str++)
        output_char(wnd, c);
    ENABLE_INTR(saved_if);
}



/*
 * There is not need to make any changes to the code below,
 * however, you are encouraged to at least look at it!
 */
#define MAXBUF (sizeof(long int) * 8)		 /* enough for binary */

char *printnum(char *b, unsigned int u, int base,
	       BOOL negflag, int length, BOOL ladjust,
	       char padc, BOOL upcase)
{
    char	buf[MAXBUF];	/* build number here */
    char	*p = &buf[MAXBUF-1];
    int		size;
    char	*digs;
    static char up_digs[] = "0123456789ABCDEF";
    static char low_digs[] = "0123456789abcdef";
    
    digs = upcase ? up_digs : low_digs;
    do {
	*p-- = digs[ u % base ];
	u /= base;
    } while( u != 0 );
    
    if (negflag)
	*b++ = '-';
    
    size = &buf [MAXBUF - 1] - p;
    
    if (size < length && !ladjust) {
	while (length > size) {
	    *b++ = padc;
	    length--;
	}
    }
    
    while (++p != &buf [MAXBUF])
	*b++ = *p;
    
    if (size < length) {
	/* must be ladjust */
	while (length > size) {
	    *b++ = padc;
	    length--;
	}
    }
    return b;
}


/*
 *  This version implements therefore following printf features:
 *
 *	%d	decimal conversion
 *	%u	unsigned conversion
 *	%x	hexadecimal conversion
 *	%X	hexadecimal conversion with capital letters
 *	%o	octal conversion
 *	%c	character
 *	%s	string
 *	%m.n	field width, precision
 *	%-m.n	left adjustment
 *	%0m.n	zero-padding
 *	%*.*	width and precision taken from arguments
 *
 *  This version does not implement %f, %e, or %g.  It accepts, but
 *  ignores, an `l' as in %ld, %lo, %lx, and %lu, and therefore will not
 *  work correctly on machines for which sizeof(long) != sizeof(int).
 *  It does not even parse %D, %O, or %U; you should be using %ld, %o and
 *  %lu if you mean long conversion.
 *
 *  This version implements the following nonstandard features:
 *
 *	%b	binary conversion
 *
 */


#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define ctod(c) ((c) - '0')


void vsprintf(char *buf, const char *fmt, va_list argp)
{
    char		*p;
    char		*p2;
    int			length;
    int			prec;
    int			ladjust;
    char		padc;
    int			n;
    unsigned int        u;
    int			negflag;
    char		c;
    
    while (*fmt != '\0') {
	if (*fmt != '%') {
	    *buf++ = *fmt++;
	    continue;
	}
	fmt++;
	if (*fmt == 'l')
	    fmt++;	     /* need to use it if sizeof(int) < sizeof(long) */
	
	length = 0;
	prec = -1;
	ladjust = FALSE;
	padc = ' ';
	
	if (*fmt == '-') {
	    ladjust = TRUE;
	    fmt++;
	}
	
	if (*fmt == '0') {
	    padc = '0';
	    fmt++;
	}
	
	if (isdigit (*fmt)) {
	    while (isdigit (*fmt))
		length = 10 * length + ctod (*fmt++);
	}
	else if (*fmt == '*') {
	    length = va_arg (argp, int);
	    fmt++;
	    if (length < 0) {
		ladjust = !ladjust;
		length = -length;
	    }
	}
	
	if (*fmt == '.') {
	    fmt++;
	    if (isdigit (*fmt)) {
		prec = 0;
		while (isdigit (*fmt))
		    prec = 10 * prec + ctod (*fmt++);
	    } else if (*fmt == '*') {
		prec = va_arg (argp, int);
		fmt++;
	    }
	}
	
	negflag = FALSE;
	
	switch(*fmt) {
	case 'b':
	case 'B':
	    u = va_arg (argp, unsigned int);
	    buf = printnum (buf, u, 2, FALSE, length, ladjust, padc, 0);
	    break;
	    
	case 'c':
	    c = va_arg (argp, int);
	    *buf++ = c;
	    break;
	    
	case 'd':
	case 'D':
	    n = va_arg (argp, int);
	    if (n >= 0)
		u = n;
	    else {
		u = -n;
		negflag = TRUE;
	    }
	    buf = printnum (buf, u, 10, negflag, length, ladjust, padc, 0);
	    break;
	    
	case 'o':
	case 'O':
	    u = va_arg (argp, unsigned int);
	    buf = printnum (buf, u, 8, FALSE, length, ladjust, padc, 0);
	    break;
	    
	case 's':
	    p = va_arg (argp, char *);
	    if (p == (char *)0)
		p = "(NULL)";
	    if (length > 0 && !ladjust) {
		n = 0;
		p2 = p;
		for (; *p != '\0' && (prec == -1 || n < prec); p++)
		    n++;
		p = p2;
		while (n < length) {
		    *buf++ = ' ';
		    n++;
		}
	    }
	    n = 0;
	    while (*p != '\0') {
		if (++n > prec && prec != -1)
		    break;
		*buf++ = *p++;
	    }
	    if (n < length && ladjust) {
		while (n < length) {
		    *buf++ = ' ';
		    n++;
		}
	    }
	    break;
	    
	case 'u':
	case 'U':
	    u = va_arg (argp, unsigned int);
	    buf = printnum (buf, u, 10, FALSE, length, ladjust, padc, 0);
	    break;
	    
	case 'x':
	    u = va_arg (argp, unsigned int);
	    buf = printnum (buf, u, 16, FALSE, length, ladjust, padc, 0);
	    break;
	    
	case 'X':
	    u = va_arg (argp, unsigned int);
	    buf = printnum (buf, u, 16, FALSE, length, ladjust, padc, 1);
	    break;
	    
	case '\0':
	    fmt--;
	    break;
	    
	default:
	    *buf++ = *fmt;
	}
	fmt++;
    }
    *buf = '\0';
}



void wprintf(WINDOW* wnd, const char *fmt, ...)
{
    va_list	argp;
    char	buf[160];

    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    output_string(wnd, buf);
    va_end(argp);
}




static WINDOW kernel_window_def = {0, 0, 80, 25, 0, 0, ' '};
WINDOW* kernel_window = &kernel_window_def;


void kprintf(const char *fmt, ...)
{
    va_list	  argp;
    char	  buf[160];

    va_start(argp, fmt);
    vsprintf(buf, fmt, argp);
    output_string(kernel_window, buf);
    va_end(argp);
}


