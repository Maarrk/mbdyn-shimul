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

/* Rods */

#ifndef RODJ_H
#define RODJ_H

#include "joint.h"
#include "constltp.h"

extern const char* psRodNames[];


/* Rod - begin */

class Rod : 
virtual public Elem, public Joint, public ConstitutiveLaw1DOwner {
public:
	/* Tipi di rods */
	enum Type {
		UNKNOWN = -1,
	
		ELASTIC = 0,
		VISCOELASTIC,
		VISCOELASTICWITHOFFSET,
	
		LASTRODTYPE
	};

private: 
	Rod::Type RodT;
   
protected:
	const StructNode* pNode1;
	const StructNode* pNode2;
	doublereal dL0;
   
	Vec3 v;
	doublereal dElle;

	doublereal dEpsilon;
	doublereal dEpsilonPrime;
   
	/* Le funzioni di assemblaggio sono le stesse, cambiano gli indici 
	 * delle equazioni. Allora, dopo aver settato indici e matrici,
	 * le routines normali chiamano queste due che eseguono i calcoli 
	 *
	 * Purtroppo questa semplificazione vale solo per i Rod senza offset 
	 * e puramente elastici. Allora non dichiaro le funzioni come virtuali
	 * in quanto non devono essere usate direttamente da classi derivate
	 */
	void AssMat(FullSubMatrixHandler& WorkMat, doublereal dCoef = 1.);
	void AssVec(SubVectorHandler& WorkVec);
   
	/* Sets the type */
	void SetRodType(Rod::Type T) { 
		RodT = T;
	};

public:
	/* Costruttore non banale */
	Rod(unsigned int uL, const DofOwner* pDO,
			const ConstitutiveLaw1D* pCL,
			const StructNode* pN1, const StructNode* pN2,
			doublereal dLength, flag fOut,
			flag fHasOffsets = 0);

	/* Distruttore */
	virtual ~Rod(void);

	virtual inline void* pGet(void) const { 
		return (void*)this;
	};

	/* Tipo di Joint */
	virtual Joint::Type GetJointType(void) const {
		return Joint::ROD; 
	};
      
	/* Contributo al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	virtual void
	AfterConvergence(const VectorHandler& X, const VectorHandler& XP);

	/* Tipo di Rod */
	virtual Rod::Type GetRodType(void) const {
		return Rod::ELASTIC; 
	};

	virtual unsigned int iGetNumDof(void) const { 
		return 0;
	};

	virtual void
	WorkSpaceDim(integer* piNumRows, integer* piNumCols) const { 
		*piNumRows = 6; 
		*piNumCols = 6; 
	};

	virtual VariableSubMatrixHandler&
	AssJac(VariableSubMatrixHandler& WorkMat, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);
	   
	virtual void
	AssMats(VariableSubMatrixHandler& WorkMatA, 
 			VariableSubMatrixHandler& WorkMatB,
 			const VectorHandler& XCurr, 
 			const VectorHandler& XPrimeCurr);
	
	virtual SubVectorHandler&
	AssRes(SubVectorHandler& WorkVec, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);
   
	virtual void Output(OutputHandler& OH) const;

	/* Output di un modello NASTRAN equivalente
	 * nella configurazione corrente */
	virtual void Output_pch(std::ostream& out) const;

	/* funzioni usate nell'assemblaggio iniziale */
	virtual unsigned int iGetInitialNumDof(void) const {
		return 0;
	};
	virtual void
	InitialWorkSpaceDim(integer* piNumRows, integer* piNumCols) const { 
		*piNumRows = 6; 
		*piNumCols = 6; 
	};

	/* Contributo allo jacobiano durante l'assemblaggio iniziale */
	virtual VariableSubMatrixHandler& 
	InitialAssJac(VariableSubMatrixHandler& WorkMat, 
     			const VectorHandler& XCurr);

	/* Contributo al residuo durante l'assemblaggio iniziale */   
	virtual SubVectorHandler&
	InitialAssRes(SubVectorHandler& WorkVec, const VectorHandler& XCurr);   

	/* *******PER IL SOLUTORE PARALLELO******** */        
	/* Fornisce il tipo e la label dei nodi che sono connessi all'elemento
	 * utile per l'assemblaggio della matrice di connessione fra i dofs */
	virtual void
	GetConnectedNodes(int& NumNodes, Node::Type* NdTyps,
			unsigned int* NdLabels) {
		NumNodes = 2;
		NdTyps[0] = pNode1->GetNodeType();
		NdLabels[0] = pNode1->GetLabel();
		NdTyps[1] = pNode2->GetNodeType();
		NdLabels[1] = pNode2->GetLabel();
	};
	/* ************************************************ */

	/* Adams output stuff */
	virtual unsigned int iGetNumDummyParts(void) const {
		return 1;
	};
	virtual void
	GetDummyPartPos(unsigned int part, Vec3& x, Mat3x3& R) const;
	virtual void
	GetDummyPartVel(unsigned int part, Vec3& v, Vec3& w) const;
#ifdef USE_ADAMS	
	virtual std::ostream&
	WriteAdamsDummyPartCmd(std::ostream& out, unsigned int part,
			unsigned int firstId) const;
#endif /* USE_ADAMS */

	virtual unsigned int iGetNumPrivData(void) const;
	virtual unsigned int iGetPrivDataIdx(const char *s) const;
	virtual doublereal dGetPrivData(unsigned int i) const;
};

