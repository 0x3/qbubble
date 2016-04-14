#include <windows.h> 
#include <stdio.h> 

#include "console_board.h"

static HANDLE _hStdIn(GetStdHandle(STD_INPUT_HANDLE));
static HANDLE _hStdOut(GetStdHandle(STD_OUTPUT_HANDLE));

#define CHECK_ERROR(x) \
{ \
	if (0 == (x)) \
		{ \
		printf(#x " Error: %u", GetLastError()); \
		getchar(); \
		} \
}

static bool g_keepLogLine = false;

#define LOG(x, ...) \
{ \
	COORD __coord; \
	DWORD __tempOut; \
	__coord.X = 0; \
	__coord.Y = 24; \
	WriteConsoleOutputCharacter(_hStdOut, _text + 80 * 24, 80, __coord, &__tempOut); \
	WriteConsoleOutputAttribute(_hStdOut, _color + 80 * 24, 80, __coord, &__tempOut); \
	SetConsoleTextAttribute(_hStdOut, 0x0e); \
	SetConsoleCursorPos(0, 24); \
	printf(x, __VA_ARGS__); \
	g_keepLogLine = true; \
}


ConsoleBoard::ConsoleBoard()
{
	// Randomizer
	srand(GetTickCount());
	
	// Mouse input
	SetConsoleMode(_hStdIn, ENABLE_MOUSE_INPUT);

	// No echo
	SetConsoleMode(_hStdOut, 0);

	// DOS code page
	SetConsoleOutputCP(437);

	// No scroll bar
	// SetConsoleScreenBufferSize(_hStdOut, COORD{80, 25});
	{
		COORD coord;
		coord.X = 80;
		coord.Y = 25;
		SetConsoleScreenBufferSize(_hStdOut, coord);
	}

	// No cursor
	{
		CONSOLE_CURSOR_INFO ci;
		ci.dwSize = 100;
		ci.bVisible = FALSE;
		SetConsoleCursorInfo(_hStdOut, &ci);
	}

	memset(_text, 0, sizeof(_text));
	memset(_color, 0x0F, sizeof(_color));

	loop();
}

static void SetConsoleCursorPos(int x, int y)
{
	COORD coord;
	coord.X = (short)x;
	coord.Y = (short)y;
	SetConsoleCursorPosition(_hStdOut, coord);
}

void ConsoleBoard::print()
{
	memset(_text, 0, sizeof(_text));
	memset(_color, 0x0F, sizeof(_color));

	switch (_board.getState())
	{
	case GameState::Title:
		break;
	default:
		drawBoard();
		break;
	}

	{
		COORD coord;
		DWORD tempOut;
		coord.X = 0;
		coord.Y = 0;
		unsigned cellCount = 80 * (g_keepLogLine ? 24 : 25);
		WriteConsoleOutputCharacter(_hStdOut, _text, cellCount, coord, &tempOut);
		WriteConsoleOutputAttribute(_hStdOut, _color, cellCount, coord, &tempOut);
		g_keepLogLine = false;
	}

	switch (_board.getState())
	{
	case GameState::Title:
	{
		short int y = 6, x = 32;

		SetConsoleTextAttribute(_hStdOut, 0x0f);
		SetConsoleCursorPos(x, y++);
		printf("QUANTUM 4-INLINE");
		SetConsoleCursorPos(x, y++);
		printf("================");

		SetConsoleTextAttribute(_hStdOut, 0x07);

		x = 12;
		y++;
		SetConsoleCursorPos(x, y++);
		printf("Select a superposition using the numbers from 1 to 7,");
		SetConsoleCursorPos(x, y++);
		printf("at least two positions must be active.");
		SetConsoleCursorPos(x, y++);
		printf("Press [Up] arrow to randomize the superposition.");
		SetConsoleCursorPos(x, y++);
		printf("Use [Left] and [Right] arrows to move the superposition.");
		SetConsoleCursorPos(x, y++);
		printf("Press [Space] or [Down] arrow to drop the superposition.");
		SetConsoleCursorPos(x, y++);
		printf("The first one to connect four in a line wins.");
		SetConsoleCursorPos(x, y++);


		y++;
		SetConsoleCursorPos(x, y++);
		printf("Press [Escape] to exit, [Enter] to start...");
		break;
	}
	case GameState::Celebration:
	{
		short int y = 1, x = 33;
		SetConsoleTextAttribute(_hStdOut, 0x0f);
		SetConsoleCursorPos(x, y++);
		unsigned winner = _board.getWinner();
		if (winner)
		{
			printf(winner == 1 ? "Magenta wins!" : "Yellow wins !");
			SetConsoleCursorPos(x, y++);
			printf("=============");
		}
		else
		{
			printf(" Tied Game ! ", winner);
			SetConsoleCursorPos(x, y++);
			printf(" =========== ");
		}
	}
	case GameState::SelectSuperposition:
	{
		SetConsoleTextAttribute(_hStdOut, 0x07);
		SetConsoleCursorPos(32, 4);
		printf(_board.getState() == GameState::SelectSuperposition ?
			"[ DROP TOKENS ]" : "[  NEXT GAME  ]");
		SetConsoleTextAttribute(_hStdOut, 0x08);
		SetConsoleCursorPos(0, 0);
		printf("%u", _board.getRandSeed());
	}
	default:
		break;
	}

}

void ConsoleBoard::drawBoard()
{
	const MoveResult action = _board.getActionResult();
	unsigned row = 0;
	const Cell* pToken;

	unsigned newColumnFlags = 0;
	unsigned collapsedColumnFlags = 0;
	if (action == MoveResult::SuccessWithCollapse)
	{
		newColumnFlags = action.getOkColumns();
		collapsedColumnFlags = action.getExceptColumns();
	}
	if (action == MoveResult::Success)
	{
		newColumnFlags = action.getOkColumns();
	}

	bool endGame = _board.getState() == GameState::Celebration;

	for (; (pToken = _board.getRow(row)) != 0; ++row)
	{
		// grid
		int y = (23 - row * 3) * 80;
		for (int x = 4; x < 75; ++x)
		{
			int i = y + x;
			char c = (x - 4) % 10 ? '-' : '+';
			_text[i] = c;
			_color[i] = 0x08;

			c = (x - 4) % 10 ? ' ' : '|';
			i -= 80;
			_text[i] = c;
			_color[i] = 0x08;
			i -= 80;
			_text[i] = c;
			_color[i] = 0x08;
		}

		// Tokens
		for (unsigned count = 0; count < BOARD_MAX_COLUMNS; ++count)
		{
			int i = y - 80 + 6 + count * 10;
			unsigned player = pToken[count].getDefinite();
			unsigned short tokenColor = player == 1 ? 0x5D : 0x6E;
			if (endGame)
			{
				tokenColor |= pToken[count].isWinner() ? 0x80 : 0x00;
				tokenColor &= pToken[count].isWinner() ? 0xF0 : 0xFF;
			}
			if (player)
			{
				if (!endGame && (collapsedColumnFlags & (1 << count)))
				{
					if (_board.isTopDefinite(row, count))
					{
						tokenColor &= 0xF0;
						tokenColor |= 0x80;
					}
				}

				char flagChar = (char)(pToken[count].getDefiniteFlag() + '0');
				for (int j = i; j < i + 7; ++j)
				{
					//_text[j] = ' ';
					_text[j] = (j == i + 3) ? flagChar : ' ';
					_text[j - 80] = ' ';
					_color[j] = tokenColor;
					_color[j - 80] = tokenColor;
				}
				continue;
			}
			i -= 80;

			unsigned superposition = pToken[count].getSuperpositionFlags();
			while (superposition)
			{
				const int DELTA_POSITIONS[] = {3, 4, 2, 5, 1, 6, 0};
				int index = Cell::removeLowestSetBit(superposition);
				int pos = i + DELTA_POSITIONS[index % 7];
				pos += (index >= 7) * 80;
				_text[pos] = (char)('0' + index);
				unsigned short tokenColor = (index % BOARD_MAX_PLAYERS) ? 0x06 : 0x05;
				if ((newColumnFlags & (1 << count)))
				{
					if (_board.isTopSuperposition(row, count))
					{
						tokenColor |= 0x08;
					}
				}
				_color[pos] = tokenColor;
			}
		}
	}

	unsigned player = _board.getPlayer();
	unsigned superPosition = _board.getSuperPosition();
	unsigned tokenNumber = _board.getTokenIndexToUse();
	char tokenChar = (char)('0' + tokenNumber);

	unsigned columnFlags = 0;
	if (action == MoveResult::PositionsNotAvailable)
	{
		columnFlags = action.getExceptColumns();
	}

	if (!endGame)
	{
		for (int i = 0; i < 4; ++i)
		{
			int j = 80;
			_text[i + j] = '<';
			_color[i + j] = 0x07;
			j = 80 + 76;
			_text[i + j] = '>';
			_color[i + j] = 0x07;
		}

		for (unsigned count = 0; count < BOARD_MAX_COLUMNS; ++count)
		{
			int i = 80 + (6 + count * 10);
			for (int j = i; j < i + 7; ++j)
			{
				if (0 == (superPosition & (1 << count)))
				{
					_text[j] = '-';
					_color[j] = (player == 1) ? 0x05 : 0x06;
				}
				else
				{
					_text[j] = (j == i + 3) ? tokenChar : ' ';
					if (columnFlags & (1 << count))
					{
						_color[j] = (player == 1) ? 0x0D : 0x0E;
					}
					else
					{
						_color[j] = (player == 1) ? 0xd0 : 0xe0;
					}
					_text[j + 80] = '-';
					_color[j + 80] = (player == 1) ? 0x05 : 0x06;
				}
			}
		}
	}
}

void ConsoleBoard::start()
{
	_board.start();
	print();
}

void ConsoleBoard::drop()
{
	if (!_board.drop())
	{
		LOG(_board.getLastError());
	}
	print();
}

void ConsoleBoard::flip(unsigned column)
{
	if (!_board.flipSuperPositionFlag(column))
	{
		LOG("Invalid column");
	}
	print();
}

void ConsoleBoard::random()
{
	_board.selectSuperpositionAtRandom();
	print();
}

void ConsoleBoard::left()
{
	if ((_board.getSuperPosition() & 1) == 0)
	{
		_board.setSuperPosition(_board.getSuperPosition() >> 1);
		print();
	}
}

void ConsoleBoard::right()
{
	if ((_board.getSuperPosition() &
		(1 << (BOARD_MAX_COLUMNS - 1))) == 0)
	{
		_board.setSuperPosition((_board.getSuperPosition() << 1) & 0x7f);
		print();
	}
}

void ConsoleBoard::loop()
{
	INPUT_RECORD ir;
	unsigned long readCount = 0;
	print();

	// Main loop
	for (;;)
	{
		if (!ReadConsoleInput(_hStdIn, &ir, 1, &readCount)) continue;
		
		if (ir.EventType == MOUSE_EVENT)
		{
			MOUSE_EVENT_RECORD& er = ir.Event.MouseEvent;
			if ((er.dwEventFlags != MOUSE_MOVED) &&
				(er.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED))
			{
				switch (_board.getState())
				{
					case GameState::Title:
					{
						start();
						break;
					}
					case GameState::Celebration:
					{
						if ((er.dwMousePosition.Y >= 4) && (er.dwMousePosition.Y <= 5))
						{
							int col = er.dwMousePosition.X;
							if ((col >= 31) && (col <= 47))
							{
								start();
							}
						}
						break;
					}
					case GameState::SelectSuperposition:
					{
						if ((er.dwMousePosition.Y == 1) || (er.dwMousePosition.Y == 2))
						{
							int col = er.dwMousePosition.X;
							if (col <= 3)
							{
								left();
							}
							else if (col >= 76)
							{
								right();
							}
							else
							{
								col = (er.dwMousePosition.X + 5) % 10;
								if (col >= 1 && col <= 7)
								{
									flip((er.dwMousePosition.X - 6) / 10);
								}
							}
						}
						else if ((er.dwMousePosition.Y >= 4) && (er.dwMousePosition.Y <= 5))
						{
							int col = er.dwMousePosition.X;
							if ((col >= 31) && (col <= 47))
							{
								drop();
							}
						}
						break;
					}
				}
			}
		}
		else if (ir.EventType == KEY_EVENT)
		{
			KEY_EVENT_RECORD& er = ir.Event.KeyEvent;
			if (er.bKeyDown && er.wVirtualKeyCode == VK_ESCAPE) break;

			if (er.bKeyDown)
			{
				switch (_board.getState())
				{
					case GameState::Title:
					case GameState::Celebration:
					{
						unsigned k = er.wVirtualKeyCode;
						if (k == VK_RETURN)
						{
							start();
						}
						break;
					}
					case GameState::SelectSuperposition:
					{
						short unsigned code = er.wVirtualKeyCode;
						char c = er.uChar.AsciiChar;
						switch (code)
						{
							case VK_UP: c = 'x'; break;
							case VK_DOWN: c = ' '; break;
							case VK_LEFT: c = 'a'; break;
							case VK_RIGHT: c = 's'; break;
						}

						if (c >= '1' && c <= '7')
						{
							flip(c - '1');
						}
						else if (c == ' ')
						{
							drop();
						}
						else if (c == 'x')
						{
							random();
						}
						else if (c == 'a')
						{
							left();
						}
						else if (c == 's')
						{
							right();
						}
						else if (c == 'z')
						{
							_board.setSuperPosition(3);
							print();
						}
						break;
					}
				}
			}
		}
	}
}



