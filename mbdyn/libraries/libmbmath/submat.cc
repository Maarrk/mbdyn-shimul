/*
 * MBDyn (C) is a multibody analysis code.
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2004
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* sottomatrici */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <string.h>	/* for memset() */
#include <ac/iomanip>

#include <submat.h>

/* SubMatrixHandler - begin */

SubMatrixHandler::~SubMatrixHandler(void)
{
	NO_OP;
}

/* SubMatrixHandler - end */


/* FullSubMatrixHandler - begin */

FullSubMatrixHandler::FullSubMatrixHandler(integer iIntSize,
		integer* piTmpVec,
		integer iDoubleSize,
		doublereal* pdTmpMat,
		integer iMaxCols,
		doublereal **ppdCols)
: FullMatrixHandler(pdTmpMat, ppdCols, iDoubleSize, 1, 1, iMaxCols),
iVecSize(iIntSize), piRowm1(0), piColm1(0)
{
#ifdef DEBUG
	IsValid();
#endif

	/*
	 * Non posso fare controlli piu' precisi sulla consistenza
	 * delle dimensioni delle aree di lavoro perche' e' legale
	 * che la submatrix sia in seguito ridimensionata nei modi
	 * piu' vari. */
	ASSERT(piTmpVec);
	piRowm1 = piTmpVec - 1;
	piColm1 = piRowm1 + iNumRows;
}

FullSubMatrixHandler::FullSubMatrixHandler(integer iNR, integer iNC)
: FullMatrixHandler(iNR, iNC), iVecSize(iNR + iNC), piRowm1(0), piColm1(0)
{
	ASSERT(bOwnsMemory);
	SAFENEWARR(piRowm1, integer, iVecSize);

	piRowm1--;
	piColm1 = piRowm1 + iNR;
}


FullSubMatrixHandler::~FullSubMatrixHandler(void)
{
	if (bOwnsMemory) {
		piRowm1++;
		SAFEDELETEARR(piRowm1);
	}
}

#ifdef DEBUG
void
FullSubMatrixHandler::IsValid(void) const
{
	FullMatrixHandler::IsValid();

	ASSERT(iVecSize > 0);
	ASSERT(iNumRows + iNumCols <= iVecSize);
	ASSERT(piRowm1 != NULL);
	ASSERT(piColm1 != NULL);

#ifdef DEBUG_MEMMANAGER
	ASSERT(defaultMemoryManager.fIsValid(piRowm1 + 1,
				iVecSize*sizeof(integer)));
#endif /* DEBUG_MEMMANAGER */
}
#endif /* DEBUG */


void
FullSubMatrixHandler::Init(const doublereal& dResetVal)
{
#ifdef DEBUG
	IsValid();
#endif /* DEBUG */

	FullMatrixHandler::Init(dResetVal);

#if 0
	/*
	 * this is not strictly required, because all the indices should
	 * be explicitly set before the matrix is used
	 */
	for (integer i = iNumRows + iNumCols; i > 0; i--) {
		piRowm1[i] = 0;
	}
#endif
}

/*
 * Modifica le dimensioni correnti
 */
void
FullSubMatrixHandler::Resize(integer iNewRow, integer iNewCol) {
#ifdef DEBUG
	IsValid();
#endif /* DEBUG */

	ASSERT(iNewRow > 0);
	ASSERT(iNewCol > 0);
	ASSERT(iNewRow + iNewCol <= iVecSize);
	ASSERT(iNewRow*iNewCol <= iRawSize);

	if (iNewRow <= 0
			|| iNewCol <= 0
			|| iNewRow + iNewCol > iVecSize
			|| iNewRow*iNewCol > iRawSize
			|| iNewCol > iMaxCols) {
		silent_cerr("FullSubMatrixHandler::Resize() - error"
				<< std::endl);

		THROW(SubMatrixHandler::ErrResize());
	}

	iNumRows = iNewRow;
	iNumCols = iNewCol;

	/*
	 * FIXME: don't call FullMatrixHandler::Resize, because
	 * we know there's room enough and we don't want to
	 * waste time regenerating support stuff; we simply
	 * use a portion of the matrix as is.
	 */
#if 0
	FullMatrixHandler::Resize(iNewRow, iNewCol);

	ASSERT(piRowm1 != NULL);
	piColm1 = piRowm1 + iNewRow;
#endif

#ifdef DEBUG
	IsValid();
#endif /* DEBUG */
}

/* Ridimensiona ed inizializza. */
void
FullSubMatrixHandler::ResizeInit(integer ir, integer ic, const doublereal& d)
{
	FullSubMatrixHandler::Resize(ir, ic);
	FullSubMatrixHandler::Init(d);
}

/*
 * Collega la matrice Full alla memoria che gli viene passata
 * in ingresso
 */
void
FullSubMatrixHandler::Attach(int iRows, int iCols, integer* piTmpIndx)
{
	iNumRows = iRows;
	iNumCols = iCols;
	
	if (bOwnsMemory) {
		if (piRowm1 != 0) {
			piRowm1++;
			SAFEDELETEARR(piRowm1);
		}
		piRowm1 = piTmpIndx - 1;
		piColm1 = piRowm1 + iNumRows;
	}
}

