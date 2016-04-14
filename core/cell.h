#ifndef __CELL_H__
#define __CELL_H__

const unsigned BOARD_MAX_PLAYERS = 2;

class Cell
{
public:

	Cell();
	unsigned getDefinite() const;
	unsigned getSuperpositionFlags() const;

	void setToNotAvailable();
	bool isAvailable() const;
	void setToRandom();
	void addFlagNoChecks(unsigned flag);

	bool removeFlag(unsigned flag);
	void collapseTo(unsigned flag);
	void setWinner();
	bool isWinner() const;
	unsigned getDefiniteFlag() const;

	static unsigned setRandSeed(unsigned* pRandSeed = nullptr);
	static int getLowestSetBit(unsigned flags);
	// Removes the lowest set bit and returns its position, -1 if flags is 0.
	static int removeLowestSetBit(unsigned& flags);
	static unsigned numberOfSetBits(unsigned flags);
	static int getLowestSetBitIndex(unsigned flags);
	static int getPlayerTokenIndex(unsigned flags);
	static unsigned pickFlagAtRandom(unsigned flags);
	static unsigned pickFlagsAtRandom(unsigned flags);
	static unsigned getAvailableFlag(unsigned usedFlags, unsigned player);

private:

	int _data;
};

// A set of columns ocupied by a token
typedef char column_t;
class Columns
{
public:
	Columns(column_t columnsFlags = 0);

	operator column_t();

	column_t& get();

	const column_t& get() const;

	// Pick at random from the current selection
	void pickAtRandom();

	static void shuffle(column_t* array, unsigned n);

private:
	column_t _columnsFlags;
};


#endif //__CELL_H__