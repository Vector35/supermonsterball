#pragma once

#include <stddef.h>
#include <inttypes.h>
#include <termios.h>
#include <string>

class Terminal
{
	static Terminal* m_terminal;
	volatile bool m_sizeChanged, m_quit;
	size_t m_cols, m_rows;

	struct termios m_startupTerminalSettings;

	bool m_queue;
	std::string m_queueContents;

	void SetupRawMode();

public:
	Terminal();
	static void Init();
	static Terminal* GetTerminal() { return m_terminal; }
	static void WindowChangeCallback(int sig);
	static void QuitCallback(int sig);
	static void TerminalReset();

	size_t GetWidth() { return m_cols; }
	size_t GetHeight() { return m_rows; }
	bool HasSizeChanged() { return m_sizeChanged; }
	bool HasQuit() { return m_quit; }

	void UpdateWindowSize();

	void BeginOutputQueue();
	void Output(const std::string& contents);
	void EndOutputQueue();

	void ClearScreen();
	void ClearLine();
	void HideCursor();
	void ShowCursor();
	void SetCursorPosition(size_t x, size_t y);

	void SetColor(uint8_t foreground, uint8_t background);

	std::string GetInput();
	bool IsInputUpMovement(const std::string& input);
	bool IsInputDownMovement(const std::string& input);
	bool IsInputLeftMovement(const std::string& input);
	bool IsInputRightMovement(const std::string& input);
};