/* somma una matrice di tipo Mat3x3 in una data posizione */
void
FullSubMatrixHandler::Add(integer iRow, integer iCol, const Mat3x3& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	/* Nota: assume che la matrice sia organizzata
	 * "per colonne" (stile FORTRAN) e assume
	 * che la matrice Mat3x3 sia organizzata anch'essa per colonne */
	doublereal* pdFrom = m.pGetMat();

	ppdColsm1[iCol][iRow] += pdFrom[M11];
	ppdColsm1[iCol][iRow + 1] += pdFrom[M21];
	ppdColsm1[iCol][iRow + 2] += pdFrom[M31];

	ppdColsm1[iCol + 1][iRow] += pdFrom[M12];
	ppdColsm1[iCol + 1][iRow + 1] += pdFrom[M22];
	ppdColsm1[iCol + 1][iRow + 2] += pdFrom[M32];

	ppdColsm1[iCol + 2][iRow] += pdFrom[M13];
	ppdColsm1[iCol + 2][iRow + 1] += pdFrom[M23];
	ppdColsm1[iCol + 2][iRow + 2] += pdFrom[M33];

#if 0
	for (unsigned c = 0; c < 2; c++) {
		for (unsigned r = 0; r < 2; r++) {
			ppdColsm1[iCol + c][iRow + r] += pdFrom[r];
		}
		pdFrom += 3;
	}
#endif
}


/* sottrae una matrice di tipo Mat3x3 in una data posizione
 * analoga ala precedente, con il meno (per evitare temporanei ecc) */
void
FullSubMatrixHandler::Sub(integer iRow, integer iCol, const Mat3x3& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1
	 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	doublereal* pdFrom = m.pGetMat();

	ppdColsm1[iCol][iRow] -= pdFrom[M11];
	ppdColsm1[iCol][iRow + 1] -= pdFrom[M21];
	ppdColsm1[iCol][iRow + 2] -= pdFrom[M31];

	ppdColsm1[iCol + 1][iRow] -= pdFrom[M12];
	ppdColsm1[iCol + 1][iRow + 1] -= pdFrom[M22];
	ppdColsm1[iCol + 1][iRow + 2] -= pdFrom[M32];

	ppdColsm1[iCol + 2][iRow] -= pdFrom[M13];
	ppdColsm1[iCol + 2][iRow + 1] -= pdFrom[M23];
	ppdColsm1[iCol + 2][iRow + 2] -= pdFrom[M33];

#if 0
	for (unsigned c = 0; c < 2; c++) {
		for (unsigned r = 0; r < 2; r++) {
			ppdColsm1[iCol + c][iRow + r] -= pdFrom[r];
		}
		pdFrom += 3;
	}
#endif
}


/* mette una matrice di tipo Mat3x3 in una data posizione;
 * analoga alle precedenti */
void
FullSubMatrixHandler::Put(integer iRow, integer iCol, const Mat3x3& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1
	 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	doublereal* pdFrom = m.pGetMat();

	ppdColsm1[iCol][iRow] = pdFrom[M11];
	ppdColsm1[iCol][iRow + 1] = pdFrom[M21];
	ppdColsm1[iCol][iRow + 2] = pdFrom[M31];

	ppdColsm1[iCol + 1][iRow] = pdFrom[M12];
	ppdColsm1[iCol + 1][iRow + 1] = pdFrom[M22];
	ppdColsm1[iCol + 1][iRow + 2] = pdFrom[M32];

	ppdColsm1[iCol + 2][iRow] = pdFrom[M13];
	ppdColsm1[iCol + 2][iRow + 1] = pdFrom[M23];
	ppdColsm1[iCol + 2][iRow + 2] = pdFrom[M33];

#if 0
	for (unsigned c = 0; c < 2; c++) {
		for (unsigned r = 0; r < 2; r++) {
			ppdColsm1[iCol + c][iRow + r] = pdFrom[r];
		}
		pdFrom += 3;
	}
#endif
}


/* somma una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::Add(integer iRow, integer iCol, const Mat3xN& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - m.iGetNumCols() + 1);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = 3; r > 0; r--) {
		for (integer c = m.iGetNumCols(); c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] += m(r, c);
		}
	}
}

/* sottrae una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::Sub(integer iRow, integer iCol, const Mat3xN& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - m.iGetNumCols() + 1);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = 3; r > 0; r--) {
		for (integer c = m.iGetNumCols(); c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] -= m(r, c);
		}
	}
}

/* setta una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::Put(integer iRow, integer iCol, const Mat3xN& m)
{
	/* iRow e iCol sono gli indici effettivi di riga e colonna
	 * es. per il primo coefficiente:
	 *     iRow = 1, iCol = 1 */

#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - 2);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - m.iGetNumCols() + 1);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = 3; r > 0; r--) {
		for (integer c = m.iGetNumCols(); c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] = m(r, c);
		}
	}
}

/* somma una matrice di tipo MatNx3 in una data posizione */
void
FullSubMatrixHandler::Add(integer iRow, integer iCol, const MatNx3& m)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - m.iGetNumRows() + 1);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = m.iGetNumRows(); r > 0; r--) {
		for (integer c = 3; c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] += m(r, c);
		}
	}
}

