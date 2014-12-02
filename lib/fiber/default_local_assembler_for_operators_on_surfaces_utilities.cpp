#include "default_local_assembler_for_operators_on_surfaces_utilities.hpp"

#include "shapeset.hpp"
#include "explicit_instantiation.hpp"
#include "raw_grid_geometry.hpp"

namespace Fiber {

template <typename BasisFunctionType>
void DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<BasisFunctionType>::
    checkConsistencyOfGeometryAndShapesets(
        const RawGridGeometry<CoordinateType> &rawGeometry,
        const std::vector<const Shapeset<BasisFunctionType> *> &shapesets) {
  if (rawGeometry.vertices().n_rows != 3)
    throw std::invalid_argument(
        "DefaultLocalAssemblerForOperatorsOnSurfacesUtilities::"
        "checkConsistencyOfGeometryAndShapesets(): "
        "vertex coordinates must be three-dimensional");
  const size_t elementCount = rawGeometry.elementCornerIndices().n_cols;
  if (rawGeometry.elementCornerIndices().n_rows < 3 ||
      4 < rawGeometry.elementCornerIndices().n_rows)
    throw std::invalid_argument(
        "DefaultLocalAssemblerForOperatorsOnSurfacesUtilities::"
        "checkConsistencyOfGeometryAndShapesets(): "
        "Elements must have either 3 or 4 corners");
  if (!rawGeometry.auxData().is_empty() &&
      rawGeometry.auxData().n_cols != elementCount)
    throw std::invalid_argument(
        "DefaultLocalAssemblerForOperatorsOnSurfacesUtilities::"
        "checkConsistencyOfGeometryAndShapesets(): "
        "number of columns of auxData must match that of "
        "elementCornerIndices");
  if (shapesets.size() != elementCount)
    throw std::invalid_argument(
        "DefaultLocalAssemblerForOperatorsOnSurfacesUtilities::"
        "checkConsistencyOfGeometryAndShapesets(): "
        "size of shapesets must match the number of columns of "
        "elementCornerIndices");
}

template <typename BasisFunctionType>
void DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<BasisFunctionType>::
    precalculateElementSizesAndCentersForSingleGrid(
        const RawGridGeometry<CoordinateType> &rawGeometry,
        std::vector<CoordinateType> &elementSizesSquared,
        arma::Mat<CoordinateType> &elementCenters,
        CoordinateType &averageElementSize) {
  const size_t elementCount = rawGeometry.elementCount();
  const int worldDim = rawGeometry.worldDimension();

  averageElementSize = 0.; // We will store here temporarily
                           // the sum of element sizes
  elementSizesSquared.resize(elementCount);
  for (int e = 0; e < elementCount; ++e) {
    elementSizesSquared[e] = elementSizeSquared(e, rawGeometry);
    averageElementSize += sqrt(elementSizesSquared[e]);
  }
  averageElementSize /= elementCount;

  elementCenters.set_size(worldDim, elementCount);
  for (int e = 0; e < elementCount; ++e)
    elementCenters.col(e) = elementCenter(e, rawGeometry);
}

template <typename BasisFunctionType>
inline typename DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<
    BasisFunctionType>::CoordinateType
DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<BasisFunctionType>::
    elementSizeSquared(int elementIndex,
                       const RawGridGeometry<CoordinateType> &rawGeometry) {
  // This implementation could be optimised
  CoordinateType maxEdgeLengthSquared = 0.;
  const arma::Mat<int> &cornerIndices = rawGeometry.elementCornerIndices();
  const arma::Mat<CoordinateType> &vertices = rawGeometry.vertices();
  arma::Col<CoordinateType> edge;
  if (cornerIndices(cornerIndices.n_rows - 1, elementIndex) == -1) {
    // Triangular element
    const int cornerCount = 3;
    for (int i = 0; i < cornerCount; ++i) {
      edge = vertices.col(cornerIndices((i + 1) % cornerCount, elementIndex)) -
             vertices.col(cornerIndices(i, elementIndex));
      CoordinateType edgeLengthSquared = arma::dot(edge, edge);
      maxEdgeLengthSquared = std::max(maxEdgeLengthSquared, edgeLengthSquared);
    }
  } else {
    // Quadrilateral element. We assume it is convex.
    edge = vertices.col(cornerIndices(2, elementIndex)) -
           vertices.col(cornerIndices(0, elementIndex));
    maxEdgeLengthSquared = arma::dot(edge, edge);
    edge = vertices.col(cornerIndices(3, elementIndex)) -
           vertices.col(cornerIndices(1, elementIndex));
    CoordinateType edgeLengthSquared = arma::dot(edge, edge);
    maxEdgeLengthSquared = std::max(maxEdgeLengthSquared, edgeLengthSquared);
  }
  return maxEdgeLengthSquared;
}

template <typename BasisFunctionType>
inline arma::Col<typename DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<
    BasisFunctionType>::CoordinateType>
DefaultLocalAssemblerForOperatorsOnSurfacesUtilities<
    BasisFunctionType>::elementCenter(int elementIndex,
                                      const RawGridGeometry<CoordinateType> &
                                          rawGeometry) {
  const arma::Mat<int> &cornerIndices = rawGeometry.elementCornerIndices();
  const arma::Mat<CoordinateType> &vertices = rawGeometry.vertices();
  const int maxCornerCount = cornerIndices.n_rows;
  // each element has at least one corner
  arma::Col<CoordinateType> center(
      vertices.col(cornerIndices(0, elementIndex)));
  int i = 1;
  for (; i < maxCornerCount; ++i) {
    int cornerIndex = cornerIndices(i, elementIndex);
    if (cornerIndex == -1)
      break;
    center += vertices.col(cornerIndex);
  }
  // now i contains the number of corners of the specified element
  center /= i;
  return center;
}

FIBER_INSTANTIATE_CLASS_TEMPLATED_ON_BASIS(
    DefaultLocalAssemblerForOperatorsOnSurfacesUtilities);

} // namespace Fiber
