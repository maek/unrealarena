/*
 * Daemon GPL Source Code
 * Copyright (C) 2016  Unreal Arena
 * Copyright (C) 1999-2010  id Software LLC, a ZeniMax Media company
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

#include "framework/ConsoleField.h"

/*
=============================================================
tty console routines

NOTE: if the user is editing a line when something gets printed to the early
console then it won't look good so we provide CON_Hide and CON_Show to be
called before and after a stdout or stderr output
=============================================================
*/

static bool       stdin_active;
// general flag to tell about tty console mode
static bool       ttycon_on = false;
static int            ttycon_hide = 0;

// some key codes that the terminal may be using, initialised on start up
static int            TTY_erase;
static int            TTY_eof;

static struct termios TTY_tc;

static Console::Field TTY_field(INT_MAX);

void WriteToStdout(const char* text) {
	if (write( STDOUT_FILENO, text, strlen(text)) < 0) {
        Log::Warn("Error writing to the terminal: %s", strerror(errno));
    }
}

/*
=================
CON_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
static void CON_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAXPRINTMSG ];
	int         length = 0;

	// Approximations of g_color_table (q_math.c)
#define A_BOLD 16
#define A_DIM  32
	static const char colour16map[2][32] = {
		{ // Variant 1 (xterm)
			0 | A_BOLD, 1,          2,          3,
			4,          6,          5,          7,
			3 | A_DIM,  7 | A_DIM,  7 | A_DIM,  7 | A_DIM,
			2 | A_DIM,  3 | A_DIM,  4 | A_DIM,  1 | A_DIM,
			3 | A_DIM,  3 | A_DIM,  6 | A_DIM,  5 | A_DIM,
			6 | A_DIM,  5 | A_DIM,  6 | A_DIM,  2 | A_BOLD,
			2 | A_DIM,  1,          1 | A_DIM,  3 | A_DIM,
			3 | A_DIM,  2 | A_DIM,  5,          3 | A_BOLD
		},
		{ // Variant 1 (vte)
			0 | A_BOLD, 1,          2,          3 | A_BOLD,
			4,          6,          5,          7,
			3        ,  7 | A_DIM,  7 | A_DIM,  7 | A_DIM,
			2 | A_DIM,  3,          4 | A_DIM,  1 | A_DIM,
			3 | A_DIM,  3 | A_DIM,  6 | A_DIM,  5 | A_DIM,
			6 | A_DIM,  5 | A_DIM,  6 | A_DIM,  2 | A_BOLD,
			2 | A_DIM,  1,          1 | A_DIM,  3 | A_DIM,
			3 | A_DIM,  2 | A_DIM,  5,          3 | A_BOLD
		}
	};
	static const char modifier[][4] = { "", ";1", ";2", "" };

	int index = com_ansiColor.Get() ? 1 : 0;

	if ( index >= (int) ARRAY_LEN( colour16map ) )
	{
		index = 0;
	}

	while ( *msg )
	{
		if ( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if ( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if ( *msg == '\n' )
			{
				// Issue a reset and then the newline
				fputs( "\033[0;49;37m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				int colour = colour16map[ index ][ ( msg[ 1 ] - '0' ) & 31 ];

				Com_sprintf( buffer, sizeof( buffer ), "\033[%s%d%sm",
				             (colour & 0x30) == 0 ? "0;" : "",
				             30 + ( colour & 15 ), modifier[ ( colour / 16 ) & 3 ] );
				fputs( buffer, stderr );
				msg += 2;
			}
		}
		else
		{
			if ( length >= MAXPRINTMSG - 1 )
			{
				break;
			}

			if ( *msg == Q_COLOR_ESCAPE && msg[1] == Q_COLOR_ESCAPE )
			{
				++msg;
			}

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if ( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

/*
==================
CON_FlushIn

Flush stdin, I suspect some terminals are sending a LOT of shit
FIXME relevant?
==================
*/
static void CON_FlushIn()
{
	char key;

	while ( read( STDIN_FILENO, &key, 1 ) > 0 ) {}
}

