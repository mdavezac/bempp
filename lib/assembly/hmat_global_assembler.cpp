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

#include "bempp/common/config_ahmed.hpp"
#include "bempp/common/config_trilinos.hpp"

#include "hmat_global_assembler.hpp"

#include "assembly_options.hpp"
#include "context.hpp"
#include "evaluation_options.hpp"
#include "discrete_boundary_operator_composition.hpp"
#include "discrete_sparse_boundary_operator.hpp"
#include "weak_form_hmat_assembly_helper.hpp"
#include "discrete_hmat_boundary_operator.hpp"

#include "../common/armadillo_fwd.hpp"
#include "../common/auto_timer.hpp"
#include "../common/chunk_statistics.hpp"
#include "../common/to_string.hpp"
#include "../fiber/explicit_instantiation.hpp"
#include "../fiber/local_assembler_for_integral_operators.hpp"
#include "../fiber/scalar_traits.hpp"
#include "../fiber/shared_ptr.hpp"
#include "../space/space.hpp"
#include "../common/bounding_box.hpp"

#include "../hmat/block_cluster_tree.hpp"
#include "../hmat/geometry_interface.hpp"
#include "../hmat/geometry_data_type.hpp"
#include "../hmat/geometry.hpp"
#include "../hmat/hmatrix.hpp"
#include "../hmat/data_accessor.hpp"
#include "../hmat/hmatrix_dense_compressor.hpp"
#include "../hmat/hmatrix_aca_compressor.hpp"

#include <stdexcept>
#include <fstream>
#include <iostream>

#include <boost/type_traits/is_complex.hpp>

#include <tbb/atomic.h>
#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/concurrent_queue.h>

#include <Teuchos_ParameterList.hpp>

namespace Bempp {

namespace {

template <typename BasisFunctionType>
class SpaceHMatGeometryInterface : public hmat::GeometryInterface {

public:
  typedef typename Fiber::ScalarTraits<BasisFunctionType>::RealType
  CoordinateType;
  SpaceHMatGeometryInterface(const Space<BasisFunctionType> &space)
      : m_counter(0) {
    space.getGlobalDofBoundingBoxes(m_bemppBoundingBoxes);
  }
  shared_ptr<const hmat::GeometryDataType> next() override {

    if (m_counter == m_bemppBoundingBoxes.size())
      return shared_ptr<hmat::GeometryDataType>();

    auto lbound = m_bemppBoundingBoxes[m_counter].lbound;
    auto ubound = m_bemppBoundingBoxes[m_counter].ubound;
    auto center = m_bemppBoundingBoxes[m_counter].reference;
    m_counter++;
    return shared_ptr<hmat::GeometryDataType>(new hmat::GeometryDataType(
        hmat::BoundingBox(lbound.x, ubound.x, lbound.y, ubound.y, lbound.z,
                          ubound.z),
        std::array<double, 3>({{center.x, center.y, center.z}})));
  }

