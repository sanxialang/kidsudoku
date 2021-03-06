#include "SudokuUtil.h"
#include <math.h>

#define COUNT(a) bcounts[(a & m_uMask)]
#define IS_PRESET(a) ((a & PRESET_BIT) == PRESET_BIT)

static SudokuSolver* s_pSolver = 0;
static SudokuGenerator* s_pGenerator = 0;

static const unsigned char bcounts[512] = {
       0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
       1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
       1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
       1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
       2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
       3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
       3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
       4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,5,6,6,7,6,7,7,8,6,7,7,8,7,8,8,9
};

static const unsigned short bmasks[9] = { 0x01, 0x03, 0x07, 0x0f, 0x01f, 0x03f, 0x07f, 0x0ff, 0x01ff};

SudokuSolver::SudokuSolver()
    : m_iMaxSCount(1),
      m_uMask(0x1FF),
      m_uNumbers(0) {
    memset(m_pSolution, 0, MAX_CELLS);
}

SudokuSolver::SudokuSolver(const SudokuSolver* solver) {
    m_iMaxSCount = solver->m_iMaxSCount;
    m_uMask = solver->m_uMask;

    m_uNumbers = solver->m_uNumbers;
    m_uRows = solver->m_uRows;
    m_uCols = solver->m_uCols;

    m_uGridRows = solver->m_uGridRows;
    m_uGridCols = solver->m_uGridCols;

    m_uGridsInRow = solver->m_uGridsInRow;
    m_uGridsInCol = solver->m_uGridsInCol;

    m_iSolutionCount = solver->m_iSolutionCount;
    memcpy(m_pSolution, solver->m_pSolution, MAX_CELLS);

    memcpy(m_pRowConstraints, solver->m_pRowConstraints, sizeof(unsigned short) * MAX_ROWS);
    memcpy(m_pColConstraints, solver->m_pColConstraints, sizeof(unsigned short) * MAX_COLS);
    memcpy(m_pBlockConstraints, solver->m_pBlockConstraints, sizeof(unsigned short) * MAX_BLOCKS);
    memcpy(m_pCellConstraints, solver->m_pCellConstraints, sizeof(unsigned short) * MAX_CELLS);
}

SudokuSolver::~SudokuSolver() {
}

SudokuSolver* SudokuSolver::getInstance() {
    if (!s_pSolver) {
        s_pSolver = new SudokuSolver();
    }
    return s_pSolver;
}

int SudokuSolver::search(int k, bool oneShot) {
    //check if search traversed each element.
    if (k >= m_uCols * m_uRows)
        return 1;

    if (IS_PRESET(m_pCellConstraints[k]))
        return search(k + 1, oneShot);

    //if it is a preset cell, just go to next step
    int row = k / m_uCols;
    int col = k % m_uCols;
    int grid = (row / m_uGridRows) * m_uGridsInCol + col / m_uGridCols;
    unsigned short constraint = (m_pCellConstraints[k] & m_pRowConstraints[row] & m_pColConstraints[col] & m_pBlockConstraints[grid]);

    int count = COUNT(constraint);
    if (count == 0)
        return 0;

    int scnt = 0;
    for (int i = 0; i < count; i++) {
        unsigned short num = getValue(constraint, i);
        if (!supose(row, col, num))
            continue;
        scnt += search(k+1, oneShot);
        if ((oneShot && scnt > 0) || scnt > m_iMaxSCount)
            return scnt;

        resume(row, col, num);
    }

    return scnt;
}

void SudokuSolver::setSize(short rows_per_grid, short cols_per_grid, short grids_in_row, short grids_in_col) {
    m_uNumbers = rows_per_grid * cols_per_grid;
    m_uRows = rows_per_grid * grids_in_row;
    m_uCols = cols_per_grid * grids_in_col;

    m_uGridCols = cols_per_grid;
    m_uGridRows = rows_per_grid;
    m_uGridsInCol = grids_in_col;
    m_uGridsInRow = grids_in_row;

    memset(m_pSolution, 0, MAX_CELLS);
    m_uMask = bmasks[m_uNumbers - 1];
    initConstraints();
}

void SudokuSolver::initConstraints() {
    for (int i = 0; i < MAX_NUMBER; i++) {
        m_pRowConstraints[i] = m_uMask;
        m_pColConstraints[i] = m_uMask;
        m_pBlockConstraints[i] = m_uMask;
    }

    for (int i = 0; i < MAX_CELLS; i++) {
        m_pCellConstraints[i] = m_uMask;
    }
}

bool SudokuSolver::supose(int row, int col, unsigned short number) {
    if (number <= 0 || number > m_uNumbers)
        return false;

    int grid = (row / m_uGridRows) * m_uGridsInCol + col / m_uGridCols;

    unsigned short mask = 1 << (number -1);

    if ((mask & m_pRowConstraints[row]) == 0 || (mask & m_pColConstraints[col]) == 0 || (mask & m_pBlockConstraints[grid]) == 0)
        return false;

    m_pSolution[row*m_uCols + col] = number;

    //update constraints
    m_pRowConstraints[row] ^= mask;
    m_pColConstraints[col] ^= mask;
    m_pBlockConstraints[grid] ^= mask;
    return true;

}