/* sottrae una matrice di tipo MatNx3 in una data posizione */
void
FullSubMatrixHandler::Sub(integer iRow, integer iCol, const MatNx3& m)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - m.iGetNumRows() + 1);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = m.iGetNumRows(); r > 0; r--) {
		for (integer c = 3; c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] -= m(r, c);
		}
	}
}

/* setta una matrice di tipo MatNx3 in una data posizione */
void
FullSubMatrixHandler::Put(integer iRow, integer iCol, const MatNx3& m)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iRow > 0);
	ASSERT(iRow <= iNumRows - m.iGetNumRows() + 1);
	ASSERT(iCol > 0);
	ASSERT(iCol <= iNumCols - 2);
#endif /* DEBUG */

	--iRow;
	--iCol;
	for (int r = m.iGetNumRows(); r > 0; r--) {
		for (integer c = 3; c > 0; c--) {
			ppdColsm1[iCol + c][iRow + r] = m(r, c);
		}
	}
}


/* setta una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::PutDiag(integer iFirstRow, integer iFirstCol,
		const Vec3& v)
{
	/* iFirstRow e iFirstCol sono gli indici effettivi di riga e colonna -1
	 * es. per il primo coefficiente:
	 *     iRow = 0, iCol = 0 */

#ifdef DEBUG
	IsValid();

	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstRow <= iNumRows - 3);
	ASSERT(iFirstCol >= 0);
	ASSERT(iFirstCol <= iNumCols - 3);
#endif /* DEBUG */

	const doublereal *pdv = v.pGetVec();

	ppdColsm1[iFirstCol + 1][iFirstRow + 1] = pdv[V1];
	ppdColsm1[iFirstCol + 2][iFirstRow + 2] = pdv[V2];
	ppdColsm1[iFirstCol + 3][iFirstRow + 3] = pdv[V3];
}


/* setta una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::PutDiag(integer iFirstRow, integer iFirstCol,
		const doublereal& d)
{
	/* iFirstRow e iFirstCol sono gli indici effettivi di riga e colonna -1
	 * es. per il primo coefficiente:
	 *     iRow = 0, iCol = 0 */

#ifdef DEBUG
	IsValid();

	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstRow <= iNumRows - 3);
	ASSERT(iFirstCol >= 0);
	ASSERT(iFirstCol <= iNumCols - 3);
#endif /* DEBUG */

	ppdColsm1[iFirstCol + 1][iFirstRow + 1] = d;
	ppdColsm1[iFirstCol + 2][iFirstRow + 2] = d;
	ppdColsm1[iFirstCol + 3][iFirstRow + 3] = d;
}


/* setta una matrice di tipo Mat3xN in una data posizione */
void
FullSubMatrixHandler::PutCross(integer iFirstRow, integer iFirstCol,
		const Vec3& v)
{
	/* iFirstRow e iFirstCol sono gli indici effettivi di riga e colonna -1
	 * es. per il primo coefficiente:
	 *     iRow = 0, iCol = 0 */

#ifdef DEBUG
	IsValid();

	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstRow <= iNumRows - 3);
	ASSERT(iFirstCol >= 0);
	ASSERT(iFirstCol <= iNumCols - 3);
#endif /* DEBUG */

	const doublereal *pdv = v.pGetVec();

	ppdColsm1[iFirstCol + 1][ iFirstRow + 2] = -pdv[V3];
	ppdColsm1[iFirstCol + 1][ iFirstRow + 3] = pdv[V2];

	ppdColsm1[iFirstCol + 2][ iFirstRow + 1] = pdv[V3];
	ppdColsm1[iFirstCol + 2][ iFirstRow + 3] = -pdv[V1];

	ppdColsm1[iFirstCol + 3][ iFirstRow + 1] = -pdv[V2];
	ppdColsm1[iFirstCol + 3][ iFirstRow + 2] = pdv[V1];
}


/* somma la matrice ad un matrix handler usando i metodi generici */
MatrixHandler&
FullSubMatrixHandler::AddTo(MatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	ASSERT(MH.iGetNumRows() >= iNumRows);
	ASSERT(MH.iGetNumCols() >= iNumCols);

	for (integer c = iNumCols; c > 0; c--) {
		ASSERT(piColm1[c] > 0);
		ASSERT(piColm1[c] <= MH.iGetNumCols());

		for (integer r = iNumRows; r > 0; r--) {
			ASSERT(piRowm1[r] > 0);
			ASSERT(piRowm1[r] <= MH.iGetNumRows());

			MH(piRowm1[r], piColm1[c]) += ppdColsm1[c][r];
		}
	}

	return MH;
}


