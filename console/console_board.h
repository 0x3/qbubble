#ifndef __CONSOLE_BOARD_H__
#define __CONSOLE_BOARD_H__

#include "../core/board.h"

class ConsoleBoard
{
public:
	ConsoleBoard();

private:

	void loop();
	void print();
	void drawBoard();

	void start();
	void drop();
	void flip(unsigned column);
	void random();
	void left();
	void right();


	// Fields
	char _text[80 * 25];
	unsigned short _color[80 * 25];
	Board _board;
};


#endif