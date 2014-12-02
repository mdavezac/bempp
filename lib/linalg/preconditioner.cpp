// Copyright (C) 2011-2012 by the BEM++ Authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "bempp/common/config_trilinos.hpp"
#include "bempp/common/config_ahmed.hpp"

#if defined(WITH_TRILINOS)

#include "preconditioner.hpp"

#include "../assembly/discrete_blocked_boundary_operator.hpp"
#include "../assembly/discrete_boundary_operator.hpp"
#include "../fiber/_2d_array.hpp"
#include "../fiber/explicit_instantiation.hpp"
#include "../fiber/scalar_traits.hpp"

#include <Teuchos_RCP.hpp>
#include <Teuchos_RCPBoostSharedPtrConversions.hpp>
#include <Thyra_LinearOpBase.hpp>
#include <Thyra_PreconditionerBase.hpp>
#include <Thyra_DefaultPreconditioner.hpp>

namespace Bempp {

template <typename ValueType>
Preconditioner<ValueType>::Preconditioner(TeuchosPreconditionerPtr precPtr)
    : m_precPtr(precPtr) {}

template <typename ValueType> Preconditioner<ValueType>::~Preconditioner() {}

template <typename ValueType>
Preconditioner<ValueType> discreteOperatorToPreconditioner(const shared_ptr<
    const DiscreteBoundaryOperator<ValueType>> &discreteOperator) {
  Teuchos::RCP<const DiscreteBoundaryOperator<ValueType>> op =
      Teuchos::rcp(discreteOperator);

  typename Preconditioner<ValueType>::TeuchosPreconditionerPtr precOp =
      Teuchos::rcp_static_cast<const Thyra::PreconditionerBase<ValueType>>(
          Thyra::unspecifiedPrec(
              Teuchos::rcp_static_cast<const Thyra::LinearOpBase<ValueType>>(
                  op)));

  return Preconditioner<ValueType>(precOp);
}

template <typename ValueType>
Preconditioner<ValueType> discreteBlockDiagonalPreconditioner(const std::vector<
    shared_ptr<const DiscreteBoundaryOperator<ValueType>>> &opVector) {
  size_t n = opVector.size();

  if (n == 0)
    throw std::runtime_error("discreteBlockDiagonalPreconditioner: "
                             "Input array must not be empty");

  Fiber::_2dArray<typename Preconditioner<
      ValueType>::DiscreteBoundaryOperatorPtr> opStructure(n, n);
  std::vector<size_t> rowCounts(n);
  std::vector<size_t> columnCounts(n);
  for (size_t i = 0; i < n; ++i) {
    opStructure(i, i) = opVector[i];
    rowCounts[i] = opVector[i]->rowCount();
    columnCounts[i] = opVector[i]->columnCount();
  }

  Teuchos::RCP<const Thyra::LinearOpBase<ValueType>> op(
      new DiscreteBlockedBoundaryOperator<ValueType>(opStructure, rowCounts,
                                                     columnCounts));

  typename Preconditioner<ValueType>::TeuchosPreconditionerPtr precOp =
      Teuchos::rcp_static_cast<const Thyra::PreconditionerBase<ValueType>>(
          Thyra::unspecifiedPrec(op));
  return Preconditioner<ValueType>(precOp);
}

#define INSTANTIATE_FREE_FUNCTIONS(VALUE)                                      \
  template Preconditioner<VALUE> discreteOperatorToPreconditioner(             \
      const shared_ptr<const DiscreteBoundaryOperator<VALUE>> &                \
          discreteOperator);                                                   \
  template Preconditioner<VALUE> discreteBlockDiagonalPreconditioner(          \
      const std::vector<shared_ptr<const DiscreteBoundaryOperator<VALUE>>> &   \
          opVector);

FIBER_INSTANTIATE_CLASS_TEMPLATED_ON_RESULT(Preconditioner);
FIBER_ITERATE_OVER_VALUE_TYPES(INSTANTIATE_FREE_FUNCTIONS);

} // namespace Bempp

#endif // WITH_TRILINOS