/* somma la matrice ad un FullMatrixHandler */
MatrixHandler&
FullSubMatrixHandler::AddTo(FullMatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	ASSERT(MH.iGetNumRows() >= iNumRows);
	ASSERT(MH.iGetNumCols() >= iNumCols);

	doublereal **ppd = MH.ppdColsm1;

	for (integer c = iNumCols; c > 0; c--) {
		ASSERT(piColm1[c] > 0);
		ASSERT(piColm1[c] <= MH.iGetNumCols());

		for (integer r = iNumRows; r > 0; r--) {
			ASSERT(piRowm1[r] > 0);
			ASSERT(piRowm1[r] <= MH.iGetNumRows());

			ppd[piColm1[r]][piRowm1[c]] += ppdColsm1[c][r];
		}
	}

	return MH;
}


/* sottrae la matrice da un matrix handler usando i metodi generici */
MatrixHandler&
FullSubMatrixHandler::SubFrom(MatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	ASSERT(MH.iGetNumRows() >= iNumRows);
	ASSERT(MH.iGetNumCols() >= iNumCols);

	for (integer c = iNumCols; c > 0; c--) {
		ASSERT(piColm1[c] > 0);
		ASSERT(piColm1[c] <= MH.iGetNumCols());

		for (integer r = iNumRows; r > 0; r--) {
			ASSERT(piRowm1[r] > 0);
			ASSERT(piRowm1[r] <= MH.iGetNumRows());

			MH(piRowm1[r], piColm1[c]) -= ppdColsm1[c][r];
		}
	}

	return MH;
}


/* sottrae la matrice da un FullMatrixHandler */
MatrixHandler&
FullSubMatrixHandler::SubFrom(FullMatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	ASSERT(MH.iGetNumRows() >= iNumRows);
	ASSERT(MH.iGetNumCols() >= iNumCols);

	doublereal **ppd = MH.ppdColsm1;

	for (integer c = iNumCols; c > 0; c--) {
		ASSERT(piColm1[c] > 0);
		ASSERT(piColm1[c] <= MH.iGetNumCols());

		for (integer r = iNumRows; r > 0; r--) {
			ASSERT(piRowm1[r] > 0);
			ASSERT(piRowm1[r] <= MH.iGetNumRows());

			ppd[piColm1[c]][piRowm1[r]] -= ppdColsm1[c][r];
		}
	}

	return MH;
};


/* output, usato principalmente per debug */
std::ostream&
operator << (std::ostream& out, const FullSubMatrixHandler& m)
{
#ifdef DEBUG
	m.IsValid();
#endif /* DEBUG */

	integer iRow = (integer)m.iNumRows;
	integer iCol = (integer)m.iNumCols;
	integer* piRowm1 = (integer *)m.piRowm1;
	integer* piColm1 = (integer *)m.piColm1;
	doublereal** ppd = (doublereal **)m.ppdColsm1;

	ASSERT(iRow > 0);
	ASSERT(iCol > 0);
	ASSERT(piRowm1 != NULL);
	ASSERT(piColm1 != NULL);
	ASSERT(ppd != NULL);

	out << std::setw(12) << "";
	for (integer c = 1; c < iCol; c++) {
		out << std::setw(12) << piColm1[c];
	}
	out << std::endl << std::endl;

	for (integer r = 1; r < iRow; r++) {
		out << std::setw(12) << piRowm1[r];
		for (integer c = 0; c < iCol; c++) {
			out << std::setw(12) << ppd[c][r];
		}
 		out << std::endl;
	}

	return out << std::endl;
}

/* FullSubMatrixHandler - end */


/* SparseSubMatrixHandler - begin */

SparseSubMatrixHandler::SparseSubMatrixHandler(integer iTmpInt,
		integer* piTmpIndex, integer iTmpDouble, doublereal* pdTmpMat)
: bOwnsMemory(false),
iIntSize(iTmpInt), iDoubleSize(iTmpDouble),
iNumItems(iTmpInt/2),
piRowm1(0), piColm1(0),
pdMatm1(0)
{
	ASSERT(piTmpIndex);
	ASSERT(pdTmpMat);

	piRowm1 = piTmpIndex - 1;
	piColm1 = piRowm1 + iNumItems;

	pdMatm1 = pdTmpMat - 1;

#ifdef DEBUG
	IsValid();
#endif /* DEBUG */
}

SparseSubMatrixHandler::SparseSubMatrixHandler(integer iTmpInt)
: iIntSize(2*iTmpInt), iDoubleSize(iTmpInt),
iNumItems(iTmpInt), piRowm1(0), piColm1(0), pdMatm1(0)
{
	SAFENEWARR(pdMatm1, doublereal, iNumItems);
	pdMatm1--;

	SAFENEWARR(piRowm1, integer, iIntSize);
	piRowm1--;
	piColm1 = piRowm1 + iNumItems;
	
	bOwnsMemory = true;
}

/* Distruttore banale.
 * Nota: dato che la classe non possiede la memoria,
 * non ne deve deallocare
 */
SparseSubMatrixHandler::~SparseSubMatrixHandler(void)
{
	if (bOwnsMemory) {
		pdMatm1++;
		SAFEDELETEARR(pdMatm1);

		piRowm1++;
		SAFEDELETEARR(piRowm1);
	}
}

