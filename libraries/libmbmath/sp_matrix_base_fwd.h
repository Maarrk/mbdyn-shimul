/*
 * MBDyn (C) is a multibody analysis code.
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2020
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
 AUTHOR: Reinhard Resch <mbdyn-user@a1.net>
	Copyright (C) 2020(-2020) all rights reserved.

	The copyright of this code is transferred
	to Pierangelo Masarati and Paolo Mantegazza
	for use in the software MBDyn as described
	in the GNU Public License version 2.1
*/

#ifndef __SP_MATRIX_BASE_FWD_H__INCLUDED__
#define __SP_MATRIX_BASE_FWD_H__INCLUDED__

#include "sp_gradient_base.h"

namespace sp_grad {
     enum SpMatrixSize: index_type {
	  DYNAMIC = -1
     };

     enum class SpMatOpType: index_type {
	  SCALAR,
	  MATRIX
     };

     template <typename ValueType, index_type NumRows = SpMatrixSize::DYNAMIC, index_type NumCols = SpMatrixSize::DYNAMIC>
     class SpMatrixBase;

     template <typename ValueType>
     class SpMatrixData;

     namespace util {
	  template <typename T>
	  struct remove_all {
	       typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
	  };

	  template <typename ValueType>
	  struct SpMatrixDataTraits {
	       static inline void Construct(SpMatrixData<ValueType>& oData, index_type iNumDeriv, void* pExtraMem);
	       static constexpr inline size_t uOffsetDeriv(size_t uSizeGrad) noexcept;
	       static constexpr inline size_t uSizeDeriv(index_type iNumDeriv, index_type iNumItems) noexcept;
	       static constexpr inline size_t uAlign() noexcept;
	  };

	  template <>
	  struct SpMatrixDataTraits<SpGradient> {
	       static inline void Construct(SpMatrixData<SpGradient>& oData, index_type iNumDeriv, void* pExtraMem);
	       static constexpr inline size_t uOffsetDeriv(size_t uSizeGrad) noexcept;
	       static constexpr inline size_t uSizeDeriv(index_type iNumDeriv, index_type iNumItems) noexcept;
	       static constexpr inline size_t uAlign() noexcept;
	  };
     }

     template <typename ValueType>
     class SpMatrixBaseData {
     protected:
	  inline SpMatrixBaseData(index_type iNumRows,
				  index_type iNumCols,
				  index_type iRefCnt,
				  index_type iNumDeriv) noexcept;

     public:
	  inline index_type iGetNumRows() const noexcept;
	  inline index_type iGetNumCols() const noexcept;
	  inline index_type iGetNumElem() const noexcept;
	  inline index_type iGetRefCnt() const noexcept;
	  inline index_type iGetMaxDeriv() const noexcept;
	  inline bool
	  bCheckSize(index_type iNumRowsReq,
		     index_type iNumColsReq,
		     index_type iNumDerivReq) const noexcept;

	  inline static SpMatrixBaseData* pGetNullData() noexcept;

     protected:
	  index_type iNumRows;
	  index_type iNumCols;
#ifdef USE_MULTITHREAD
	  std::atomic<index_type> iRefCnt;
#else
	  index_type iRefCnt;
#endif
	  index_type iNumDeriv;
	  
	  ValueType rgData[0];
	  
     private:
	  static struct NullData {
	       NullData();
	       ~NullData();
	       SpMatrixBaseData* pNullData;
	  } oNullData;
     } SP_GRAD_ALIGNMENT(alignof(SpDerivData*));
     
     template <typename ValueType>
     class SpMatrixData: public SpMatrixBaseData<ValueType> {
     protected:
	  inline SpMatrixData(index_type iNumRows,
			      index_type iNumCols,
			      index_type iRefCnt,
			      index_type iNumDeriv,
			      void* pExtraMem);
	  inline ~SpMatrixData();

     public:
	  inline ValueType* pGetData() noexcept;
	  inline ValueType* pGetData(index_type iRow) noexcept;
	  inline const ValueType* pGetData() const noexcept;
	  inline const ValueType* pGetData(index_type iRow) const noexcept;
	  inline void Attach(ValueType* pData) noexcept;
	  inline void Detach(ValueType* pData);
	  inline void IncRef() noexcept;
	  inline void DecRef();
	  inline ValueType* begin() noexcept;
	  inline const ValueType* begin() const noexcept;
	  constexpr inline bool bIsOwnerOf(const ValueType* pData) const noexcept;	 
	  inline ValueType* end() noexcept;
	  inline const ValueType* end() const noexcept;
     };