  std::size_t numberOfEntities() const override {
    return m_bemppBoundingBoxes.size();
  }
  void reset() override { m_counter = 0; }

private:
  std::size_t m_counter;
  std::vector<BoundingBox<CoordinateType>> m_bemppBoundingBoxes;
};

template <typename BasisFunctionType>
shared_ptr<hmat::DefaultBlockClusterTreeType>
generateBlockClusterTree(const Space<BasisFunctionType> &testSpace,
                         const Space<BasisFunctionType> &trialSpace,
                         int minBlockSize, int maxBlockSize, double eta) {

  hmat::Geometry testGeometry;
  hmat::Geometry trialGeometry;

  auto testSpaceGeometryInterface = shared_ptr<hmat::GeometryInterface>(
      new SpaceHMatGeometryInterface<BasisFunctionType>(testSpace));

  auto trialSpaceGeometryInterface = shared_ptr<hmat::GeometryInterface>(
      new SpaceHMatGeometryInterface<BasisFunctionType>(trialSpace));

  hmat::fillGeometry(testGeometry, *testSpaceGeometryInterface);
  hmat::fillGeometry(trialGeometry, *trialSpaceGeometryInterface);

  auto testClusterTree = shared_ptr<hmat::DefaultClusterTreeType>(
      new hmat::DefaultClusterTreeType(testGeometry, minBlockSize));

  auto trialClusterTree = shared_ptr<hmat::DefaultClusterTreeType>(
      new hmat::DefaultClusterTreeType(trialGeometry, minBlockSize));

  shared_ptr<hmat::DefaultBlockClusterTreeType> blockClusterTree(
      new hmat::DefaultBlockClusterTreeType(testClusterTree, trialClusterTree,
                                            maxBlockSize,
                                            hmat::StandardAdmissibility(eta)));

  return blockClusterTree;
}
} // end anonymous namespace
template <typename BasisFunctionType, typename ResultType>
std::unique_ptr<DiscreteBoundaryOperator<ResultType>>
HMatGlobalAssembler<BasisFunctionType, ResultType>::assembleDetachedWeakForm(
    const Space<BasisFunctionType> &testSpace,
    const Space<BasisFunctionType> &trialSpace,
    const std::vector<LocalAssemblerForIntegralOperators *> &localAssemblers,
    const std::vector<LocalAssemblerForIntegralOperators *> &
        localAssemblersForAdmissibleBlocks,
    const std::vector<const DiscreteBndOp *> &sparseTermsToAdd,
    const std::vector<ResultType> &denseTermMultipliers,
    const std::vector<ResultType> &sparseTermMultipliers,
    const Context<BasisFunctionType, ResultType> &context, int symmetry) {

  const AssemblyOptions &options = context.assemblyOptions();
  const auto hMatParameterList =
      context.globalParameterList().sublist("HMatParameters");
  const bool indexWithGlobalDofs =
      (hMatParameterList.template get<std::string>("HMatAssemblyMode") ==
       "GlobalAssembly");
  const bool verbosityAtLeastDefault =
      (options.verbosityLevel() >= VerbosityLevel::DEFAULT);
  const bool verbosityAtLeastHigh =
      (options.verbosityLevel() >= VerbosityLevel::HIGH);

  auto testSpacePointer = Fiber::make_shared_from_const_ref(testSpace);
  auto trialSpacePointer = Fiber::make_shared_from_const_ref(trialSpace);

  shared_ptr<const Space<BasisFunctionType>> actualTestSpace;
  shared_ptr<const Space<BasisFunctionType>> actualTrialSpace;
  if (indexWithGlobalDofs) {
    actualTestSpace = testSpacePointer->discontinuousSpace(testSpacePointer);
    actualTrialSpace = trialSpacePointer->discontinuousSpace(trialSpacePointer);
  } else {
    actualTestSpace = testSpacePointer;
    actualTrialSpace = trialSpacePointer;
  }

  auto minBlockSize =
      hMatParameterList.template get<unsigned int>("minBlockSize");
  auto maxBlockSize =
      hMatParameterList.template get<unsigned int>("maxBlockSize");
  auto eta = hMatParameterList.template get<double>("eta");

  auto blockClusterTree = generateBlockClusterTree(
      *actualTestSpace, *actualTrialSpace, minBlockSize, maxBlockSize, eta);

  // blockClusterTree->writeToPdfFile("tree.pdf", 1024, 1024);

  WeakFormHMatAssemblyHelper<BasisFunctionType, ResultType> helper(
      *actualTestSpace, *actualTrialSpace, blockClusterTree, localAssemblers,
      sparseTermsToAdd, denseTermMultipliers, sparseTermMultipliers);

  // hmat::HMatrixDenseCompressor<ResultType, 2> compressor(helper);
  // shared_ptr<hmat::CompressedMatrix<ResultType>> hMatrix(
  //    new hmat::DefaultHMatrixType<ResultType>(blockClusterTree, compressor));

  hmat::HMatrixAcaCompressor<ResultType, 2> compressor(helper, 1E-3, 30);
  shared_ptr<hmat::CompressedMatrix<ResultType>> hMatrix(
      new hmat::DefaultHMatrixType<ResultType>(blockClusterTree, compressor));

  return std::unique_ptr<DiscreteBoundaryOperator<ResultType>>(
      new DiscreteHMatBoundaryOperator<ResultType>(hMatrix));

  // return std::unique_ptr<DiscreteBoundaryOperator<ResultType>>();
}

template <typename BasisFunctionType, typename ResultType>
std::unique_ptr<DiscreteBoundaryOperator<ResultType>>
HMatGlobalAssembler<BasisFunctionType, ResultType>::assembleDetachedWeakForm(
    const Space<BasisFunctionType> &testSpace,
    const Space<BasisFunctionType> &trialSpace,
    LocalAssemblerForIntegralOperators &localAssembler,
    LocalAssemblerForIntegralOperators &localAssemblerForAdmissibleBlocks,
    const Context<BasisFunctionType, ResultType> &context, int symmetry) {
  typedef LocalAssemblerForIntegralOperators Assembler;
  std::vector<Assembler *> localAssemblers(1, &localAssembler);
  std::vector<Assembler *> localAssemblersForAdmissibleBlocks(
      1, &localAssemblerForAdmissibleBlocks);
  std::vector<const DiscreteBndOp *> sparseTermsToAdd;
  std::vector<ResultType> denseTermsMultipliers(1, 1.0);
  std::vector<ResultType> sparseTermsMultipliers;

  return assembleDetachedWeakForm(testSpace, trialSpace, localAssemblers,
                                  localAssemblersForAdmissibleBlocks,
                                  sparseTermsToAdd, denseTermsMultipliers,
                                  sparseTermsMultipliers, context, symmetry);
}

FIBER_INSTANTIATE_CLASS_TEMPLATED_ON_BASIS_AND_RESULT(HMatGlobalAssembler);

} // namespace Bempp
