#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include "terminal.h"

using namespace std;


Terminal* Terminal::m_terminal = nullptr;


Terminal::Terminal()
{
	m_quit = false;
	tcgetattr(0, &m_startupTerminalSettings);
	UpdateWindowSize();
}


void Terminal::SetupRawMode()
{
	struct termios term = m_startupTerminalSettings;
	term.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON);
	term.c_oflag &= ~OPOST;
	term.c_cflag |= CS8;
	term.c_lflag &= ~(ECHO | ICANON | IEXTEN);
	tcsetattr(0, TCSAFLUSH, &term);
}


void Terminal::WindowChangeCallback(int)
{
	m_terminal->m_sizeChanged = true;
}


void Terminal::QuitCallback(int)
{
	m_terminal->m_quit = true;
}


void Terminal::TerminalReset()
{
	tcsetattr(0, TCSAFLUSH, &m_terminal->m_startupTerminalSettings);
	m_terminal->EndOutputQueue();
	m_terminal->Output("\033[?1049l");
	m_terminal->Output("\033[0m");
	m_terminal->ShowCursor();
}


void Terminal::Init()
{
	m_terminal = new Terminal();
	signal(SIGWINCH, WindowChangeCallback);
	signal(SIGINT, QuitCallback);
	m_terminal->SetupRawMode();
	atexit(TerminalReset);
	m_terminal->Output("\033[?1049h");
}


void Terminal::UpdateWindowSize()
{
	m_sizeChanged = false;

	struct winsize ws;
	if ((!isatty(0)) || (ioctl(0, TIOCGWINSZ, &ws) != 0))
	{
		printf("Unable to determine terminal size. This game must be run from a valid TTY.\n");
		exit(1);
	}

	m_cols = ws.ws_col;
	m_rows = ws.ws_row;
}


void Terminal::BeginOutputQueue()
{
	m_queue = true;
}


void Terminal::Output(const string& contents)
{
	if (m_queue)
	{
		m_queueContents += contents;
		return;
	}

	const char* ptr = contents.c_str();
	size_t len = contents.size();
	while (len > 0)
	{
		int result = write(1, ptr, len);
		if (result == 0)
		{
			if (errno == EINTR)
				continue;
			return;
		}
		len -= result;
		ptr += result;
	}
}


void Terminal::EndOutputQueue()
{
	m_queue = false;
	Output(m_queueContents);
	m_queueContents = "";
}


void Terminal::ClearScreen()
{
	Output("\033[2J");
}


void Terminal::ClearLine()
{
	Output("\033[K");
}


void Terminal::HideCursor()
{
	Output("\033[?25l");
}


void Terminal::ShowCursor()
{
	Output("\033[?25h");
}


void Terminal::SetCursorPosition(size_t x, size_t y)
{
	char cmd[32];
	sprintf(cmd, "\033[%d;%dH", (int)y + 1, (int)x + 1);
	Output(cmd);
}


void Terminal::SetColor(uint8_t foreground, uint8_t background)
{
	char cmd[32];
	sprintf(cmd, "\033[38;5;%dm\033[48;5;%dm", foreground, background);
	Output(cmd);
}


string Terminal::GetInput()
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	if (select(1, &readfds, nullptr, nullptr, &timeout) != 1)
		return "";

	char buf[8];
	int result = read(1, buf, 8);
	if (result <= 0)
		return "";
	return string(buf, (size_t)result);
}


bool Terminal::IsInputUpMovement(const string& input)
{
	return (input == "\033[A") || (input == "w") || (input == "W") || (input == "k") || (input == "K");
}


bool Terminal::IsInputDownMovement(const string& input)
{
	return (input == "\033[B") || (input == "s") || (input == "S") || (input == "j") || (input == "J");
}


bool Terminal::IsInputLeftMovement(const string& input)
{
	return (input == "\033[D") || (input == "a") || (input == "A") || (input == "h") || (input == "H");
}


bool Terminal::IsInputRightMovement(const string& input)
{
	return (input == "\033[C") || (input == "d") || (input == "D") || (input == "l") || (input == "L");
}