     template <typename ValueType, index_type NumRows, index_type NumCols>
     class SpMatrixDataCTD: public SpMatrixData<ValueType> {
     private:
	  static constexpr index_type iNumRowsStatic = NumRows;
	  static constexpr index_type iNumColsStatic = NumCols;

	  inline SpMatrixDataCTD(index_type iNumRows,
				 index_type iNumCols,
				 index_type iRefCnt,
				 index_type iNumDeriv,
				 void* pExtraMem);
     public:
	  using SpMatrixData<ValueType>::pGetData;
	  inline constexpr index_type iGetNumRows() const noexcept;
	  inline constexpr index_type iGetNumCols() const noexcept;
	  inline constexpr index_type iGetNumElem() const noexcept;
	  inline ValueType* pGetData(index_type iRow, index_type iCol) noexcept;
	  inline const ValueType* pGetData(index_type iRow, index_type iCol) const noexcept;
	  inline ValueType* end() noexcept;
	  inline const ValueType* end() const noexcept;
	  static inline SpMatrixDataCTD* pAllocate(index_type iNumRows, index_type iNumCols, index_type iNumDeriv);
     };

     namespace util {
	  enum class MatTranspEvalFlag: index_type  {
	       DIRECT,
	       TRANSPOSED
	  };

	  template <MatTranspEvalFlag eTransp, SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate>
	  struct MatEvalHelper;

	  template <SpGradCommon::ExprEvalFlags eCompr>
	  struct MatEvalHelperCompr;

	  template <>
	  struct MatEvalHelperCompr<SpGradCommon::ExprEvalDuplicate> {
	       static constexpr SpGradCommon::ExprEvalFlags eExprEvalFlags = SpGradCommon::ExprEvalDuplicate;

	       template <MatTranspEvalFlag eTransp, typename Expr, typename ValueType, index_type NumRows, index_type NumCols>
	       static inline void ElemEval(const Expr& oExpr, SpMatrixBase<ValueType, NumRows, NumCols>& A);
	  };

	  template <>
	  struct MatEvalHelperCompr<SpGradCommon::ExprEvalUnique> {
	       static constexpr SpGradCommon::ExprEvalFlags eExprEvalFlags = SpGradCommon::ExprEvalUnique;

	       template <MatTranspEvalFlag eTransp, typename Expr, typename ValueType, index_type NumRows, index_type NumCols>
	       static inline void ElemEval(const Expr& oExpr, SpMatrixBase<ValueType, NumRows, NumCols>& A);
	  };

	  template <MatTranspEvalFlag eTransp>
	  struct MatEvalHelperTransp;

	  template <>
	  struct MatEvalHelperTransp<MatTranspEvalFlag::DIRECT> {
	       static constexpr MatTranspEvalFlag eTranspFlag = MatTranspEvalFlag::DIRECT;

	       template <typename ValueType, index_type NumRows, index_type NumCols>
	       static inline void ResizeReset(SpMatrixBase<ValueType, NumRows, NumCols>& A,
					      index_type iNumRows,
					      index_type iNumCols,
					      index_type iMaxSize);

	       template <typename ValueType, index_type NumRows, index_type NumCols>
	       static inline ValueType& GetElem(SpMatrixBase<ValueType, NumRows, NumCols>& A,
						index_type i,
						index_type j);
	  };

	  template <>
	  struct MatEvalHelperTransp<MatTranspEvalFlag::TRANSPOSED> {
	       static constexpr MatTranspEvalFlag eTranspFlag = MatTranspEvalFlag::TRANSPOSED;

	       template <typename ValueType, index_type NumRows, index_type NumCols>
	       static inline void ResizeReset(SpMatrixBase<ValueType, NumRows, NumCols>& A,
					      index_type iNumRows,
					      index_type iNumCols,
					      index_type iMaxSize);

	       template <typename ValueType, index_type NumRows, index_type NumCols>
	       static inline ValueType& GetElem(SpMatrixBase<ValueType, NumRows, NumCols>& A,
						index_type i,
						index_type j);
	  };

