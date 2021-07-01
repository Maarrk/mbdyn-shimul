/* $Header$ */
/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2021
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
/*
 * Author: Andrea Zanoni <andrea.zanoni@polimi.it>
 *         Pierangelo Masarati <masarati@aero.polimi.it>
 */

#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */

#include <cmath>
#include <cfloat>

#include "dataman.h"
#include "constltp_impl.h"

class MusclePennestriCL
: public ElasticConstitutiveLaw<doublereal, doublereal> {
protected:
	doublereal Li;
	doublereal L0;
	doublereal V0;
	doublereal F0;
	DriveOwner Activation;
	bool bActivationOverflow;
	bool bActivationOverflowWarn;
	doublereal a;
	doublereal aReq;	// requested activation: for output only
	virtual std::ostream& Restart_int(std::ostream& out) const {
		return out;
	};
#ifdef USE_NETCDF
	MBDynNcVar Var_dAct;
	MBDynNcVar Var_dActReq;
#endif // USE_NETCDF
public:
	MusclePennestriCL(const TplDriveCaller<doublereal> *pTplDC, doublereal dPreStress,
		doublereal Li, doublereal L0, doublereal V0, doublereal F0,
		const DriveCaller *pAct, bool bActivationOverflow, bool bActivationOverflowWarn)
	: ElasticConstitutiveLaw<doublereal, doublereal>(pTplDC, dPreStress),
	Li(Li), L0(L0), V0(V0), F0(F0),
	Activation(pAct), bActivationOverflow(bActivationOverflow), bActivationOverflowWarn(bActivationOverflowWarn)
#ifdef USE_NETCDFC
	, Var_dAct(0), Var_dActReq(0)
#endif // USE_NETCDFC
	{
		NO_OP;
	};

	virtual ~MusclePennestriCL(void) {
		NO_OP;
	};

	virtual ConstLawType::Type GetConstLawType(void) const {
		return ConstLawType::VISCOELASTIC;
	};

	virtual ConstitutiveLaw<doublereal, doublereal>* pCopy(void) const {
		ConstitutiveLaw<doublereal, doublereal>* pCL = 0;

		// pass parameters to copy constructor
		SAFENEWWITHCONSTRUCTOR(pCL, MusclePennestriCL,
			MusclePennestriCL(pGetDriveCaller()->pCopy(),
				PreStress,
				Li, L0, V0, F0,
				Activation.pGetDriveCaller()->pCopy(),
				bActivationOverflow, 
				bActivationOverflowWarn));
		return pCL;
	};

	virtual std::ostream& Restart(std::ostream& out) const;
	virtual std::ostream& OutputAppend(std::ostream& out) const;
	virtual void NetCDFOutputAppend(OutputHandler& OH) const;
	virtual void OutputAppendPrepare(OutputHandler& OH, const std::string& name);

	virtual void Update(const doublereal& Eps, const doublereal& EpsPrime);
};

class MusclePennestriErgoCL
: public MusclePennestriCL {
public:
	MusclePennestriErgoCL(const TplDriveCaller<doublereal> *pTplDC, doublereal dPreStress,
		doublereal Li, doublereal L0, doublereal V0, doublereal F0,
		const DriveCaller *pAct, bool bActivationOverflow, bool bActivationOverflowWarn)
	: MusclePennestriCL(pTplDC, dPreStress, Li, L0, V0, F0, pAct, bActivationOverflow, bActivationOverflowWarn)
	{
		NO_OP;
	};

	virtual ~MusclePennestriErgoCL(void) {
		NO_OP;
	};

	virtual ConstLawType::Type GetConstLawType(void) const {
		return ConstLawType::ELASTIC;
	};

	virtual ConstitutiveLaw<doublereal, doublereal>* pCopy(void) const {
		ConstitutiveLaw<doublereal, doublereal>* pCL = 0;

		// pass parameters to copy constructor
		SAFENEWWITHCONSTRUCTOR(pCL, MusclePennestriErgoCL,
			MusclePennestriErgoCL(pGetDriveCaller()->pCopy(),
				PreStress,
				Li, L0, V0, F0,
				Activation.pGetDriveCaller()->pCopy(),
				bActivationOverflow, 
				bActivationOverflowWarn));
		return pCL;
	};

	virtual void Update(const doublereal& Eps, const doublereal& EpsPrime) {
		MusclePennestriCL::Update(Eps, 0.);
	};

protected:
	virtual std::ostream& Restart_int(std::ostream& out) const {
		out << ", ergonomy, yes";
		return out;
	};
};

