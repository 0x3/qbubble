//#define _CRT_SECURE_NO_WARNINGS

#include "board.h"

#include <memory.h>

const int BOARD_VICTORY_TOKENS = 4;


////////////////////////////////////////////////////////////////////////////////
// Board
////////////////////////////////////////////////////////////////////////////////

Board::Board() : _gameState(GameState::Title), _actionResult(MoveResult::None),
	_superPosition(0), _turn(0), _flagToUse(0), _winner(0), _randSeed(0)
{
}

GameState Board::getState() const
{
	return _gameState;
}

const MoveResult Board::getActionResult()
{
	auto temp = _actionResult;
	_actionResult = MoveResult::None;
	return temp;
}

unsigned Board::getWinner() const
{
	return _winner;
}

unsigned Board::getPlayer() const
{
	return 1 + (_turn % BOARD_MAX_PLAYERS);
}

Columns Board::getSuperPosition() const
{
	return _superPosition;
}

unsigned Board::getTokenIndexToUse() const
{
	return Cell::getPlayerTokenIndex(_flagToUse);
}

bool Board::isTopDefinite(unsigned row, unsigned column) const
{
	unsigned index = row * BOARD_MAX_STORED_COLUMNS + column;
	if (index >= BOARD_MAX_TOKENS) return false;
	if (!_cells[index].getDefinite()) return false;
	if (row == BOARD_MAX_ROWS - 1) return true;
	index += BOARD_MAX_STORED_COLUMNS;
	return !_cells[index].getDefinite();
}

bool Board::isTopSuperposition(unsigned row, unsigned column) const
{
	unsigned index = row * BOARD_MAX_STORED_COLUMNS + column;
	if (index >= BOARD_MAX_TOKENS) return false;
	return _cells[index].getSuperpositionFlags() != 0;
}

bool Board::hasValidSuperposition() const
{
	if (_superPosition.get() & ~BOARD_ALL_COLUMNS_FLAGS) return false;
	auto bitsCount = Cell::numberOfSetBits(_superPosition.get());
	if (bitsCount >= 2) return true;
	if (bitsCount && onlyOneColumnFree()) return true;
	return false;
}

unsigned Board::getRandSeed() const
{
	return _randSeed;
}

// Set the row ready for updates and returns the used flags in it
unsigned Board::getReadyRow()
{
	unsigned usedFlags = 0;

	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		_rowIndex[i] = 0;
		_row._cells[i].setToNotAvailable();

		for (int j = 0; j < BOARD_MAX_ROWS; ++j)
		{
			Cell token = _cells[j * BOARD_MAX_STORED_COLUMNS + i];
			if (token.getDefinite() == 0)
			{
				usedFlags |= token.getSuperpositionFlags();
				_row._cells[i] = token;
				_rowIndex[i] = j;
				break;
			}
		}
	}

	return usedFlags;
}

void Board::setReadyRow()
{
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if (_row._cells[i].isAvailable())
		{
			_cells[_rowIndex[i] * BOARD_MAX_STORED_COLUMNS + i] = _row._cells[i];
		}
	}
}

const Cell* Board::getRow(unsigned row) const
{
	if (row >= BOARD_MAX_ROWS) return nullptr;
	return _cells + (row * BOARD_MAX_STORED_COLUMNS);
}

void Board::prepareNextTurn()
{
	if (_gameState != GameState::SelectSuperposition)
	{
		//unsigned seed = 3365;
		_randSeed = Cell::setRandSeed();
		_turn = 0;
		memset(_cells, 0, sizeof(_cells));
		_row.clear();
	}
	else
	{
		++_turn;
	}

	unsigned usedFlags = getReadyRow();
	_superPosition = 0;
	//selectSuperpositionAtRandom();
	_flagToUse = Cell::getAvailableFlag(usedFlags, getPlayer());
	_gameState = GameState::SelectSuperposition;
}

unsigned Board::getSelectedFullColumns() const
{
	unsigned result = 0;
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if ((_superPosition.get() & (1 << i)) && !_row._cells[i].isAvailable())
		{
			result |= 1 << i;
		}
	}
	return result;
}