	  template <MatTranspEvalFlag eTransp> struct TranspMatEvalHelper;

	  template <>
	  struct TranspMatEvalHelper<MatTranspEvalFlag::DIRECT> {
	       static constexpr MatTranspEvalFlag Value = MatTranspEvalFlag::TRANSPOSED;
	  };

	  template <>
	  struct TranspMatEvalHelper<MatTranspEvalFlag::TRANSPOSED> {
	       static constexpr MatTranspEvalFlag Value = MatTranspEvalFlag::DIRECT;
	  };

	  enum MatAccessFlag: unsigned {
	       ELEMENT_WISE = 0x1,
	       ITERATORS = 0x2,
	       MATRIX_WISE = 0x4
	  };
     };

     template <typename VALUE, typename DERIVED>
     class SpMatElemExprBase
     {
	  static_assert(std::is_same<VALUE, doublereal>::value || std::is_same<VALUE, SpGradient>::value);
	  
     protected:
	  constexpr SpMatElemExprBase() noexcept {}
	  ~SpMatElemExprBase() noexcept {}

     public:
	  typedef VALUE ValueType;
	  typedef DERIVED DerivedType;
	  static constexpr index_type iNumElemOps = DERIVED::iNumElemOps;
	  static constexpr unsigned uMatAccess = DERIVED::uMatAccess;
	  static constexpr index_type iNumRowsStatic = DERIVED::iNumRowsStatic;
	  static constexpr index_type iNumColsStatic = DERIVED::iNumColsStatic;
	  static constexpr SpMatOpType eMatOpType = DERIVED::eMatOpType;
	  static constexpr SpGradCommon::ExprEvalFlags eExprEvalFlags = DERIVED::eExprEvalFlags;

	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemEval(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;

	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename Func, typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemAssign(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;
	  
	  
	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename ValueTypeA,
		    index_type NumRowsA, index_type NumColsA>
	  inline void Eval(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const {
	       pGetRep()->template Eval<eTransp, eCompr>(A);
	  }

	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename Func,
		    typename ValueTypeA,
		    index_type NumRowsA, index_type NumColsA>
	  inline void AssignEval(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const {
	       pGetRep()->template AssignEval<eTransp, eCompr, Func>(A);
	  }
	  
	  index_type iGetNumRows() const {
	       return pGetRep()->iGetNumRows();
	  }

	  index_type iGetNumCols() const {
	       return pGetRep()->iGetNumCols();
	  }

	  constexpr doublereal dGetValue(index_type i, index_type j) const {
	       SP_GRAD_ASSERT(i >= 1);
	       SP_GRAD_ASSERT(i <= iGetNumRows() || iGetNumRows() == SpMatrixSize::DYNAMIC);
	       SP_GRAD_ASSERT(j >= 1);
	       SP_GRAD_ASSERT(j <= iGetNumCols() || iGetNumCols() == SpMatrixSize::DYNAMIC);
	       return pGetRep()->dGetValue(i, j);
	  }

	  template <typename ExprType, typename Expr>
	  constexpr bool bHaveRefTo(const SpMatElemExprBase<ExprType, Expr>& A) const {
	       return pGetRep()->bHaveRefTo(A);
	  }

	  constexpr index_type iGetSize(index_type i, index_type j) const {
	       return pGetRep()->iGetSize(i, j);
	  }

	  index_type iGetMaxSizeElem() const {
	       index_type iMaxSize = 0;

	       for (index_type j = 1; j <= iGetNumCols(); ++j) {
		    for (index_type i = 1; i <= iGetNumRows(); ++i) {
			 iMaxSize = std::max(iMaxSize, iGetSize(i, j));
		    }
	       }

	       return iMaxSize;
	  }

	  index_type iGetMaxSize() const {
	       return pGetRep()->iGetMaxSize();
	  }

	  template <typename ValueTypeB>
	  void InsertDeriv(ValueTypeB& g, doublereal dCoef, index_type i, index_type j) const {
	       pGetRep()->InsertDeriv(g, dCoef, i, j);
	  }

	  void GetDofStat(SpGradDofStat& s, index_type i, index_type j) const {
	       pGetRep()->GetDofStat(s, i, j);
	  }

	  void InsertDof(SpGradExpDofMap& oExpDofMap, index_type i, index_type j) const {
	       pGetRep()->InsertDof(oExpDofMap, i, j);
	  }

	  template <typename ValueTypeB>
	  void AddDeriv(ValueTypeB& g, doublereal dCoef, const SpGradExpDofMap& oExpDofMap, index_type i, index_type j) const {
	       pGetRep()->AddDeriv(g, dCoef, oExpDofMap, i, j);
	  }

	  const ValueType* begin() const {
	       return pGetRep()->begin();
	  }

	  const ValueType* end() const {
	       return pGetRep()->end();
	  }

	  inline constexpr index_type iGetRowOffset() const noexcept { return pGetRep()->iGetRowOffset(); }
	  inline constexpr index_type iGetColOffset() const noexcept { return pGetRep()->iGetColOffset(); }

	  constexpr const DERIVED* pGetRep() const noexcept {
	       return static_cast<const DERIVED*>(this);
	  }
	  
	  template <util::MatTranspEvalFlag eTransp, typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemEvalUncompr(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;

	  template <util::MatTranspEvalFlag eTransp, typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemEvalCompr(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;

	  template <util::MatTranspEvalFlag eTransp, typename Func, typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemAssignCompr(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;

	  template <util::MatTranspEvalFlag eTransp, typename Func, typename ValueTypeA, index_type NumRowsA, index_type NumColsA>
	  inline void ElemAssignUncompr(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const;	  
     };

     template <typename DERIVED>
     class SpConstMatElemAdapter: public SpMatElemExprBase<doublereal, DERIVED> {
	  // Provides simplified interfaces to Vec3, Mat3x3, Mat3xN, MatNx3 and MatNxN
     protected:
	  constexpr SpConstMatElemAdapter() noexcept {}
	  ~SpConstMatElemAdapter() noexcept {}

     public:	  
	  static constexpr index_type iNumElemOps = 0;
	  static constexpr unsigned uMatAccess = util::MatAccessFlag::ELEMENT_WISE |
	       util::MatAccessFlag::ITERATORS |
	       util::MatAccessFlag::MATRIX_WISE;
	  static constexpr SpMatOpType eMatOpType = SpMatOpType::MATRIX;
	  static constexpr SpGradCommon::ExprEvalFlags eExprEvalFlags = SpGradCommon::ExprEvalDuplicate;
	  inline constexpr index_type iGetSize(index_type i, index_type j) const noexcept { return 0; }
	  inline constexpr index_type iGetMaxSize() const noexcept { return 0; }
	  template <typename ValueType_B>
	  inline constexpr void InsertDeriv(ValueType_B& g, doublereal dCoef, index_type i, index_type j) const noexcept {}
	  inline constexpr void GetDofStat(SpGradDofStat& s, index_type i, index_type j) const noexcept {}
	  inline constexpr void InsertDof(SpGradExpDofMap& oExpDofMap, index_type i, index_type j) const noexcept {};
	  template <typename ValueTypeB>
	  inline constexpr void AddDeriv(ValueTypeB& g, doublereal dCoef, const SpGradExpDofMap& oExpDofMap, index_type i, index_type j) const noexcept {}
	  template <typename ExprType, typename Expr>
	  inline constexpr bool bHaveRefTo(const SpMatElemExprBase<ExprType, Expr>& A) const noexcept { return false; }
	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename ValueTypeA, index_type iNumRowsA, index_type iNumColsA>
	  void Eval(SpMatrixBase<ValueTypeA, iNumRowsA, iNumColsA>& A) const {
	       this->template ElemEval<eTransp, eCompr>(A);
	  }

	  template <util::MatTranspEvalFlag eTransp = util::MatTranspEvalFlag::DIRECT,
		    SpGradCommon::ExprEvalFlags eCompr = SpGradCommon::ExprEvalDuplicate,
		    typename Func,
		    typename ValueTypeA,
		    index_type NumRowsA, index_type NumColsA>
	  void AssignEval(SpMatrixBase<ValueTypeA, NumRowsA, NumColsA>& A) const {
	       this->template ElemAssignCompr<eTransp, Func>(A);
	  }
     };
}
#endif