#ifdef DEBUG
void
SparseSubMatrixHandler::IsValid(void) const
{
	ASSERT(iIntSize > 0);
	ASSERT(iIntSize%2 == 0);
	ASSERT(iDoubleSize > 0);
	ASSERT(iIntSize >= 2*iDoubleSize);
	ASSERT(iNumItems >= 0);
	ASSERT(piRowm1 != NULL);
	ASSERT(piColm1 != NULL);
	ASSERT(pdMatm1 != NULL);

#ifdef DEBUG_MEMMANAGER
	ASSERT(defaultMemoryManager.fIsValid(piRowm1 + 1,
				iIntSize*sizeof(integer)));
	ASSERT(defaultMemoryManager.fIsValid(pdMatm1 + 1,
				iDoubleSize*sizeof(doublereal)));
#endif /* DEBUG_MEMMANAGER */
}
#endif /* DEBUG */


/*
 * Ridimensiona la matrice.
 * Nota: solo il primo argomento viene considerato,
 * e rappresenta il numero totale di entries.
 * Questo metodo deve essere chiamato prima di qualsiasi
 * operazione sulla matrice.
 */
void
SparseSubMatrixHandler::Resize(integer iNewRow, integer iNewCol)
{
#ifdef DEBUG
	IsValid();
#endif /* DEBUG */

	ASSERT(iNewRow > 0);
	ASSERT(2*iNewRow <= iIntSize);
	ASSERT(iNewRow <= iDoubleSize);
	ASSERT(piRowm1 != NULL);

	if (iNewRow <= 0
			|| 2*iNewRow > iIntSize
			|| iNewRow > iDoubleSize) {
		silent_cerr("SparseSubMatrixHandler::Resize() - error"
				<< std::endl);
		THROW(SparseSubMatrixHandler::ErrResize());
	}

	iNumItems = iNewRow;

#ifdef DEBUG
	IsValid();
#endif /* DEBUG */
}

/*
 * Ridimensiona ed inizializza.
 * Unione dei due metodi precedenti
 */
void
SparseSubMatrixHandler::ResizeInit(integer iNewRow, integer iNewCol,
		const doublereal& dCoef)
{
	Resize(iNewRow, iNewCol);
	Init(dCoef);
}

/*
 * Collega la matrice sparsa alla memoria che gli viene passata
 * in ingresso
 */
void
SparseSubMatrixHandler::Attach(int iNumEntr, doublereal* pdTmpMat, 
		integer* piTmpIndx)
{
	if (bOwnsMemory) {
		piRowm1++;
		SAFEDELETEARR(piRowm1);

		pdMatm1++;
		SAFEDELETEARR(pdMatm1);

		bOwnsMemory = false;
	}

	ASSERT(iNumEntr > 0);
	ASSERT(piTmpIndx);
	ASSERT(pdTmpMat);

	iIntSize = iNumEntr*2;
	iDoubleSize = iNumEntr;
	iNumItems = iNumEntr;
	piRowm1 = piTmpIndx - 1;
	piColm1 = piRowm1 + iNumEntr;
	pdMatm1 = pdTmpMat - 1;

#ifdef DEBUG
	IsValid();
#endif /* DEBUG */
}

void
SparseSubMatrixHandler::PutDiag(integer iSubIt, integer iFirstRow,
		integer iFirstCol, const Vec3& v)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iNumItems >= 3);
	ASSERT(iSubIt > 0);
	ASSERT(iSubIt <= iNumItems - 2);
	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstCol >= 0);
#endif /* DEBUG */

	/*
	 * Attenzione agli argomenti:
	 * iSubIt e' il primo indice della matrice da utilizzare,
	 * con 1 <= iSubit <= iCurSize;
	 * iFirstRow e' il primo indice di riga -1, ovvero il primo indice
	 * di riga della sottomatrice diag(v) piena e' iFirstRow + 1
	 * iFirstCol e' il primo indice di colonna -1, ovvero il primo indice
	 * di colonna della sottomatrice diag(v) piena e' iFirstCol + 1
	 * v e' il vettore che genera diag(v)
	 */

	/* Matrice diag(v) :
	 *
	 *         1   2   3
	 *
	 * 1    |  v1  0   0  |
	 * 2    |  0   v2  0  |
	 * 3    |  0   0   v3 |
	 */

 	/* assume che il Vec3 sia un'array di 3 reali */
	const doublereal* pdFrom = v.pGetVec();

	doublereal* pdm = pdMatm1 + iSubIt;
	integer* pir = piRowm1 + iSubIt;
	integer* pic = piColm1 + iSubIt;

	/* Coefficiente 1,1 */
	pdm[0] = pdFrom[V1];
	pir[0] = iFirstRow + 1;
	pic[0] = iFirstCol + 1;

	/* Coefficiente 2,2 */
	pdm[1] = pdFrom[V2];
	pir[1] = iFirstRow + 2;
	pic[1] = iFirstCol + 2;

	/* Coefficiente 3,3 */
	pdm[2] = pdFrom[V3];
	pir[2] = iFirstRow + 3;
	pic[2] = iFirstCol + 3;
}


void
SparseSubMatrixHandler::PutDiag(integer iSubIt, integer iFirstRow,
	       integer iFirstCol, const doublereal& d)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iNumItems >= 3);
	ASSERT(iSubIt > 0);
	ASSERT(iSubIt <= iNumItems - 2);
	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstCol >= 0);
