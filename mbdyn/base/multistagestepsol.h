#ifndef MULTISTAGESTEPSOL_H
#define MULTISTAGESTEPSOL_H

#include <unistd.h>
#include <cfloat>
#include <cmath>
#include <deque>

#include "myassert.h"
#include "mynewmem.h"
#include "except.h"
#include "solman.h"
#include "dataman.h"
#include "dofown.h"
#include "drive.h"
#include "nonlinpb.h"
#include "nonlin.h"
#include "stepsol.h"

/* Stage2Integrator - begin */
class Stage2Integrator :   
	public StepNIntegrator
{
protected:
	VectorHandler *pXPrev, *pXInte;
	VectorHandler *pXPrimePrev, *pXPrimeInte; 
public:
	Stage2Integrator(const integer MaxIt,
			const doublereal dT,
			const doublereal dSolutionTol,
			const bool bmod_res_test);

	virtual ~Stage2Integrator(void);

	virtual doublereal
	Advance(Solver* pS, 
			const doublereal TStep, 
			const doublereal dAlph, 
			const StepChange StType,
			std::deque<MyVectorHandler*>& qX,
	 		std::deque<MyVectorHandler*>& qXPrime,
			MyVectorHandler*const pX,
 			MyVectorHandler*const pXPrime,
			integer& EffIter,
			doublereal& Err,
			doublereal& SolErr);

protected:
	void PredictDofforStage1(const int DCount,
		const DofOrder::Order Order,
		const VectorHandler* const pSol = 0) const;
	virtual void PredictforStage1(void);

    void PredictDofforStage2(const int DCount,
		const DofOrder::Order Order,
		const VectorHandler* const pSol = 0) const;
	virtual void PredictforStage2(void);

    virtual doublereal 
     	dPredDerforStage1(const doublereal& dXm1,
			const doublereal& dXPm1) const = 0;
   
   	virtual doublereal 
     	dPredStateforStage1(const doublereal& dXm1,
			const doublereal& dXP,
			const doublereal& dXPm1) const = 0;   

   	virtual doublereal 
     	dPredDerAlgforStage1(const doublereal& dXm1,
			const doublereal& dXPm1)  const = 0;
		    
   	virtual doublereal 
     	dPredStateAlgforStage1(const doublereal& dXm1,
			const doublereal& dXP,
			const doublereal& dXPm1) const = 0;
   	
    virtual doublereal 
     	dPredDerforStage2(const doublereal& dXm1,
			const doublereal& dXm2,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const = 0;
   
   	virtual doublereal 
     	dPredStateforStage2(const doublereal& dXm1,
			const doublereal& dXm2,
			const doublereal& dXP,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const = 0;   

   	virtual doublereal 
     	dPredDerAlgforStage2(const doublereal& dXm1,
            const doublereal& dXm2,
			const doublereal& dXPm1,
			const doublereal& dXPm2)  const = 0;
		    
   	virtual doublereal 
     	dPredStateAlgforStage2(const doublereal& dXm1,
		 	const doublereal& dXm2,
			const doublereal& dXP,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const = 0;

	virtual void SetCoef(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep) = 0;

	virtual void SetCoefforStage1(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep) = 0;

    virtual void SetCoefforStage2(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep) = 0;

};
/* Stage2Integrator - end */

/* TunableBatheSolver - begin */

class TunableBatheSolver: 
	public Stage2Integrator
{
protected:
	DriveOwner Rho;
	DriveOwner AlgebraicRho;

	integer iStage;

	doublereal mp[2];
	doublereal np[2];
    
    doublereal gamma;

	doublereal a[2][2];
	doublereal b[3][2];

   
public:
	TunableBatheSolver(const doublereal Tl, 
			const doublereal dSolTol, 
			const integer iMaxIt,
			const DriveCaller* pRho,
			const DriveCaller* pAlgRho,
			const bool bmod_res_test);

	~TunableBatheSolver(void);

protected:
	void SetCoef(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep);

	void SetCoefforStage1(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep);

    void SetCoefforStage2(doublereal dT, 
			doublereal dAlpha,
			enum StepChange NewStep);

	void SetDriveHandler(const DriveHandler* pDH);

    doublereal 
	dPredDerforStage1(const doublereal& dXm1,
			const doublereal& dXPm1) const;
   
	doublereal 
	dPredStateforStage1(const doublereal& dXm1,
			const doublereal& dXP,
			const doublereal& dXPm1) const;

	doublereal 
	dPredDerAlgforStage1(const doublereal& dXm1,
			const doublereal& dXPm1) const;

	doublereal 
	dPredStateAlgforStage1(const doublereal& dXm1,
			const doublereal& dXP,
			const doublereal& dXPm1) const;
    
    doublereal 
	dPredDerforStage2(const doublereal& dXm1,
			const doublereal& dXm2,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const;
   
	doublereal 
	dPredStateforStage2(const doublereal& dXm1,
			const doublereal& dXm2,
			const doublereal& dXP,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const;

	doublereal 
	dPredDerAlgforStage2(const doublereal& dXm1,
            const doublereal& dXm2,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const;

	doublereal 
	dPredStateAlgforStage2(const doublereal& dXm1,
			const doublereal& dXm2,
			const doublereal& dXP,
			const doublereal& dXPm1,
			const doublereal& dXPm2) const;
};

/* TunableBatheSolver - end */
#endif