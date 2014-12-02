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

#include "abstract_boundary_operator_sum.hpp"

#include "context.hpp"
#include "discrete_boundary_operator_sum.hpp"

#include "../fiber/explicit_instantiation.hpp"

namespace Bempp {

template <typename BasisFunctionType_, typename ResultType_>
AbstractBoundaryOperatorSum<BasisFunctionType_, ResultType_>::
    AbstractBoundaryOperatorSum(
        const BoundaryOperator<BasisFunctionType, ResultType> &term1,
        const BoundaryOperator<BasisFunctionType, ResultType> &term2,
        int symmetry)
    : Base(term1.domain(), term1.range(), term1.dualToRange(),
           "(" + term1.label() + ") + (" + term2.label() + ")",
           symmetry & AUTO_SYMMETRY ? (term1.abstractOperator()->symmetry() &
                                       term2.abstractOperator()->symmetry())
                                    : symmetry),
      m_term1(term1), m_term2(term2) {
  assert(m_term1.abstractOperator());
  assert(m_term2.abstractOperator());

  if (m_term1.domain() != m_term2.domain())
    throw std::invalid_argument(
        "AbstractBoundaryOperatorSum::AbstractBoundaryOperatorSum(" +
        m_term1.label() + ", " + m_term2.label() +
        "): Domains of the two terms must be equal");
  if (m_term1.range() != m_term2.range())
    throw std::invalid_argument(
        "AbstractBoundaryOperatorSum::AbstractBoundaryOperatorSum(" +
        m_term1.label() + ", " + m_term2.label() +
        "): Ranges of the two terms must be equal");
  if (m_term1.dualToRange() != m_term2.dualToRange())
    throw std::invalid_argument(
        "AbstractBoundaryOperatorSum::AbstractBoundaryOperatorSum(" +
        m_term1.label() + ", " + m_term2.label() +
        "): Spaces dual to the ranges of the two terms must be equal");
}

template <typename BasisFunctionType, typename ResultType>
bool
AbstractBoundaryOperatorSum<BasisFunctionType, ResultType>::isLocal() const {
  return (m_term1.abstractOperator()->isLocal() &&
          m_term2.abstractOperator()->isLocal());
}

template <typename BasisFunctionType, typename ResultType>
BoundaryOperator<BasisFunctionType, ResultType>
AbstractBoundaryOperatorSum<BasisFunctionType, ResultType>::term1() const {
  return m_term1;
}

template <typename BasisFunctionType, typename ResultType>
BoundaryOperator<BasisFunctionType, ResultType>
AbstractBoundaryOperatorSum<BasisFunctionType, ResultType>::term2() const {
  return m_term2;
}

FIBER_INSTANTIATE_CLASS_TEMPLATED_ON_BASIS_AND_RESULT(
    AbstractBoundaryOperatorSum);

} // namespace Bempp