#endif /* DEBUG */

	/* Attenzione agli argomenti:
	 * iSubIt e' il primo indice della matrice da utilizzare,
	 * con 1 <= iSubit <= iCurSize;
	 * iFirstRow e' il primo indice di riga -1, ovvero il
	 * primo indice di riga della sottomatrice I*d piena e' iFirstRow+1
	 * iFirstCol e' il primo indice di colonna -1, ovvero il
	 * primo indice di colonna della sottomatrice I*d piena e' iFirstCol+1
	 * v e' il vettore che genera I*d */

	/* Matrice I*d :
	 *
	 *         1   2   3
	 *
	 * 1    |  d   0   0 |
	 * 2    |  0   d   0 |
	 * 3    |  0   0   d |
	 */

	doublereal* pdm = pdMatm1 + iSubIt;
	integer* pir = piRowm1 + iSubIt;
	integer* pic = piColm1 + iSubIt;

	/* Coefficiente 1,1 */
	pdm[0] = d;
	pir[0] = iFirstRow + 1;
	pic[0] = iFirstCol + 1;

	/* Coefficiente 2,2 */
	pdm[1] = d;
	pir[1] = iFirstRow + 2;
	pic[1] = iFirstCol + 2;

	/* Coefficiente 3,3 */
	pdm[2] = d;
	pir[2] = iFirstRow + 3;
	pic[2] = iFirstCol + 3;
}


void
SparseSubMatrixHandler::PutCross(integer iSubIt, integer iFirstRow,
	       integer iFirstCol, const Vec3& v)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iNumItems >= 6);
	ASSERT(iSubIt > 0);
	ASSERT(iSubIt <= iNumItems - 5);
	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstCol >= 0);
#endif /* DEBUG */

	/* Attenzione agli argomenti:
	 * iSubIt e' il primo indice della matrice da utilizzare,
	 * con 1 <= iSubit <= iCurSize;
	 * iFirstRow e' il primo indice di riga -1, ovvero il
	 * primo indice di riga della sottomatrice v/\ piena e' iFirstRow+1
	 * iFirstCol e' il primo indice di colonna -1, ovvero il
	 * primo indice di colonna della sottomatrice v/\ piena e' iFirstCol+1
	 * v e' il vettore che genera v/\ */

	/* Matrice v/\ :
	 *
	 *         1   2   3
	 *
	 * 1    |  0  -v3  v2 |
	 * 2    |  v3  0  -v1 |
	 * 3    | -v2  v1  0  |
	 */

	/* assume che il Vec3 sia un'array di 3 reali */
	const doublereal* pdFrom = v.pGetVec();

	/* Coefficiente 1,2 */
	doublereal* pdm = pdMatm1 + iSubIt;
	integer* pir = piRowm1 + iSubIt;
	integer* pic = piColm1 + iSubIt;

	pdm[0] = -pdFrom[V3];               // -v.dGet(3);
	pir[0] = iFirstRow + 1;
	pic[0] = iFirstCol + 2;

	/* Coefficiente 1,3 */
	pdm[1] = pdFrom[V2];                // v.dGet(2);
	pir[1] = iFirstRow + 1;
	pic[1] = iFirstCol + 3;

	/* Coefficiente 2,1 */
	pdm[2] = pdFrom[V3];                // v.dGet(3);
	pir[2] = iFirstRow + 2;
	pic[2] = iFirstCol + 1;

	/* Coefficiente 2,3 */
	pdm[3] = -pdFrom[V1];               // -v.dGet(1);
	pir[3] = iFirstRow + 2;
	pic[3] = iFirstCol + 3;

	/* Coefficiente 3,1 */
	pdm[4] = -pdFrom[V2];                // -v.dGet(2);
	pir[4] = iFirstRow + 3;
	pic[4] = iFirstCol + 1;

	/* Coefficiente 3,2 */
	pdm[5] = pdFrom[V1];                 // v.dGet(1);
	pir[5] = iFirstRow + 3;
	pic[5] = iFirstCol + 2;
}


void
SparseSubMatrixHandler::Init(const doublereal& dCoef)
{
#ifdef DEBUG
	IsValid();
#endif /* DEBUG */

	ASSERT(iNumItems > 0);

#ifdef HAVE_MEMSET
	if (dCoef == 0.) {
		memset(pdMatm1 + 1, 0, iNumItems*sizeof(doublereal));
	} else
#endif /* HAVE_MEMSET */
	{
		for (integer i = iNumItems; i > 0; i--) {
			pdMatm1[i] = dCoef;
		}
	}
}


/* Inserisce una matrice 3x3;
 * si noti che non ci sono Add, Sub, ecc. perche' la filosofia
 * della matrice sparsa prevede che ad ogni item (riga, colonna, valore)
 * corrisponda un termine che poi verra' sommato ad una matrice vera,
 * senza controlli su eventuali duplicazioni. */