/*
==================
CON_Back

Output a backspace

NOTE: it seems on some terminals just sending '\b' is not enough so instead we
send "\b \b"
(FIXME there may be a way to find out if '\b' alone would work though)
==================
*/
static void CON_Back()
{
	WriteToStdout("\b \b");
}

/*
==================
CON_Hide

Clear the display of the line currently edited
bring cursor back to beginning of line
==================
*/
static void CON_Hide()
{
	if ( ttycon_on )
	{
		if ( ttycon_hide )
		{
			ttycon_hide++;
			return;
		}

		for (int i = TTY_field.GetText().size(); i-->0;) {
			CON_Back();
		}

#ifdef UNREALARENA
		CON_Back(); // Delete " "
		CON_Back(); // Delete ">"
#else
		CON_Back(); // Delete "]"
#endif
		ttycon_hide++;
	}
}

/*
==================
CON_Show

Show the current line
FIXME need to position the cursor if needed?
==================
*/
static void CON_Show()
{
	if ( ttycon_on )
	{
		assert( ttycon_hide > 0 );
		ttycon_hide--;

		if ( ttycon_hide == 0 )
		{
#ifdef UNREALARENA
			WriteToStdout("> ");
#else
			WriteToStdout("]");
#endif

			std::string text = Str::UTF32To8(TTY_field.GetText());
			WriteToStdout(text.c_str());
		}
	}
}

/*
==================
CON_Shutdown_TTY

Never exit without calling this, or your terminal will be left in a pretty bad state
==================
*/
void CON_Shutdown_TTY()
{
	if ( ttycon_on )
	{
#ifdef UNREALARENA
		CON_Back(); // Delete " "
		CON_Back(); // Delete ">"
#else
		CON_Back(); // Delete "]"
#endif
		tcsetattr( STDIN_FILENO, TCSADRAIN, &TTY_tc );
	}

	// Restore blocking to stdin reads
	fcntl( STDIN_FILENO, F_SETFL, fcntl( STDIN_FILENO, F_GETFL, 0 ) & ~O_NONBLOCK );
}

/*
==================
CON_SigCont
Reinitialize console input after receiving SIGCONT, as on Linux the terminal seems to lose all
set attributes if user did CTRL+Z and then does fg again.
==================
*/

void CON_SigCont( int )
{
	void CON_Init_TTY();

	CON_Init_TTY();
}

/*
==================
CON_Init_TTY

Initialize the console input (tty mode if possible)
==================
*/
void CON_Init_TTY()
{
	struct termios tc;

	// If the process is backgrounded (running non-interactively),
	// then SIGTTIN or SIGTTOU is emitted; if not caught, turns into a SIGSTP
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	// If SIGCONT is received, reinitialize console
	signal( SIGCONT, CON_SigCont );

	// Make stdin reads non-blocking
	fcntl( STDIN_FILENO, F_SETFL, fcntl( STDIN_FILENO, F_GETFL, 0 ) | O_NONBLOCK );

	const char *term = getenv( "TERM" );
	bool stdinIsATTY = isatty( STDIN_FILENO ) && isatty( STDOUT_FILENO ) && isatty( STDERR_FILENO ) &&
	                   !( term && ( !strcmp( term, "raw" ) || !strcmp( term, "dumb" ) ) );
	if ( !stdinIsATTY )
	{
        Log::Notice( "tty console mode disabled\n" );
		ttycon_on = false;
		stdin_active = true;
		return;
	}

	tcgetattr( STDIN_FILENO, &TTY_tc );
	TTY_erase = TTY_tc.c_cc[ VERASE ];
	TTY_eof = TTY_tc.c_cc[ VEOF ];
	tc = TTY_tc;

	/*
	ECHO: don't echo input characters
	ICANON: enable canonical mode.  This  enables  the  special
	characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	STATUS, and WERASE, and buffers by lines.
	ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	DSUSP are received, generate the corresponding signal
	*/
	tc.c_lflag &= ~( ECHO | ICANON );

	/*
	ISTRIP strip off bit 8
	INPCK enable input parity checking
	*/
	tc.c_iflag &= ~( ISTRIP | INPCK );
	tc.c_cc[ VMIN ] = 1;
	tc.c_cc[ VTIME ] = 0;
	tcsetattr( STDIN_FILENO, TCSADRAIN, &tc );
	ttycon_on = true;
}

