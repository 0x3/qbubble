#include "row.h"
#include <memory.h>

column_t& Tokens::operator[](unsigned index)
{
	return _columns[index].get();
}

const column_t& Tokens::operator[](unsigned index) const
{
	return _columns[index].get();
}

void Tokens::shuffle(unsigned count)
{
	Columns::shuffle((column_t*)_columns, count);
}

bool Tokens::collapse(unsigned tokenCount)
{
	// All tokens collapsed
	if (0 == tokenCount) return true;

	// Get all the columns ocupied by the current token
	--tokenCount;
	unsigned columnsFlags = _columns[tokenCount];
	if (0 == columnsFlags) return false;

	Tokens notEmptyColumns;
	unsigned notEmptyColumnsCount = 0;
	do 
	{
		unsigned oldState = columnsFlags;
		// Remove lowest set bit, each bit represents a column
		columnsFlags &= columnsFlags - 1;
		notEmptyColumns[notEmptyColumnsCount++] = (column_t)(oldState ^ columnsFlags);
	}
	while (columnsFlags);

	// Don't prefer one column over any other and check all permutations
	notEmptyColumns.shuffle(notEmptyColumnsCount);
	for (unsigned i = 0; i < notEmptyColumnsCount; ++i)
	{
		auto currentColumn = notEmptyColumns[i];
		Tokens oldSate(*this);

		// Collapse current token into one position
		for (unsigned j = 0; j < tokenCount; ++j)
		{
			_columns[j].get() &= ~currentColumn;
		}
		_columns[tokenCount] = currentColumn;

		// Collapse the remaining tokens
		if (collapse(tokenCount))
		{
			return true;
		}

		*this = oldSate;
	}

	return false;
}

void Row::clear()
{
	memset(_cells, 0, sizeof(_cells));
}

Columns Row::getCollapsedColumns(const Row& afterCollapse)
{
	Columns result = 0;
	for (unsigned i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if (_cells[i].getDefinite()) continue;
		if (afterCollapse._cells[i].getDefinite())
		{
			result.get() |= 1 << i;
		}
	}
	return result;
}

// Cheks if the columns specified by column flags are already full and can't
// store another token.
bool Row::collapse(Columns columnFlags)
{
	if (!columnFlags) return true;

	Row temp(*this);

	// Get all the flags involved in the collapse
	unsigned maxColumns = BOARD_MAX_COLUMNS;
	unsigned usedFlags = 0;
	for (unsigned i = BOARD_MAX_COLUMNS; i-- > 0;)
	{
		unsigned flag = 1 << i;
		if (0 == (flag & columnFlags)) continue;

		usedFlags |= temp._cells[i].getSuperpositionFlags();
		--maxColumns;
		temp._cells[i] = temp._cells[maxColumns];
	}

	// Add those indirectly related
	unsigned currentFlags, col = 0;
	while (col < maxColumns)
	{
		currentFlags = temp._cells[col].getSuperpositionFlags();
		if (currentFlags & usedFlags)
		{
			usedFlags |= currentFlags;
			--maxColumns;
			temp._cells[col] = temp._cells[maxColumns];
			col = 0;
		}
		else
		{
			++col;
		}
	}

	// Fill an structure containing the columns ocupied by each flag
	Tokens flagsColumns;
	unsigned columnsFlagsAffected = 0;
	for (unsigned i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		unsigned columnFlag = 1 << i;
		unsigned flagIndex = 0;
		unsigned flags = usedFlags & _cells[i].getSuperpositionFlags();
		unsigned tempUsedFlags = usedFlags;
		while (flags)
		{
			if (tempUsedFlags & 1)
			{
				if (flags & 1)
				{
					flagsColumns[flagIndex] |= columnFlag;
					columnsFlagsAffected |= columnFlag;
				}
				++flagIndex;
			}
			tempUsedFlags >>= 1;
			flags >>= 1;
		}
	}

	// If collapse was possible update the row information
	if (flagsColumns.collapse(Cell::numberOfSetBits(usedFlags)))
	{
		// Set all the affected cells to zero, before adding the collapsed states
		{
			unsigned columnIndex = 0;
			while (columnsFlagsAffected)
			{
				if (columnsFlagsAffected & 1)
				{
					_cells[columnIndex].collapseTo(0);
				}
				++columnIndex;

				columnsFlagsAffected >>= 1;
			}
		}

		unsigned flagsColumnsIndex = 0;
		unsigned tokenFlagIndex = 0;
		unsigned tempUsedFlags = usedFlags;
		while (tempUsedFlags)
		{
			if (tempUsedFlags & 1)
			{
				unsigned tokenFlag = 1 << tokenFlagIndex;
				unsigned columnFlag = flagsColumns[flagsColumnsIndex];
				if (columnFlag)
				{
					unsigned columnIndex = Cell::getLowestSetBit(columnFlag);
					if (columnIndex < BOARD_MAX_COLUMNS)
					{
						_cells[columnIndex].collapseTo(tokenFlag);
					}
				}
				++flagsColumnsIndex;
			}
			++tokenFlagIndex;
			tempUsedFlags >>= 1;
		}
		return true;
	}

	return false;
}

void Row::addSuperPosition(Columns columnFlags, unsigned tokenFlagToAdd, bool endGame)
{
	unsigned indexUsed = 0;
	unsigned flagsAdded = 0;
	for (unsigned i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if ((columnFlags & (1 << i)) && (0 == _cells[i].getDefinite()))
		{
			_cells[i].addFlagNoChecks(tokenFlagToAdd);
			++flagsAdded;
			indexUsed = i;
		}
	}

	if ((1 == flagsAdded) && !endGame &&
		(1 == Cell::numberOfSetBits(_cells[indexUsed].getSuperpositionFlags())))
	{
		_cells[indexUsed].collapseTo(tokenFlagToAdd);
	}
}