void SudokuSolver::resume(int row, int col, unsigned short number) {
    unsigned short mask = 1 << (number -1);
    int grid = (row / m_uGridRows) * m_uGridsInCol + col / m_uGridCols;

    m_pSolution[row*m_uCols + col] = 0;

    //update constraints
    m_pRowConstraints[row] |= mask;
    m_pColConstraints[col] |= mask;
    m_pBlockConstraints[grid] |= mask;
}

int SudokuSolver::try_solve(bool oneShot /*= true*/) {
    m_iSolutionCount = search(0, oneShot);
    return m_iSolutionCount;
}

void SudokuSolver::setNumber(int row, int col, int value) {
    if (value <= 0 || value > m_uNumbers)
        return;

    supose(row, col, value);
    m_pCellConstraints[row*m_uCols + col] |= PRESET_BIT;
}

int SudokuSolver::unsetNumber(int row, int col, bool exclude /* = false*/) {
    if (m_pSolution[row*m_uCols + col] == 0)
        return 0;

    int value = m_pSolution[row*m_uCols + col];
    resume(row, col, value);
	m_pCellConstraints[row*m_uCols + col] = m_uMask;
    if (exclude)
    	m_pCellConstraints[row*m_uCols + col] ^= (1 << (value - 1));
    return value;
}

//#include <iostream>
//void SudokuSolver::printBox() {
//    for (int i = 0; i < m_uCols * m_uRows; i++) {
//        if (i % m_uCols == 0)
//            std::cout << std::endl;
//        std::cout << int(m_pSolution[i]) << " ";
//    }
//    std::cout << std::endl;
//}

SudokuGenerator* SudokuGenerator::getInstance() {
    if (!s_pGenerator) {
        s_pGenerator = new SudokuGenerator();
    }
    return s_pGenerator;
}

void SudokuGenerator::setSize(short rows_per_grid, short cols_per_grid, short grids_in_row, short grids_in_col) {
    m_solver.setSize(rows_per_grid, cols_per_grid, grids_in_row, grids_in_col);
}

bool SudokuGenerator::generateBox() {
    if (m_solver.m_uNumbers <= 0)
        return false;

label_random:
    m_solver.initConstraints();
    memset(m_solver.m_pSolution, 0, MAX_CELLS);

    int ROW = 0, COL = 0, BLOCK = 0;
    unsigned short value = rand() % m_solver.m_uNumbers + 1;
    m_solver.setNumber(0, 0, value);
    ROW |= (1 << (value - 1));
    COL |= (1 << (value - 1));
    BLOCK |= (1 << (value - 1));

    //generate col 0's random permutation
    for (int i = 1; i < m_solver.m_uRows; i++) {
        value = rand() % m_solver.m_uNumbers;
        while (((1 << value) & COL) != 0) {
            value = (value + 1) % m_solver.m_uNumbers;
        }
        m_solver.setNumber(i, 0, value + 1);

        COL |= (1 << value);
        if (i < m_solver.m_uGridRows)
            BLOCK |= (1 << value);
    }

    //generate block 0's random permutation
    for (int i = 0; i < m_solver.m_uGridRows; i++) {
        for (int j = 1; j < m_solver.m_uGridCols; j++) {
            value = rand() % m_solver.m_uNumbers;
            while (((1 << value) & BLOCK) != 0) {
                value = (value + 1) % m_solver.m_uNumbers;
            }
            m_solver.setNumber(i, j, value + 1);

            BLOCK |= (1 << value);

            //for row 0, the set value should change the constrain for col too.
            if (i == 0)
                ROW |= (1 << value);
       }
    }

    //generate row 0's random permutation
    for (int i = m_solver.m_uGridCols; i < m_solver.m_uCols; i++) {
        value = rand() % m_solver.m_uNumbers;
        while (((1 << value) & ROW) != 0) {
            value = (value + 1) % m_solver.m_uNumbers;
        }
        m_solver.setNumber(0, i, value + 1);
        ROW |= (1 << value);
    }

    m_solver.try_solve();
    if (m_solver.m_iSolutionCount <= 0)
        goto label_random;

    return true;
}

void SudokuGenerator::setPuzzleDifficulty(Difficulty difficulty) {
	if (difficulty == m_eDifficulty)
		return;

	m_eDifficulty = difficulty;
	if (m_pDigger) {
		delete m_pDigger;
		m_pDigger = nullptr;
	}
}

bool SudokuGenerator::generate() {
	if (m_pDigger) {
		delete m_pDigger;
		m_pDigger = nullptr;
	}

	if (!generateBox())
		return false;

	//the box generated, it's solved, so constraint should be clear to prepare for generating puzzle.
	m_solver.clearConstraint();
	CellDigger* digger = CellDigger::create(&m_solver, m_eDifficulty);
	if (!digger || !digger->digoutPuzzle())
		return false;

	m_pDigger = digger;
	return true;
}