void
SparseSubMatrixHandler::PutMat3x3(integer iSubIt, integer iFirstRow,
		integer iFirstCol, const Mat3x3& m)
{
#ifdef DEBUG
	IsValid();

	ASSERT(iNumItems >= 9);
	ASSERT(iSubIt > 0);
	ASSERT(iSubIt <= iNumItems - 8);
	ASSERT(iFirstRow >= 0);
	ASSERT(iFirstCol >= 0);
#endif /* DEBUG */

	/* Attenzione agli argomenti:
	 * iSubIt e' il primo indice della matrice da utilizzare,
	 * con 1 <= iSubit <= iCurSize;
	 * iFirstRow e' il primo indice di riga -1, ovvero il
	 * primo indice di riga della sottomatrice v/\ piena e' iFirstRow+1
	 * iFirstCol e' il primo indice di colonna -1, ovvero il
	 * primo indice di colonna della sottomatrice m piena e' iFirstCol+1
	 */

	/* Per efficienza, vengono scritte esplicitamente tutte
	 * le assegnazioni;
	 * la funzione quindi non e' messa in linea intenzionalmente */

	/* Coefficienti 1,1-3,1 */
	doublereal* pdFrom = (doublereal*)m.pGetMat();
	doublereal* pdTmpMat = pdMatm1 + iSubIt;
   	integer* piTmpRow = piRowm1 + iSubIt;
	integer* piTmpCol = piColm1 + iSubIt;

	iFirstRow++;
	iFirstCol++;

	/* Prima riga */
	pdTmpMat[0] = pdFrom[M11];
	piTmpRow[0] = iFirstRow++;
	piTmpCol[0] = iFirstCol;

	pdTmpMat[1] = pdFrom[M21];
	piTmpRow[1] = iFirstRow++;
	piTmpCol[1] = iFirstCol;

	pdTmpMat[2] = pdFrom[M31];
	piTmpRow[2] = iFirstRow;
	piTmpCol[2] = iFirstCol;

	/* Seconda riga */
	iFirstRow -= 2;
	iFirstCol++;

	pdTmpMat[3] = pdFrom[M12];
	piTmpRow[3] = iFirstRow++;
	piTmpCol[3] = iFirstCol;

	pdTmpMat[4] = pdFrom[M22];
	piTmpRow[4] = iFirstRow++;
	piTmpCol[4] = iFirstCol;

	pdTmpMat[5] = pdFrom[M32];
	piTmpRow[5] = iFirstRow;
	piTmpCol[5] = iFirstCol;

	/* Terza riga */
	iFirstRow -= 2;
	iFirstCol++;

	pdTmpMat[6] = pdFrom[M13];
	piTmpRow[6] = iFirstRow++;
	piTmpCol[6] = iFirstCol;

	pdTmpMat[7] = pdFrom[M23];
	piTmpRow[7] = iFirstRow++;
	piTmpCol[7] = iFirstCol;

	pdTmpMat[8] = pdFrom[M33];
	piTmpRow[8] = iFirstRow;
	piTmpCol[8] = iFirstCol;
}


/* somma la matrice ad un matrix handler usando i metodi generici */
MatrixHandler&
SparseSubMatrixHandler::AddTo(MatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	for (integer i = iNumItems; i > 0; i--) {
		ASSERT(piRowm1[i] > 0);
		ASSERT(piRowm1[i] <= MH.iGetNumRows());
		ASSERT(piColm1[i] > 0);
		ASSERT(piColm1[i] <= MH.iGetNumCols());

		MH(piRowm1[i], piColm1[i]) += pdMatm1[i];
	}

	return MH;
}


/* somma la matrice ad un FullMatrixHandler */
MatrixHandler&
SparseSubMatrixHandler::AddTo(FullMatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	doublereal **ppd = MH.ppdColsm1;

	for (integer i = iNumItems; i > 0; i--) {
		ASSERT(piRowm1[i] > 0);
		ASSERT(piRowm1[i] <= MH.iGetNumRows());
		ASSERT(piColm1[i] > 0);
		ASSERT(piColm1[i] <= MH.iGetNumCols());

		ppd[piColm1[i]][piRowm1[i]] += pdMatm1[i];
	}

	return MH;
}


/* sottrae la matrice da un matrix handler usando i metodi generici */
MatrixHandler&
SparseSubMatrixHandler::SubFrom(MatrixHandler& MH) const 
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	for (integer i = iNumItems; i > 0; i--) {
		ASSERT(piRowm1[i] > 0);
		ASSERT(piRowm1[i] <= MH.iGetNumRows());
		ASSERT(piColm1[i] > 0);
		ASSERT(piColm1[i] <= MH.iGetNumCols());

		MH(piRowm1[i], piColm1[i]) -= pdMatm1[i];
	}

	return MH;
}


/* sottrae la matrice da un FullMatrixHandler */
MatrixHandler&
SparseSubMatrixHandler::SubFrom(FullMatrixHandler& MH) const
{
#ifdef DEBUG
	IsValid();
	MH.IsValid();
#endif /* DEBUG */

	doublereal **ppd = MH.ppdColsm1;

	for (integer i = iNumItems; i > 0; i--) {
		ASSERT(piRowm1[i] > 0);
		ASSERT(piRowm1[i] <= MH.iGetNumRows());
		ASSERT(piColm1[i] > 0);
		ASSERT(piColm1[i] <= MH.iGetNumCols());

		ppd[piColm1[i]][piRowm1[i]] -= pdMatm1[i];
	}

	return MH;
}

