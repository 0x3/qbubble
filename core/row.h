#ifndef __ROW_H__
#define __ROW_H__

#include "cell.h"

const int BOARD_MAX_STORED_COLUMNS = 8;
const int BOARD_MAX_COLUMNS = BOARD_MAX_STORED_COLUMNS - 1;
const int BOARD_ALL_COLUMNS_FLAGS = (1 << BOARD_MAX_COLUMNS) - 1;

// A group of tokens, each element describes the columns ocupied by each token
class Tokens
{
public:
	column_t& operator[] (unsigned index);

	const column_t& operator[] (unsigned index) const;

	bool collapse(unsigned tokenCount);

	void shuffle(unsigned count);

private:

	// The columns related to each token
	Columns _columns[sizeof(Cell) * 8 /* bits in a byte */];
};

// A group of cells. Each cell contains 0 to N tokens in superposition or only
// one collapsed in a definitive position.
struct Row
{
public:
	Cell _cells[BOARD_MAX_COLUMNS];

	bool collapse(Columns columnFlags);

	void addSuperPosition(Columns columnFlags, unsigned tokenFlagToAdd,
		bool endGame = false);

	Columns getCollapsedColumns(const Row& afterCollapse);

	void clear();
};

#endif //__ROW_H__
