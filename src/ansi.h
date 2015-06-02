/* ansi.h - ANSI control codes for various neat-o terminal effects. */

#include "copyright.h"

#ifndef __ANSI_H
#define __ANSI_H

#define BEEP_CHAR				'\07'
#define ESC_CHAR				'\033'

#define ANSI_CSI				'['
#define ANSI_END				'm'

#define ANSI_NORMAL				"\033[0m"

#define ANSI_HILITE				"\033[1m"
#define ANSI_INVERSE			"\033[7m"
#define ANSI_BLINK				"\033[5m"
#define ANSI_UNDER				"\033[4m"

#define ANSI_INV_BLINK			"\033[7;5m"
#define ANSI_INV_HILITE			"\033[1;7m"
#define ANSI_BLINK_HILITE		"\033[1;5m"
#define ANSI_INV_BLINK_HILITE	"\033[1;5;7m"

/* Foreground colors */

#define ANSI_BLACK				"\033[30m"
#define ANSI_RED				"\033[31m"
#define ANSI_GREEN				"\033[32m"
#define ANSI_YELLOW				"\033[33m"
#define ANSI_BLUE				"\033[34m"
#define ANSI_MAGENTA			"\033[35m"
#define ANSI_CYAN				"\033[36m"
#define ANSI_WHITE				"\033[37m"

#define ANSI_XTERM_FG			"\033[38;5;"

/* Background colors */

#define ANSI_BBLACK				"\033[40m"
#define ANSI_BRED				"\033[41m"
#define ANSI_BGREEN				"\033[42m"
#define ANSI_BYELLOW			"\033[43m"
#define ANSI_BBLUE				"\033[44m"
#define ANSI_BMAGENTA			"\033[45m"
#define ANSI_BCYAN				"\033[46m"
#define ANSI_BWHITE				"\033[47m"

#define ANSI_XTERM_BG			"\033[48;5;"

/* Numeric-only definitions */

#define N_ANSI_NORMAL			"0"
#define N_ANSI_HILITE			"1"
#define N_ANSI_INVERSE			"7"
#define N_ANSI_BLINK			"5"
#define N_ANSI_UNDER			"4"

#define N_ANSI_BLACK			"30"
#define N_ANSI_RED				"31"
#define N_ANSI_GREEN			"32"
#define N_ANSI_YELLOW			"33"
#define N_ANSI_BLUE				"34"
#define N_ANSI_MAGENTA			"35"
#define N_ANSI_CYAN				"36"
#define N_ANSI_WHITE			"37"

#define N_ANSI_BBLACK			"40"
#define N_ANSI_BRED				"41"
#define N_ANSI_BGREEN			"42"
#define N_ANSI_BYELLOW			"43"
#define N_ANSI_BBLUE			"44"
#define N_ANSI_BMAGENTA			"45"
#define N_ANSI_BCYAN			"46"
#define N_ANSI_BWHITE			"47"

#define N_ANSI_NORMAL			"0"

/* Integers */

#define I_ANSI_NORMAL		0

#define I_ANSI_HILITE		1
#define I_ANSI_INVERSE		7
#define I_ANSI_BLINK		5
#define I_ANSI_UNDER		4

#define I_ANSI_BLACK		30
#define I_ANSI_RED			31
#define I_ANSI_GREEN		32
#define I_ANSI_YELLOW		33
#define I_ANSI_BLUE			34
#define I_ANSI_MAGENTA		35
#define I_ANSI_CYAN			36
#define I_ANSI_WHITE		37

#define I_ANSI_BBLACK		40
#define I_ANSI_BRED			41
#define I_ANSI_BGREEN		42
#define I_ANSI_BYELLOW		43
#define I_ANSI_BBLUE		44
#define I_ANSI_BMAGENTA		45
#define I_ANSI_BCYAN		46
#define I_ANSI_BWHITE		47

#define I_ANSI_NUM			48
#define I_ANSI_LIM			50

#define ANST_NORMAL			0x0099
#define ANST_NONE			0x1099

/* From stringutil.c */
extern int ansi_nchartab[256];
extern char ansi_lettab[I_ANSI_NUM];
extern int ansi_mask_bits[I_ANSI_LIM];
extern int ansi_bits[I_ANSI_LIM];

#endif				/* __ANSI_H */
