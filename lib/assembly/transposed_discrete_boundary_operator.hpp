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

#ifndef bempp_transposed_discrete_boundary_operator_hpp
#define bempp_transposed_discrete_boundary_operator_hpp

#include "../common/common.hpp"

#include "discrete_boundary_operator.hpp"

#include "../common/shared_ptr.hpp"

#ifdef WITH_TRILINOS
#include <Teuchos_RCP.hpp>
#endif

namespace Bempp {

/** \ingroup composite_discrete_boundary_operators
 *  \brief Transposed and/or conjugated discrete boundary operator.
 *
 *  This class represents a transposed, conjugated or conjugate-transposed
 *  discrete boundary operator scalar. */
template <typename ValueType>
class TransposedDiscreteBoundaryOperator
    : public DiscreteBoundaryOperator<ValueType> {
public:
  typedef DiscreteBoundaryOperator<ValueType> Base;

  /** \brief Constructor.
   *
   *  Construct the discrete boundary operator representing the transpose,
   *  conjugate or conjugate transpose of the operator \p op. */
  TransposedDiscreteBoundaryOperator(TranspositionMode trans,
                                     const shared_ptr<const Base> &op);

  virtual arma::Mat<ValueType> asMatrix() const;

  virtual unsigned int rowCount() const;
  virtual unsigned int columnCount() const;

  virtual void addBlock(const std::vector<int> &rows,
                        const std::vector<int> &cols, const ValueType alpha,
                        arma::Mat<ValueType> &block) const;

#ifdef WITH_TRILINOS
public:
  virtual Teuchos::RCP<const Thyra::VectorSpaceBase<ValueType>> domain() const;
  virtual Teuchos::RCP<const Thyra::VectorSpaceBase<ValueType>> range() const;

protected:
  virtual bool opSupportedImpl(Thyra::EOpTransp M_trans) const;
#endif

private:
  bool isTransposed() const;

  virtual void applyBuiltInImpl(const TranspositionMode trans,
                                const arma::Col<ValueType> &x_in,
                                arma::Col<ValueType> &y_inout,
                                const ValueType alpha,
                                const ValueType beta) const;

private:
  TranspositionMode m_trans;
  shared_ptr<const Base> m_operator;
};

} // namespace Bempp

#endif
