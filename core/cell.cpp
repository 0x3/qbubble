#include "cell.h"

#include <stdlib.h>

const unsigned BOARD_SUPERPOSITION_FLAG = 0x80000000;
const unsigned BOARD_ALL_FLAGS = ~BOARD_SUPERPOSITION_FLAG;
const unsigned BOARD_DEFINITE_MASK = 0xF;
const unsigned BOARD_DEFINITE_FLAG_INDEX = 8;
const unsigned BOARD_WINNER_TOKEN_MASK = 0x40000000;
const unsigned BOARD_FLAGS_MASK = 0x55555555;

#define RAND_RANGED(range) ((rand() * (range)) / (RAND_MAX + 1))

////////////////////////////////////////////////////////////////////////////////
// Cell
////////////////////////////////////////////////////////////////////////////////

// Negative: non collapsed flags
// Positive: token color
// Zero: empty, not assigned

Cell::Cell() : _data(0)
{
}

// Return the zero based index of the lowest set bit, -1 if value is zero.
int Cell::getLowestSetBit(unsigned value)
{
	// Keep only the lowest bit
	value &= -(signed)(value);
	// Get its position, -1 for zero
	return (-!value) |
		(!(value & 0x55555555) +
		(!(value & 0x33333333) << 1) +
		(!(value & 0x0F0F0F0F) << 2) +
		(!(value & 0x00FF00FF) << 3) +
		(!(value & 0x0000FFFF) << 4));

	// Trick with floating point numbers
	// float f = (float)(value & -value);
	// return (*(unsigned int *)&f >> 23) - 0x7f;
}

unsigned Cell::numberOfSetBits(unsigned i)
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

void Cell::setToRandom()
{
	_data = (rand() & 1) ?
		(rand() | BOARD_SUPERPOSITION_FLAG) : RAND_RANGED(3);
}

unsigned Cell::getDefinite() const
{
	// return (_data > 0) ? _data : 0;
	return (unsigned)(-(_data > 0)) & ((unsigned)_data & BOARD_DEFINITE_MASK);
}

void Cell::setToNotAvailable()
{
	_data = BOARD_ALL_FLAGS;
}

void Cell::setWinner()
{
	_data |= BOARD_WINNER_TOKEN_MASK;
}

bool Cell::isWinner() const
{
	return (_data & BOARD_WINNER_TOKEN_MASK) != 0;
}

unsigned Cell::getDefiniteFlag() const
{
	return (unsigned)(-(_data > 0)) & ((unsigned)_data >> BOARD_DEFINITE_FLAG_INDEX);
}

bool Cell::isAvailable() const
{
	return _data != BOARD_ALL_FLAGS;
}

int Cell::removeLowestSetBit(unsigned& flags)
{
	int index = getLowestSetBit(flags);
	flags &= (flags - 1);
	return index;
}

int Cell::getPlayerTokenIndex(unsigned flags)
{
	return getLowestSetBit(flags & BOARD_ALL_FLAGS);
}

unsigned Cell::getSuperpositionFlags() const
{
	//return (_data < 0) ? _data & BOARD_ALL_FLAGS : 0;
	return (unsigned)(-(_data < 0)) & (unsigned)_data & BOARD_ALL_FLAGS;
}

void Cell::addFlagNoChecks(unsigned flag)
{
	_data |= flag | BOARD_SUPERPOSITION_FLAG;
}

bool Cell::removeFlag(unsigned flag)
{
	if (_data >= 0) return false;
	_data &= ~flag;
	return true;
}

// Sets the token to the player related to the flag
void Cell::collapseTo(unsigned flag)
{
	unsigned player = 0;
	flag &= BOARD_ALL_FLAGS;
	int position = getLowestSetBit(flag);
	while (flag)
	{
		++player;
		if (flag & BOARD_FLAGS_MASK)
		{
			_data = player | (position << BOARD_DEFINITE_FLAG_INDEX);
			return;
		}
		flag >>= 1;
	}
	_data = 0;
}

unsigned Cell::pickFlagAtRandom(unsigned flags)
{
	const unsigned selected = RAND_RANGED(Cell::numberOfSetBits(flags));
	unsigned setFlagIndex = 0;
	unsigned flagIndex = 0;
	while (flags)
	{
		if (flags & 1)
		{
			if (selected == setFlagIndex)
			{
				return 1 << flagIndex;
			}
			++setFlagIndex;
		}
		++flagIndex;
		flags >>= 1;
	}

	return 0;
}

unsigned Cell::getAvailableFlag(unsigned usedFlags, unsigned player)
{
	--player;
	if (player >= BOARD_MAX_PLAYERS) return 0;

	unsigned playerFlagsMask = (BOARD_FLAGS_MASK << player) & BOARD_ALL_FLAGS;
	unsigned availableFlags = (usedFlags & playerFlagsMask) ^ playerFlagsMask;

	// Return only the lowest set bit
	return availableFlags & -(signed)(availableFlags);
}

unsigned Cell::setRandSeed(unsigned* pRandSeed)
{
	unsigned result = pRandSeed ? *pRandSeed : rand();
	srand(result);
	return result;
}

Columns::Columns(column_t columnsFlags) : _columnsFlags(columnsFlags)
{
}

Columns::operator column_t()
{
	return _columnsFlags;
}

column_t& Columns::get()
{
	return _columnsFlags;
}

const column_t& Columns::get() const
{
	return _columnsFlags;
}

void Columns::pickAtRandom()
{
	const unsigned MAX_FLAGS = 2 + RAND_RANGED(2);
	auto columns = _columnsFlags;
	_columnsFlags = 0;

	for (unsigned i = 0; i < MAX_FLAGS; ++i)
	{
		unsigned column = Cell::pickFlagAtRandom(columns);
		_columnsFlags |= column;
		columns ^= column;
	}
}

/* Arrange the N elements of ARRAY in random order.
Only effective if N is much smaller than RAND_MAX;
if this may not be the case, use a better random
number generator. */
void Columns::shuffle(column_t *array, unsigned n)
{
	if (array && (n > 1))
	{
		unsigned i;
		for (i = 0; i < n - 1; ++i)
		{
			unsigned j = i + RAND_RANGED(n - i);
			auto t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}