#include <csDenseCellSet.hpp>
#include <csTestAssert.hpp>

#include <lsMakeGeometry.hpp>

int main() {
  using T = double;
  constexpr int D = 2;

  // two plane geometries
  lsBoundaryConditionEnum<D> boundaryCondition[D] = {
      lsBoundaryConditionEnum<D>::REFLECTIVE_BOUNDARY,
      lsBoundaryConditionEnum<D>::INFINITE_BOUNDARY};
  T bounds[2 * D] = {-1., 1., -1., 1.};

  T origin[D] = {0.};
  T normal[D] = {0.};
  normal[D - 1] = 1.;

  auto plane1 = lsSmartPointer<lsDomain<T, D>>::New(
      bounds, boundaryCondition, 0.2);
  lsMakeGeometry<T, D>(
      plane1, lsSmartPointer<lsPlane<T, D>>::New(origin, normal))
      .apply();

  origin[D - 1] = 1.;
  auto plane2 = lsSmartPointer<lsDomain<T, D>>::New(
      bounds, boundaryCondition, 0.2);
  lsMakeGeometry<T, D>(
      plane2, lsSmartPointer<lsPlane<T, D>>::New(origin, normal))
      .apply();

  auto levelSets =
      lsSmartPointer<std::vector<lsSmartPointer<lsDomain<T, D>>>>::New();
  levelSets->push_back(plane1);
  levelSets->push_back(plane2);

  csDenseCellSet<T, D> cellSet;
  int coverMaterial = 0;
  bool isAboveSurface = true;
  cellSet.setCellSetPosition(isAboveSurface);
  cellSet.setCoverMaterial(coverMaterial);
  cellSet.fromLevelSets(levelSets, nullptr, 3.);

  CSTEST_ASSERT(cellSet.getDepth() == 3.);
  CSTEST_ASSERT(cellSet.getNumberOfCells() == 160);
}