class MusclePennestriReflexiveCL
: public MusclePennestriCL {
protected:
	DriveOwner Kp;
	DriveOwner Kd;
	DriveOwner ReferenceLength;
	
public:
	MusclePennestriReflexiveCL(const TplDriveCaller<doublereal> *pTplDC, doublereal dPreStress,
		doublereal Li, doublereal L0, doublereal V0, doublereal F0,
		const DriveCaller *pAct, bool bActivationOverflow, bool bActivationOverflowWarn,
		const DriveCaller *pKp, const DriveCaller *pKd, const DriveCaller *pReferenceLength)
	: MusclePennestriCL(pTplDC, dPreStress, Li, L0, V0, F0, pAct, bActivationOverflow, bActivationOverflowWarn),
	Kp(pKp), Kd(pKd), ReferenceLength(pReferenceLength)
	{
		NO_OP;
	};

	virtual ~MusclePennestriReflexiveCL(void) {
		NO_OP;
	};

	virtual ConstitutiveLaw<doublereal, doublereal>* pCopy(void) const {
		ConstitutiveLaw<doublereal, doublereal>* pCL = 0;

		// pass parameters to copy constructor
		SAFENEWWITHCONSTRUCTOR(pCL, MusclePennestriReflexiveCL,
			MusclePennestriReflexiveCL(pGetDriveCaller()->pCopy(),
				PreStress,
				Li, L0, V0, F0,
				Activation.pGetDriveCaller()->pCopy(),
				bActivationOverflow,
				bActivationOverflowWarn,
				Kp.pGetDriveCaller()->pCopy(), Kd.pGetDriveCaller()->pCopy(),
				ReferenceLength.pGetDriveCaller()->pCopy()));
		return pCL;
	};

	virtual void Update(const doublereal& Eps, const doublereal& EpsPrime);

protected:
	virtual std::ostream& Restart_int(std::ostream& out) const;
};


class MusclePennestriReflexiveCLWithSRS
: public MusclePennestriCL {
public:
	enum SRSModel {
		SRS_LINEAR,
		SRS_EXPONENTIAL
	} m_SRSModel;
protected:
	DriveOwner Kp;
	DriveOwner Kd;
	DriveOwner ReferenceLength;
	doublereal SRSGamma;
	doublereal SRSDelta;

public:
	MusclePennestriReflexiveCLWithSRS(const TplDriveCaller<doublereal> *pTplDC, doublereal dPreStress,
		doublereal Li, doublereal L0, doublereal V0, doublereal F0,
		const DriveCaller *pAct, bool bActivationOverflow, bool bActivationOverflowWarn,
		const DriveCaller *pKp, const DriveCaller *pKd, const DriveCaller *pReferenceLength,
		const doublereal SRSGamma, const doublereal SRSDelta, 
		const SRSModel m_SRSModel)
	: MusclePennestriCL(pTplDC, dPreStress, Li, L0, V0, F0, pAct, bActivationOverflow, bActivationOverflowWarn),
	Kp(pKp), Kd(pKd), ReferenceLength(pReferenceLength), SRSGamma(SRSGamma), SRSDelta(SRSDelta), m_SRSModel(m_SRSModel)
	{
		NO_OP;
	};

	virtual ~MusclePennestriReflexiveCLWithSRS(void) {
		NO_OP;
	}

	virtual ConstitutiveLaw<doublereal, doublereal>* pCopy(void) const {
		ConstitutiveLaw<doublereal, doublereal>* pCL = 0;

		// pass parameters to copy constructor
		SAFENEWWITHCONSTRUCTOR(pCL, MusclePennestriReflexiveCLWithSRS,
			MusclePennestriReflexiveCLWithSRS(pGetDriveCaller()->pCopy(),
				PreStress,
				Li, L0, V0, F0,
				Activation.pGetDriveCaller()->pCopy(),
				bActivationOverflow,
				bActivationOverflowWarn,
				Kp.pGetDriveCaller()->pCopy(), Kd.pGetDriveCaller()->pCopy(),
				ReferenceLength.pGetDriveCaller()->pCopy(),
				SRSGamma, SRSDelta, m_SRSModel));
		return pCL;
	};

	virtual void Update(const doublereal& Eps, const doublereal& EpsPrime);
};