//////////////////////////////////////////////////////////////////////////////
// Cell Digger Implemention
//
/*----------------- Base: CellDigger --------------------------*/
CellDigger::CellDigger(SudokuSolver* solver)
	:m_pSolver(solver) {
	m_pSolution = solver->m_pSolution;
	memcpy(m_pPuzzle, m_pSolution, MAX_CELLS);
	memset(m_pDiggable, 1, MAX_CELLS);
	m_sKeeps = solver->cells();
}

CellDigger::~CellDigger() {
}

void CellDigger::prepare() {
	for (int i = 0; i < m_pSolver->cells(); i++)
		m_vctPos.push_back(i);
}

CellDigger* CellDigger::create(SudokuSolver* solver, Difficulty& difficulty) {
	CellDigger* digger = nullptr;
	if (!solver)
		return digger;

	int cells = solver->m_uCols * solver->m_uRows;

	short max = 0, min = 0;	//how many cells should be dug
	switch (difficulty) {
	case Difficulty::Easy:
		digger = new CellDiggerRandom(solver);
		min = (cells * 3 + 5) / 10;
		digger->m_sShouldKeep = cells - min;
		break;
	case Difficulty::Normal:
		digger = new CellDiggerRandom(solver);
		max = (cells * 6 + 5) / 10;
		min = (cells * 3 + 5) / 10;
		digger->m_sShouldKeep = cells - (int)(min + (pow((rand() % 1001), 1.0f/3) + 1)/ 10 * (max - min));
		if (digger->m_sShouldKeep > cells - min)
			digger->m_sShouldKeep = cells - min;
		break;

	//in fact, for now, the difficulty of puzzle generated by CellDiggerOneByOne is similar to
	//CellDiggerByNumber;
	case Difficulty::Hard:
		digger = new CellDiggerOneByOne(solver);
		break;

	case Difficulty::Evil:
		digger = new CellDiggerByNumber(solver);
		break;
	}

	return digger;
}

bool CellDigger::digoutPuzzle() {
	prepare();
	doFirstDig();

	while (!m_vctPos.empty() && !isDigDone()) {
		int pos = m_vctPos.front();
		if (tryDig(pos))
			dig(pos);

		m_vctPos.erase(m_vctPos.begin());
	}
	return checkDigResult();
}

void CellDigger::dig(int pos) {
	int row = pos / m_pSolver->cols();
	int col = pos % m_pSolver->cols();
	m_pSolver->unsetNumber(row, col, false);
	m_pPuzzle[pos] = 0;
	m_sKeeps--;
}

bool CellDigger::tryDig(int pos) {
	if (!m_pDiggable[pos])
		return false;


	SudokuSolver solver(m_pSolver);
	int row = pos / solver.cols();
	int col = pos % solver.cols();
	solver.unsetNumber(row, col, true);

	//to guarantee the sudoku has only one solution, we unsetnumber with exclude flag
	//set to 'false', so here we should could not solve out this puzzle.
	bool ret = (solver.try_solve(false) == 0);
	m_pDiggable[pos] = 0;	// move this operation into a virtual function eg. postTryDig
	return ret;
}

/*----------------- CellDiggerRandom --------------------------*/
void CellDiggerRandom::doFirstDig() {
	int pos = m_vctPos.front();
	m_vctPos.erase(m_vctPos.begin());
	dig(pos);
}

void CellDiggerRandom::prepare() {
	CellDigger::prepare();

	int size = m_vctPos.size();
	for (int i = 0; i < size; i++) {
		std::swap(m_vctPos[rand() % size], m_vctPos[i]);
	}
}

/*----------------- CellDiggerOneByOne --------------------------*/
void CellDiggerOneByOne::doFirstDig() {
	if (m_vctPos.empty())
		return;

	int pos = m_vctPos.front();
	dig(pos);
}

/*----------------- CellDiggerByNumber --------------------------*/
void CellDiggerByNumber::doFirstDig() {
	int count = m_pSolver->grids();
	while (!m_vctPos.empty() && count-- > 0) {
		int pos = m_vctPos.front();
		dig(pos);

		m_vctPos.erase(m_vctPos.begin());
	}
}

void CellDiggerByNumber::prepare() {
	std::vector<int> numbers;
	for (int i = 1; i <= m_pSolver->numbers(); i++ )
		numbers.push_back(i);

	while (!numbers.empty()) {
		int index = rand() % numbers.size();
		int number = numbers[index];
		numbers.erase(numbers.begin() + index);


		for (int i = 0, count = 0; count < m_pSolver->grids() && i < m_pSolver->cells(); i++) {
			if (m_pSolution[i] == number) {
				m_vctPos.push_back(i);
				count++;
			}
		}
	}
}