/* Rod - end */


/* ViscoElasticRod - begin */

class ViscoElasticRod : virtual public Elem, public Rod {
public:
	/* Costruttore non banale */
	ViscoElasticRod(unsigned int uL, const DofOwner* pDO,
			const ConstitutiveLaw1D* pCL,
			const StructNode* pN1, const StructNode* pN2,
			doublereal dLength, flag fOut);
   
	/* Distruttore */
	virtual ~ViscoElasticRod(void);

	virtual inline void* pGet(void) const { 
		return (void*)this;
	};

	virtual void
	AfterConvergence(const VectorHandler& X, const VectorHandler& XP);

	/* Tipo di Rod */
	virtual Rod::Type GetRodType(void) const {
		return Rod::VISCOELASTIC; 
	};

	virtual VariableSubMatrixHandler&
	AssJac(VariableSubMatrixHandler& WorkMat, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);
	
	virtual SubVectorHandler&
	AssRes(SubVectorHandler& WorkVec, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);

     	virtual void
	InitialWorkSpaceDim(integer* piNumRows, integer* piNumCols) const {
		*piNumRows = 6; 
		*piNumCols = 12;
	};
   
	/* Contributo allo jacobiano durante l'assemblaggio iniziale */
	virtual VariableSubMatrixHandler& 
	InitialAssJac(VariableSubMatrixHandler& WorkMat, 
     			const VectorHandler& XCurr);
   
	/* Contributo al residuo durante l'assemblaggio iniziale */   
	virtual SubVectorHandler&
	InitialAssRes(SubVectorHandler& WorkVec,
			const VectorHandler& XCurr);   
};

/* ViscoElasticRod - end */


/* RodWithOffset - begin */

class RodWithOffset : virtual public Elem, public Rod {
protected:
	const Vec3 f1;
	const Vec3 f2;

public:
	/* Costruttore non banale */
	RodWithOffset(unsigned int uL, const DofOwner* pDO, 
  			const ConstitutiveLaw1D* pCL,
  			const StructNode* pN1, const StructNode* pN2,
  			const Vec3& f1Tmp, const Vec3& f2Tmp,
  			doublereal dLength, flag fOut);
 
	/* Distruttore */
	virtual ~RodWithOffset(void);

	virtual inline void* pGet(void) const { 
		return (void*)this;
	};

	/* Contributo al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	virtual void
	AfterConvergence(const VectorHandler& X, const VectorHandler& XP);

	/* Tipo di Rod */
	virtual Rod::Type GetRodType(void) const { 
		return Rod::VISCOELASTICWITHOFFSET; 
	};
   
	virtual void
	WorkSpaceDim(integer* piNumRows, integer* piNumCols) const { 
		*piNumRows = 12;
		*piNumCols = 12; 
	};
         
	virtual VariableSubMatrixHandler&
	AssJac(VariableSubMatrixHandler& WorkMat, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);
	   
	virtual SubVectorHandler&
	AssRes(SubVectorHandler& WorkVec, doublereal dCoef,
			const VectorHandler& XCurr, 
			const VectorHandler& XPrimeCurr);

	/* Output di un modello NASTRAN equivalente
	 * nella configurazione corrente */
	virtual void Output_pch(std::ostream& out) const;

	/* funzioni usate nell'assemblaggio iniziale */
   
	virtual void
	InitialWorkSpaceDim(integer* piNumRows, integer* piNumCols) const {
		*piNumRows = 12;
		*piNumCols = 24; 
	};

	/* Contributo allo jacobiano durante l'assemblaggio iniziale */
	virtual VariableSubMatrixHandler& 
	InitialAssJac(VariableSubMatrixHandler& WorkMat, 
     			const VectorHandler& XCurr);
   
	/* Contributo al residuo durante l'assemblaggio iniziale */   
	virtual SubVectorHandler&
	InitialAssRes(SubVectorHandler& WorkVec, const VectorHandler& XCurr);   

	void GetDummyPartPos(unsigned int part, Vec3& x, Mat3x3& R) const;
	void GetDummyPartVel(unsigned int part, Vec3& v, Vec3& w) const;
#ifdef USE_ADAMS
	std::ostream& WriteAdamsDummyPartCmd(std::ostream& out,
		unsigned int part, unsigned int firstId) const;
#endif /* USE_ADAMS */
};

/* RodWithOffset - end */

#endif /* RODJ_H */

