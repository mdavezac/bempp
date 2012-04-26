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

#ifndef bempp_weak_form_aca_assembly_helper_hpp
#define bempp_weak_form_aca_assembly_helper_hpp

#include "../common/types.hpp"
#include "../fiber/scalar_traits.hpp"
#include "ahmed_aux_fwd.hpp"

#include <armadillo>
#include <vector>

namespace Fiber
{

template <typename ResultType> class LocalAssemblerForOperators;

} // namespace Fiber

namespace Bempp
{

class AssemblyOptions;
template <typename ResultType> class DiscreteLinearOperator;
template <typename ValueType> class Space;

/** \brief Class whose methods are called by Ahmed during assembly in the ACA mode. */
template <typename BasisFunctionType, typename ResultType>
class WeakFormAcaAssemblyHelper
{
public:
    typedef DiscreteLinearOperator<ResultType> DiscreteLinOp;
    typedef Fiber::LocalAssemblerForOperators<ResultType> LocalAssembler;
    typedef typename Fiber::ScalarTraits<ResultType>::RealType MagnitudeType;
    typedef typename AhmedTypeTraits<ResultType>::Type AhmedResultType;

    WeakFormAcaAssemblyHelper(const Space<BasisFunctionType>& testSpace,
                              const Space<BasisFunctionType>& trialSpace,
                              const std::vector<unsigned int>& p2oTestDofs,
                              const std::vector<unsigned int>& p2oTrialDofs,
                              const std::vector<LocalAssembler*>& assemblers,
                              const std::vector<const DiscreteLinOp*>& sparseTermsToAdd,
                              const std::vector<ResultType>& denseTermsMultipliers,
                              const std::vector<ResultType>& sparseTermsMultipliers,
                              const AssemblyOptions& options);

    /** Store the entries of the block defined
        by b1, n1, b2, n2 (in permuted ordering) in data */
    void cmpbl(unsigned b1, unsigned n1, unsigned b2, unsigned n2,
               AhmedResultType* data) const;

    /** Expected size of the entries in this block. */
    MagnitudeType scale(unsigned b1, unsigned n1, unsigned b2, unsigned n2) const;

private:
    /** Find the elements and local DOF indices that correspond
        to the global DOF indices stored in the entries [start, start + length)
        of array p2o. */
    void findLocalDofs(int start,
                       int globalDofCount,
                       const std::vector<unsigned int>& p2o,
                       const Space<BasisFunctionType>& space,
                       std::vector<GlobalDofIndex>& globalDofIndices,
                       std::vector<int>& elementIndices,
                       std::vector<std::vector<LocalDofIndex> >& localDofIndices,
                       std::vector<std::vector<int> >& arrayIndices) const;

private:
    const Space<BasisFunctionType>& m_testSpace;
    const Space<BasisFunctionType>& m_trialSpace;
    const std::vector<unsigned int>& m_p2oTestDofs;
    const std::vector<unsigned int>& m_p2oTrialDofs;
    const std::vector<LocalAssembler*>& m_assemblers;
    const std::vector<const DiscreteLinOp*>& m_sparseTermsToAdd;
    const std::vector<ResultType>& m_denseTermsMultipliers;
    const std::vector<ResultType>& m_sparseTermsMultipliers;
    const AssemblyOptions& m_options;
};

} // namespace Bempp

#endif