bool Board::onlyOneColumnFree() const
{
	unsigned freeCount = 0;
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if (_row._cells[i].isAvailable())
		{
			++freeCount;
		}
	}
	return 1 == freeCount;
}

bool Board::addSuperPosition()
{
	// Check if flag not available (Shouldn't happen)
	if (!_flagToUse)
	{
		_pLastError = "No more tokens available!";
		_actionResult = MoveResult::NoMoreTokens;
		return false;
	}

	// Check superposition
	if (!hasValidSuperposition())
	{
		_pLastError = "Select at least two columns.";
		_actionResult = MoveResult::InvalidSuperposition;
		return false;
	}

	// Check for full columns
	unsigned fullColumns = getSelectedFullColumns();
	if (fullColumns)
	{
		_pLastError = "Choose columns with free slots.";
		_actionResult = MoveResult(MoveResult::PositionsNotAvailable, 0, fullColumns);
		return false;
	}

	Row newState(_row);
	newState.addSuperPosition(_superPosition, _flagToUse);
	Row collapsedState(newState);
	if (!collapsedState.collapse(_superPosition))
	{
		// We need to collapse before adding new tokens
		collapsedState = _row;
		if (!collapsedState.collapse(_superPosition))
		{
			_pLastError = "Impossible to define positions!!";
			_actionResult = MoveResult::ImpossibleToCollapse;
			return false;
		}

		unsigned collapsedColumns = _row.getCollapsedColumns(collapsedState);
		// Apply collapse
		_row = collapsedState;
		setReadyRow();

		// Check for game over
		if (detectGameOver())
		{
			getReadyRow();
			_row.addSuperPosition(_superPosition, _flagToUse, true);
			setReadyRow();
			_actionResult = MoveResult(MoveResult::SuccessWithCollapse, _superPosition, collapsedColumns);
			return true;
		}

		getReadyRow();
		if (onlyOneColumnFree())
		{
			selectSuperpositionAtRandom();
		}
		bool result = addSuperPosition();
		_actionResult = MoveResult(MoveResult::SuccessWithCollapse, _superPosition, collapsedColumns);
		return result;
	}

	// Valid, check for full board
	_row = collapsedState;
	setReadyRow();
	if (isFull())
	{
		unsigned collapsedColumns = newState.getCollapsedColumns(collapsedState);
		_actionResult = MoveResult(MoveResult::SuccessWithCollapse, _superPosition, collapsedColumns);
		return true;
	}
	// Valid but not full, revert before collapse
	_row = newState;
	setReadyRow();
	_actionResult = MoveResult(MoveResult::Success, _superPosition, _superPosition);
	return true;
}