/*
==================
CON_Input_TTY
==================
*/
char *CON_Input_TTY()
{
	// we use this when sending back commands
	static char text[ MAX_EDIT_LINE ];
	int         avail;
	char        key;

	if ( ttycon_on )
	{
		avail = read( STDIN_FILENO, &key, 1 );

		if ( avail != -1 )
		{
			// we have something
			// backspace?
			// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
			if ( ( key == TTY_erase ) || ( key == 127 ) || ( key == 8 ) )
			{
				CON_Hide();
				TTY_field.DeletePrev();
				CON_Show();
				CON_FlushIn();
				return nullptr;
			}

			// check if this is a control char
			if ( ( key ) && ( key ) < ' ' )
			{
				if ( key == '\n' )
				{
					TTY_field.RunCommand(com_consoleCommand.Get());
#ifdef UNREALARENA
					WriteToStdout("\n> ");
#else
					WriteToStdout("\n]");
#endif
					return nullptr;
				}

				if ( key == '\t' )
				{
					CON_Hide();
					TTY_field.AutoComplete();
					CON_Show();
					return nullptr;
				}

				if ( key == '\x15' ) // ^U
				{
					CON_Hide();
					TTY_field.Clear();
					CON_Show();
					return nullptr;
				}

				avail = read( STDIN_FILENO, &key, 1 );

				if ( avail != -1 )
				{
					// VT 100 keys
					if ( key == '[' || key == 'O' )
					{
						avail = read( STDIN_FILENO, &key, 1 );

						if ( avail != -1 )
						{
							switch ( key )
							{
								case 'A':
									CON_Hide();
									TTY_field.HistoryPrev();
									CON_Show();
									CON_FlushIn();
									return nullptr;

								case 'B':
									CON_Hide();
									TTY_field.HistoryNext();
									CON_Show();
									CON_FlushIn();
									return nullptr;

								case 'C':
									return nullptr;

								case 'D':
									return nullptr;
							}
						}
					}
				}

				CON_FlushIn();
				return nullptr;
			}

			CON_Hide();
			TTY_field.AddChar(key);
			CON_Show();
		}

		return nullptr;
	}
	else if ( stdin_active )
	{
		int            len;
		fd_set         fdset;
		struct timeval timeout;

		FD_ZERO( &fdset );
		FD_SET( STDIN_FILENO, &fdset );  // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		if ( select( STDIN_FILENO + 1, &fdset, nullptr, nullptr, &timeout ) == -1 || !FD_ISSET( STDIN_FILENO, &fdset ) )
		{
			return nullptr;
		}

		len = read( STDIN_FILENO, text, sizeof( text ) );

		if ( len == 0 )
		{
			// eof!
			stdin_active = false;
			return nullptr;
		}

		if ( len < 1 )
		{
			return nullptr;
		}

		text[ len - 1 ] = 0; // rip off the /n and terminate

		return text;
	}

	return nullptr;
}

/*
==================
CON_Print_TTY
==================
*/
void CON_Print_TTY( const char *msg )
{
	CON_Hide();

	if ( ttycon_on && com_ansiColor.Get() )
	{
		CON_AnsiColorPrint( msg );
	}
	else
	{
		fputs( msg, stderr );
	}

	CON_Show();
}

/* fallbacks for con_curses.c */
#ifndef USE_CURSES
void CON_Init()
{
	CON_Init_TTY();
}

void CON_Shutdown()
{
	CON_Shutdown_TTY();
}

void CON_LogDump()
{
}

char *CON_Input()
{
	return CON_Input_TTY();
}

void CON_Print( const char *message )
{
	CON_Print_TTY( message );
}

void CON_Clear_f()
{
}
#endif
