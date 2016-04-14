#ifndef __BOARD_H__
#define __BOARD_H__

#include "row.h"

const int BOARD_MAX_ROWS = 6;
const int BOARD_MAX_TOKENS = BOARD_MAX_ROWS * BOARD_MAX_STORED_COLUMNS;

//enum class GameState : unsigned
//{
//	Title = 0,
//	SelectSuperposition,
//	Celebration,
//};

class GameState
{
public:
	enum GameStateEnum
	{
		Title = 0,
		Celebration,
		SelectSuperposition,
	};

	GameState(GameStateEnum state) : _state(state)
	{
	}

	operator GameStateEnum() const
	{
		return (GameStateEnum)_state;
	}

private:
	unsigned _state;
};

class MoveResult
{
public:
	enum MoveResultEnum
	{
		// _okColumns: afected columns
		// _exceptColumns: full columns
		PartialSuccess = 0,
		// _okColumns: afected columns
		// _exceptColumns: collapsed columns
		SuccessWithCollapse,
		// _okColumns: afected columns
		// _exceptColumns: 0
		Success,

		// _okColumns: 0
		// _exceptColumns: 0
		InvalidSuperposition,
		// _okColumns: 0
		// _exceptColumns: 0
		NoMoreTokens,
		// _okColumns: 0
		// _exceptColumns: not available columns
		PositionsNotAvailable,
		// _okColumns: 0
		// _exceptColumns: 0
		ImpossibleToCollapse,
		// _okColumns: 0
		// _exceptColumns: 0
		None,
	};

	MoveResult(MoveResultEnum state, unsigned okColumns = 0, unsigned exceptColumns = 0) :
		_state(state), _okColumns(okColumns), _exceptColumns(exceptColumns)
	{
	}

	operator bool() const
	{
		return _state <= Success;
	}

	operator MoveResultEnum() const
	{
		return (MoveResultEnum)_state;
	}

	unsigned getOkColumns() const
	{
		return _okColumns;
	}

	unsigned getExceptColumns() const
	{
		return _exceptColumns;
	}

private:
	unsigned _state;
	unsigned _okColumns;
	unsigned _exceptColumns;
};

class Board
{
public:
	Board();
	GameState getState() const;
	// Sets the internal action result to none
	const MoveResult getActionResult();

	unsigned getWinner() const;
	unsigned getPlayer() const;
	Columns getSuperPosition() const;
	unsigned getTokenIndexToUse() const;
	const Cell* getRow(unsigned row) const;

	bool isTopDefinite(unsigned row, unsigned column) const;
	bool isTopSuperposition(unsigned row, unsigned column) const;

	unsigned getRandSeed() const;

	void start();
	bool drop();
	
	void selectSuperpositionAtRandom();

	// Returns true iff the parameter is valid
	bool flipSuperPositionFlag(unsigned flag);
	bool setSuperPosition(Columns columns);

	const char* getLastError();
private:

	// Gets the helper row and returns all the used flags
	unsigned getReadyRow();

	// Sets the helper row and returns true iff board not full
	void setReadyRow();

	void prepareNextTurn();
	bool detectGameOver();
	bool addSuperPosition();

	bool hasValidSuperposition() const;

	bool isFull() const;
	unsigned getSelectedFullColumns() const;
	bool onlyOneColumnFree() const;

	void markWinerTokens(unsigned pos, int delta);

	Cell _cells[BOARD_MAX_TOKENS];
	Row _row;
	unsigned _rowIndex[BOARD_MAX_COLUMNS];
	GameState _gameState;
	MoveResult _actionResult;
	Columns _superPosition;
	unsigned _turn;
	unsigned _flagToUse;
	unsigned _winner;
	unsigned _randSeed;
	const char* _pLastError = "";
};

#endif