/* SparseSubMatrixHandler - end */


/* MySubVectorHandler - begin */

MySubVectorHandler::MySubVectorHandler(integer iSize)
: MyVectorHandler(), piRowm1(NULL) {
	Resize(iSize);
}

MySubVectorHandler::MySubVectorHandler(integer iSize, integer* piTmpRow,
	       doublereal* pdTmpVec)
: MyVectorHandler(iSize, pdTmpVec), piRowm1(piTmpRow - 1) {
#ifdef DEBUG
	IsValid();
#endif /* DEBUG */
}

void
MySubVectorHandler::Resize(integer iSize)
{
	if (iSize < 0) {
		std::cerr << "Negative size!" << std::endl;
		THROW(ErrGeneric());
	}

	ASSERT((piRowm1 == NULL && pdVecm1 == NULL)
			|| (piRowm1 != NULL && pdVecm1 != NULL));

	if (!bOwnsMemory && piRowm1 != NULL) {
		if (iSize > iMaxSize) {
			std::cerr << "Can't resize to " << iSize
				<< ": larger than max size " << iMaxSize
				<< std::endl;
			THROW(ErrGeneric());
		}
		iCurSize = iSize;

	} else {
		if (piRowm1 != NULL) {
			if (iSize < iMaxSize) {
				iCurSize = iSize;

			} else {
				doublereal* pd = NULL;
				SAFENEWARR(pd, doublereal, iSize);
				pd--;

				integer* pi = NULL;
				SAFENEWARR(pi, integer, iSize);
				pi--;
				
				for (integer i = iCurSize; i > 0; i--) {
					pd[i] = pdVecm1[i];
	       				pi[i] = piRowm1[i];
	    			}

				pdVecm1++;
	    			SAFEDELETEARR(pdVecm1);

				piRowm1++;
	    			SAFEDELETEARR(piRowm1);
	    			
				pdVecm1 = pd;
				piRowm1 = pi;
				iMaxSize = iCurSize = iSize;
			}

		} else {
			if (iSize > 0) {
				SAFENEWARR(pdVecm1, doublereal, iSize);
				pdVecm1--;
				
				SAFENEWARR(piRowm1, integer, iSize);
				piRowm1--;
				
				iMaxSize = iCurSize = iSize;
			}
		}
	}
}

void
MySubVectorHandler::Detach(void)
{
	if (bOwnsMemory) {
		if (pdVecm1 != NULL) {
			pdVecm1++;
			SAFEDELETEARR(pdVecm1);

			piRowm1++;
			SAFEDELETEARR(piRowm1);
		}
		
		bOwnsMemory = false;
	}
	
	iMaxSize = iCurSize = 0;
	pdVecm1 = NULL;
	piRowm1 = NULL;
}

void
MySubVectorHandler::Attach(integer iSize, doublereal* pd,
		integer* pi, integer iMSize)
{
	if (bOwnsMemory && pdVecm1 != NULL) {
		Detach();
		bOwnsMemory = false;
	}

	iMaxSize = iCurSize = iSize;
	if (iMSize >= iSize) {
		iMaxSize = iMSize;
	}

	pdVecm1 = pd - 1;
	piRowm1 = pi - 1;
}

#ifdef DEBUG
void
MySubVectorHandler::IsValid(void) const
{
	MyVectorHandler::IsValid();

	ASSERT(piRowm1 != NULL);

#ifdef DEBUG_MEMMANAGER
	ASSERT(defaultMemoryManager.fIsValid(piRowm1 + 1,
				MyVectorHandler::iMaxSize*sizeof(integer)));
#endif /* DEBUG_MEMMANAGER */
}
#endif /* DEBUG */

VectorHandler&
MySubVectorHandler::AddTo(VectorHandler& VH) const
{
#ifdef DEBUG
	IsValid();
	VH.IsValid();
#endif /* DEBUG */

	for (integer i = iGetSize(); i > 0; i--) {
		VH(piRowm1[i]) += pdVecm1[i];
	}

	return VH;
}

VectorHandler&
MySubVectorHandler::AddTo(MyVectorHandler& VH) const
{
#ifdef DEBUG
	IsValid();
	VH.IsValid();
#endif /* DEBUG */

	doublereal* pdm1 = VH.pdGetVec() - 1;
	for (integer i = iGetSize(); i > 0; i--) {
		pdm1[piRowm1[i]] += pdVecm1[i];
	}
	return VH;
}

std::ostream&
operator << (std::ostream& out, const SubVectorHandler& v)
{
#ifdef DEBUG
	v.IsValid();
#endif /* DEBUG */

	integer iRow = v.iGetSize();

	ASSERT(iRow > 0);

	for (integer i = 1; i <= iRow; i++) {
		out << std::setw(12) << v.iGetRowIndex(i)
			<< std::setw(12) << v(i) << std::endl;
	}

	return out << std::endl;
}

/* MySubVectorHandler - end */
