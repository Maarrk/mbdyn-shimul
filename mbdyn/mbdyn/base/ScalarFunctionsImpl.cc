/* 
 * HmFe (C) is a FEM analysis code. 
 *
 * Copyright (C) 1996-2003
 *
 * Marco Morandini  <morandini@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
/*
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2003
 * 
 * This code is a partial merge of HmFe and MBDyn.
 *
 * Pierangelo Masarati  <masarati@aero.polimi.it>
 * Paolo Mantegazza     <mantegazza@aero.polimi.it>
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
#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include "myassert.h"

#include "ScalarFunctionsImpl.h"
#include "interp.h"

//ptr_cast: I use an helper stuct to avoid troubles
//to be re-written when compilers will
//support partial specialization of functions template

//notare che non ho StlOverload.C dove metto il codice dei metodi statici,
//in teoria cosi' non e' corretto
template<class T1, class T2> struct ptr_cast_helper {
        static T1 cast(T2&);
};
template<class T1, class T2> struct ptr_cast_helper<T1*, T2> {
        static T1*  cast(T2&arg) {
                T1* out = dynamic_cast<T1*>(arg);
                if (out == 0) {
#ifdef MYDEBUG
                        std::cerr << "ptr_cast error" << std::endl;
#endif
                        throw std::bad_cast();
                }
                return out;
        };
};
template<class T1, class T2> struct ptr_cast_helper<T1*const, T2> {
        static T1*const  cast(T2&arg) {
                T1*const out = dynamic_cast<T1*const>(arg);
                if (out == 0) {
#ifdef MYDEBUG
                        std::cerr << "ptr_cast error" << std::endl;
#endif
                        throw std::bad_cast();
                }
                return out;
        };
};
template<class T1, class T2> struct ptr_cast_helper<const T1*, T2> {
        static const T1* cast(T2&arg) {
                const T1* out = dynamic_cast<const T1*>(arg);
                if (out == 0) {
#ifdef MYDEBUG
                        std::cerr << "ptr_cast error" << std::endl;
#endif
                        throw std::bad_cast();
                }
                return out;
        };
};
template<class T1, class T2> struct ptr_cast_helper<const T1*const, T2> {
        static const T1*const  cast(T2&arg) {
                const T1*const out = dynamic_cast<const T1*const>(arg);
                if (out == 0) {
#ifdef MYDEBUG
                        std::cerr << "ptr_cast error" << std::endl;
#endif
                        throw std::bad_cast();
                }
                return out;
        };
};

template<class T1,class T2>
T1 ptr_cast(T2& arg) {
        return ptr_cast_helper<T1,T2>::cast(arg);
};


BasicScalarFunction::~BasicScalarFunction() {};

DifferentiableScalarFunction::~DifferentiableScalarFunction() {};

ConstScalarFunction::ConstScalarFunction(const doublereal v) : y(v) {
};

ConstScalarFunction::~ConstScalarFunction() {};
doublereal ConstScalarFunction::operator()(const doublereal x) const {return y;};
doublereal ConstScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in ConstScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	default:
		return 0;
		break;
	}
};

LinearScalarFunction::LinearScalarFunction(
	const doublereal y_i,
	const doublereal y_f,
	const doublereal t_i,
	const doublereal t_f) {
	ASSERTMSGBREAK(t_i != t_f, "LinearScalarFunction error, t_i == t_f");
	m=(y_f-y_i)/(t_f-t_i);
	y0=y_i-m*t_i;
};
LinearScalarFunction::~LinearScalarFunction() {};
doublereal LinearScalarFunction::operator()(const doublereal x) const {
	return (y0+m*x);
};
doublereal LinearScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in LinearScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	case 1: 
		return m;
		break;
	default:
		return 0;
		break;
	}
};

PowScalarFunction::PowScalarFunction(const doublereal p) : pw(p) {
};
PowScalarFunction::~PowScalarFunction() {};
doublereal PowScalarFunction::operator()(const doublereal x) const {
	return (pow(x,pw));
};
doublereal PowScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in PowScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	default:
		doublereal mul = 1.;
		for (integer i=0; i<order; i++) {
			mul *= (pw-i);
		}
		return mul*pow(x,pw-order);
		break;
	}
};

LogScalarFunction::LogScalarFunction(const doublereal ml) : mul_const(ml){
};
LogScalarFunction::~LogScalarFunction() {};
doublereal LogScalarFunction::operator()(const doublereal x) const {
	if (x>0.) {
		return log(x)*mul_const;
	} else {
#warning FIXME: bogus solution to the problem (avoid fp error)
		return 0.;
	}
	//will never reach this point :(
	return log(x)*mul_const;
};
doublereal LogScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in LogScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	case 1: 
		return mul_const/x;
		break;
	case 2: 
		return -mul_const/(x*x);
		break;
	case 3: 
		return 2.*mul_const/pow(x,3.);
		break;
	case 4: 
		return -6.*mul_const/pow(x,4.);
		break;
	default:
		std::cerr << "Error, LogScalarFunction::ComputeDiff "
			<< "with diff order " << order
			<< " while the maximum implemented is 4\n";
		THROW(ErrGeneric());
		break;
	}
};

SumScalarFunction::SumScalarFunction(
	const BasicScalarFunction*const b1,
	const BasicScalarFunction*const b2
) : 
	a1(ptr_cast<const DifferentiableScalarFunction*const>(b1)), 
	a2(ptr_cast<const DifferentiableScalarFunction*const>(b2)) {
};
SumScalarFunction::~SumScalarFunction() {};
doublereal SumScalarFunction::operator()(const doublereal x) const {
	doublereal t;
	t = a1->operator()(x);
	t += a2->operator()(x);
	return t;
};
doublereal SumScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in SumScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return operator()(x);
		break;
	default:
		return a1->ComputeDiff(x,order)+
			a2->ComputeDiff(x,order);
		break;
	}
};

MulScalarFunction::MulScalarFunction(
	const BasicScalarFunction*const b1,
	const BasicScalarFunction*const b2
) :
	a1(ptr_cast<const DifferentiableScalarFunction*const>(b1)),
	a2(ptr_cast<const DifferentiableScalarFunction*const>(b2)) {
};
MulScalarFunction::~MulScalarFunction() {};
doublereal MulScalarFunction::operator()(const doublereal x) const {
	doublereal t;
	t = a1->operator()(x);
	t *= a2->operator()(x);
	return t;
};
doublereal MulScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in MulScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	default:
		return a1->ComputeDiff(x,order)*a2->operator()(x)+
			a1->operator()(x)*a2->ComputeDiff(x,order);
		break;
	}
};

CubicSplineScalarFunction::CubicSplineScalarFunction(
	const std::vector<doublereal> y_i,
	const std::vector<doublereal> x_i) {
	Y_i = y_i;
	X_i = x_i;
	ASSERTMSGBREAK(Y_i.size() == X_i.size(), "CubicSplineScalarFunction error, Y_i.size() != X_i.size()");
	std::vector<doublereal>::iterator xi,xe;
	xi = X_i.begin();
	xe = X_i.end()-1;
	for (; xi != xe; xi++) {
		if (*xi >= *(xi+1)) {
			std::cerr << "CubicSplineScalarFunction error, X_i is not ordered\n";
			THROW(ErrGeneric());
		}
	}
	spline(X_i,Y_i,b,c,d);
};
CubicSplineScalarFunction::~CubicSplineScalarFunction() {
};
doublereal CubicSplineScalarFunction::operator()(const doublereal x) const {
	return seval(x,X_i,Y_i,b,c,d);
};
doublereal CubicSplineScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in CubicSplineScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return this->operator()(x);
		break;
	case 1: 
		return seval(x,X_i,Y_i,b,c,d,1);
		break;
	case 2: 
		return seval(x,X_i,Y_i,b,c,d,2);
		break;
	case 3: 
		return seval(x,X_i,Y_i,b,c,d,3);
		break;
	default:
		return 0.;
		break;
	}
};

MultiLinearScalarFunction::MultiLinearScalarFunction(
	const std::vector<doublereal> y_i,
	const std::vector<doublereal> x_i) {
	Y_i = y_i;
	X_i = x_i;
	ASSERTMSGBREAK(X_i.size() == Y_i.size(), "MultiLinearScalarFunction error, Y_i.size() != X_i.size()");
	std::vector<doublereal>::iterator xi,xe;
	xi = X_i.begin();
	xe = X_i.end()-1;
	for (; xi != xe; xi++) {
		if (*xi >= *(xi+1)) {
			std::cerr << "MultiLinearScalarFunction error, X_i is not ordered\n";
			THROW(ErrGeneric());
		}
	}
};
MultiLinearScalarFunction::~MultiLinearScalarFunction() {
};
doublereal MultiLinearScalarFunction::operator()(const doublereal x) const {
	return leval(x,X_i,Y_i);
};
doublereal MultiLinearScalarFunction::ComputeDiff(const doublereal x, const integer order) const {
	ASSERTMSGBREAK(order >=0, "Error in MultiLinearScalarFunction::ComputeDiff, order<0");
	switch (order) {
	case 0: 
		return operator()(x);
		break;
	case 1: 
		return leval(x,X_i,Y_i,order);
		break;
	default:
		return 0.;
		break;
	}
};




//---------------------------------------

const BasicScalarFunction *const ParseScalarFunction(MBDynParser& HP,
	DataManager* const pDM)
{
   const char* sKeyWords[] = { 
      "const",
	"linear",
	"pow",
	"log",
	"sum",
	"mul",
	"cubicspline",
	"multilinear",
	NULL
   };
	enum KeyWords { 
	   CONST = 0,
	     LINEAR,
	     POW,
	     LOG,
	     SUM,
	     MUL,
	     CUBICSPLINE,
	     MULTILINEAR,
	     LASTKEYWORD
	};
	/* token corrente */
	KeyWords FuncType;

	std::string func_name;
	
	KeyTable K(HP, sKeyWords);
	
	func_name = HP.GetStringWithDelims();
	
	if (!pDM->GetScalarFunction(func_name)) {
		FuncType = KeyWords(HP.IsKeyWord());
		switch (FuncType) {
		case CONST: {
			doublereal c = HP.GetReal();
			pDM->SetScalarFunction(func_name, 
				new ConstScalarFunction(c));
			break;
		}
		case POW: {
			doublereal p = HP.GetReal();
			pDM->SetScalarFunction(func_name,
				new PowScalarFunction(p));
			break;
		}
		case LOG: {
			doublereal m = HP.GetReal();
			pDM->SetScalarFunction(func_name,
				new LogScalarFunction(m));
			break;
		}
		case LINEAR: {
			doublereal y_i = HP.GetReal();
			doublereal y_f = HP.GetReal();
			doublereal t_i = HP.GetReal();
			doublereal t_f = HP.GetReal();
			pDM->SetScalarFunction(func_name,
				new LinearScalarFunction(y_i,y_f,t_i,t_f));
			break;
		}
		case CUBICSPLINE: {
			std::vector<doublereal> y_i;
			std::vector<doublereal> x_i;
			y_i.resize(3);
			x_i.resize(3);
			for (int i=0; i<3; i++) {
				x_i[i] = HP.GetReal();
				y_i[i] = HP.GetReal();
			}
			while (HP.IsArg()) {
				int size = x_i.size();
				x_i.resize(size+1);
				y_i.resize(size+1);
				x_i[size] = HP.GetReal();
				y_i[size] = HP.GetReal();
			}
			pDM->SetScalarFunction(func_name,
				new CubicSplineScalarFunction(y_i,x_i));
			break;
		}
		case MULTILINEAR: {
			std::vector<doublereal> y_i;
			std::vector<doublereal> x_i;
			y_i.resize(2);
			x_i.resize(2);
			for (int i=0; i<2; i++) {
				x_i[i] = HP.GetReal();
				y_i[i] = HP.GetReal();
			}
			while (HP.IsArg()) {
				int size = x_i.size();
				x_i.resize(size+1);
				y_i.resize(size+1);
				x_i[size] = HP.GetReal();
				y_i[size] = HP.GetReal();
			}
			pDM->SetScalarFunction(func_name,
				new MultiLinearScalarFunction(y_i,x_i));
			break;
		}
		case SUM: {
			const BasicScalarFunction *const
				f1(ParseScalarFunction(HP,pDM));
			const BasicScalarFunction *const 
				f2(ParseScalarFunction(HP,pDM));
			pDM->SetScalarFunction(func_name,
				new SumScalarFunction(f1,f2));
			break;
		}
		case MUL: {
			const BasicScalarFunction *const
				f1(ParseScalarFunction(HP,pDM));
			const BasicScalarFunction *const 
				f2(ParseScalarFunction(HP,pDM));
			pDM->SetScalarFunction(func_name,
				new MulScalarFunction(f1,f2));
			break;
		}
		default: {
			std::cerr << "unknown ScalarFunction type"
			<< " at line " << HP.GetLineData() << std::endl;       
			THROW(DataManager::ErrGeneric());
			break;
		}
		} 
	} else if (HP.IsKeyWord() != -1) {
		std::cerr << "Error: redefinition of \"" 
			<< func_name << "\" scalar function at line "
			<< HP.GetLineData() << std::endl;
		throw MBDynParser::ErrGeneric();
	}
	
	return 	pDM->GetScalarFunction(func_name);
}