bool Board::detectGameOver()
{
	_winner = 0;

	unsigned collapsedBoard[BOARD_MAX_TOKENS];
	for (int i = 0; i < BOARD_MAX_TOKENS; ++i)
	{
		collapsedBoard[i] = _cells[i].getDefinite();
	}

	unsigned previous = 0;
	unsigned count = 0;
	unsigned lines[BOARD_MAX_PLAYERS + 1] = { 0 };

	// check rows
	for (int j = 0; j < BOARD_MAX_ROWS; ++j)
	{
		previous = 0;
		for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
		{
			unsigned index = i + j * BOARD_MAX_STORED_COLUMNS;
			unsigned current = collapsedBoard[index];
			if (previous && (current == previous))
			{
				++count;
				if (count == BOARD_VICTORY_TOKENS)
				{
					markWinerTokens(index, -1);
					++lines[previous];
					// _winner = previous;
					// return true;
				}
			}
			else
			{
				previous = current;
				count = 1;
			}
		}
	}

	// check columns
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		previous = 0;
		for (int j = 0; j < BOARD_MAX_ROWS; ++j)
		{
			unsigned index = i + j * BOARD_MAX_STORED_COLUMNS;
			unsigned current = collapsedBoard[index];
			if (previous && (current == previous))
			{
				++count;
				if (count == BOARD_VICTORY_TOKENS)
				{
					markWinerTokens(index, -BOARD_MAX_STORED_COLUMNS);
					++lines[previous];
					//_winner = previous;
					//return true;
				}
			}
			else
			{
				previous = current;
				count = 1;
			}
		}
	}

	// check '/' (as displayed)
	for (int i = BOARD_VICTORY_TOKENS - BOARD_MAX_ROWS; i <= BOARD_MAX_COLUMNS - BOARD_VICTORY_TOKENS; ++i)
	{
		previous = 0;
		for (int j = 0; j < BOARD_MAX_ROWS; ++j)
		{
			int x = i + j;
			if ((x < 0) || (x >= BOARD_MAX_COLUMNS)) continue;

			unsigned index = x + j * BOARD_MAX_STORED_COLUMNS;
			unsigned current = collapsedBoard[index];
			if (previous && (current == previous))
			{
				++count;
				if (count == BOARD_VICTORY_TOKENS)
				{
					markWinerTokens(index, -BOARD_MAX_STORED_COLUMNS - 1);
					++lines[previous];
					//_winner = previous;
					//return true;
				}
			}
			else
			{
				previous = current;
				count = 1;
			}
		}
	}

	// check '\' (as displayed)
	for (int i = BOARD_VICTORY_TOKENS - 1; i < BOARD_MAX_COLUMNS + BOARD_MAX_ROWS - BOARD_VICTORY_TOKENS; ++i)
	{
		previous = 0;
		for (int j = 0; j < BOARD_MAX_ROWS; ++j)
		{
			int x = i - j;
			if ((x < 0) || (x >= BOARD_MAX_COLUMNS)) continue;

			unsigned index = x + j * BOARD_MAX_STORED_COLUMNS;
			unsigned current = collapsedBoard[index];
			if (previous && (current == previous))
			{
				++count;
				if (count == BOARD_VICTORY_TOKENS)
				{
					markWinerTokens(index, -BOARD_MAX_STORED_COLUMNS + 1);
					++lines[previous];
				}
			}
			else
			{
				previous = current;
				count = 1;
			}
		}
	}

	// If there are lines
	if (lines[1] | lines[2])
	{
		// 0 if both players are tied, otherwise player id
		_winner = (lines[1] != lines[2]) ? ((lines[1] < lines[2]) + 1) : 0;
		return true;
	}

	return isFull();
}

bool Board::isFull() const
{
	const unsigned LAST_ROW = (BOARD_MAX_ROWS - 1) * BOARD_MAX_STORED_COLUMNS;
	// Check full board
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if (_cells[i + LAST_ROW].getDefinite() == 0)
		{
			return false;
		}
	}
	return true;
}

void Board::markWinerTokens(unsigned pos, int delta)
{
	for (int i = 0; i < BOARD_VICTORY_TOKENS; ++i)
	{
		if (pos < BOARD_MAX_TOKENS) _cells[pos].setWinner();
		pos += delta;
	}
}

bool Board::setSuperPosition(Columns columns)
{
	if (columns & ~BOARD_ALL_COLUMNS_FLAGS) return false;
	_superPosition = columns;
	return true;
}

bool Board::flipSuperPositionFlag(unsigned flag)
{
	if (flag >= BOARD_MAX_COLUMNS) return false;
	_superPosition.get() ^= 1 << flag;
	return true;
}

void Board::selectSuperpositionAtRandom()
{
	Columns columns = 0;
	for (int i = 0; i < BOARD_MAX_COLUMNS; ++i)
	{
		if (_row._cells[i].isAvailable()) columns.get() |= 1 << i;
	}
	columns.pickAtRandom();
	_superPosition = columns;
}

void Board::start()
{
	prepareNextTurn();
}

bool Board::drop()
{
	if (!addSuperPosition())
	{
		return false;
	}

	if (detectGameOver())
	{
		_gameState = GameState::Celebration;
		_superPosition = 0;
	}
	else
	{
		prepareNextTurn();
	}

	return true;
}

const char* Board::getLastError()
{
	const char* result = _pLastError;
	_pLastError = "";
	return result;
}