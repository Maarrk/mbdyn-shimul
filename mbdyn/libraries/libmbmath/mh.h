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

/*****************************************************************************
 *                                                                           *
 *                            SOLUTION MANAGER                               *
 *                                                                           *
 *****************************************************************************/

/* Pierangelo Masarati */


#ifndef MH_H
#define MH_H

#include "ac/math.h"
#include "ac/iostream"
#include "ac/f2c.h"

/* per il debugging */
#include "myassert.h"
#include "mynewmem.h"
#include "except.h"

#include "vh.h"

class SubMatrixHandler;
class VariableSubMatrixHandler;

/* MatrixHandler - begin */

class MatrixHandler {
public:
	class ErrGeneric {};
	class ErrRebuildMatrix {};

	/* read-only return value for sparse data structures */
	//static const doublereal dZero = 0.;

public:
	virtual ~MatrixHandler(void);

#ifdef DEBUG
	/* Usata per il debug */
	virtual void IsValid(void) const = 0;
#endif /* DEBUG */

	/* Resetta la matrice ecc. */
	virtual void Init(const doublereal& dResetVal = 0.) = 0;

	/* Ridimensiona la matrice */
	virtual void Resize(integer, integer) = 0;

	/* Ridimensiona ed inizializza. */
	virtual void ResizeInit(integer, integer, const doublereal&);

	/* Restituisce un puntatore all'array di reali della matrice */
	virtual inline doublereal* pdGetMat(void) const;

	/* Restituisce un puntatore al vettore delle righe */
	virtual inline integer* piGetRows(void) const;

	/* Restituisce un puntatore al vettore delle colonne */
	virtual inline integer* piGetCols(void) const;

	/* Impacchetta la matrice; restituisce il numero di elementi 
	 * diversi da zero */
	virtual integer PacMat(void);

	/* Resetta la matrice ecc. */
	virtual void Reset(const doublereal& dResetVal = 0.);

	/* Inserisce un coefficiente */
	virtual void
	PutCoef(integer iRow, integer iCol, const doublereal& dCoef) = 0;

	/* Incrementa un coefficiente - se non esiste lo crea */
	virtual void
	IncCoef(integer iRow, integer iCol, const doublereal& dCoef) = 0;

	/* Decrementa un coefficiente - se non esiste lo crea */
	virtual void
	DecCoef(integer iRow, integer iCol, const doublereal& dCoef) = 0;

	/* Restituisce un coefficiente - zero se non e' definito */
	virtual const doublereal&
	dGetCoef(integer iRow, integer iCol) const = 0;

	virtual const doublereal&
	operator () (integer iRow, integer iCol) const = 0;

	virtual doublereal&
	operator () (integer iRow, integer iCol) = 0;

	/* dimensioni */
	virtual integer iGetNumRows(void) const = 0;
	virtual integer iGetNumCols(void) const = 0;

	/* Overload di += usato per l'assemblaggio delle matrici */
	virtual MatrixHandler& operator += (const SubMatrixHandler& SubMH);

	/* Overload di -= usato per l'assemblaggio delle matrici */
	virtual MatrixHandler& operator -= (const SubMatrixHandler& SubMH);

	/* Overload di += usato per l'assemblaggio delle matrici
	 * questi li vuole ma non so bene perche'; force per la doppia
	 * derivazione di VariableSubMatrixHandler */
	virtual MatrixHandler&
	operator += (const VariableSubMatrixHandler& SubMH);
	virtual MatrixHandler&
	operator -= (const VariableSubMatrixHandler& SubMH);

	/* */
	virtual MatrixHandler& ScalarMul(const doublereal& d);

	virtual VectorHandler&
	MatVecMul(VectorHandler& out, const VectorHandler& in) const;
	virtual VectorHandler&
	MatTVecMul(VectorHandler& out, const VectorHandler& in) const;
	virtual VectorHandler&
	MatVecIncMul(VectorHandler& out, const VectorHandler& in) const;
	virtual VectorHandler&
	MatTVecIncMul(VectorHandler& out, const VectorHandler& in) const;
};

/* Restituisce un puntatore all'array di reali della matrice */
inline doublereal*
MatrixHandler::pdGetMat(void) const
{
	return NULL;
}

/* Restituisce un puntatore al vettore delle righe */
inline integer*
MatrixHandler::piGetRows(void) const
{
	return NULL;
}

/* Restituisce un puntatore al vettore delle colonne */
inline integer*
MatrixHandler::piGetCols(void) const
{
	return NULL;
}

extern std::ostream&
operator << (std::ostream& out, const MatrixHandler& MH);

/* MatrixHandler - end */

#endif /* MH_H */

