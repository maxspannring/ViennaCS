// apply the model to the geometry
#pragma once

#include <csDenseCellSet.hpp>
#include <csImplantModel.hpp>

#include <vcLogger.hpp>
#include <random>
#include <cmath>

namespace viennacs {

using namespace viennacore;

template <class NumericType, int D> class Implant {
  SmartPointer<DenseCellSet<NumericType, D>> cellSet_;
  SmartPointer<ImplantModel<NumericType, D>> model_;
  util::Parameters params_;
  std::vector<int> maskMaterials;

public:
  Implant() = default;

  void setCellSet(SmartPointer<DenseCellSet<NumericType, D>> passedCellSet) {
    cellSet_ = passedCellSet;
  }

  void setImplantModel(
      SmartPointer<ImplantModel<NumericType, D>> passedImplantModel) {
    model_ = passedImplantModel;
  }

  void setParameters(util::Parameters passedParameters){
      params_ = passedParameters;
  }

  template <class... Mats> void setMaskMaterials(Mats... mats) {
    maskMaterials = {mats...};
  }

void apply(util::Parameters params){
  if (!model_){
      Logger::getInstance().addWarning("No implant model passed to Implant.").print();
      return;
  }
  if (!cellSet_) {
      Logger::getInstance().addWarning("No cellSet passed to Implant.").print();
      return;
  }

  // now we apply the implant model to the cell set
  auto boundingBox = cellSet_->getBoundingBox();
  auto gridDelta = cellSet_->getGridDelta();
  auto concentration = cellSet_->getScalarData("concentration");
  auto material = cellSet_->getScalarData("Material");
  NumericType xLength = std::abs(boundingBox[1][0] - boundingBox[0][0]);
  NumericType yLength = std::abs(boundingBox[1][1] - boundingBox[0][1]);
  std::cout << "xLength: " << xLength << std::endl;
  std::cout << "yLength: " << yLength << std::endl;
  int numberOfcellsXdirection = xLength / gridDelta;
  int numberOfcellsYdirection = yLength / gridDelta;
  std::cout << "X-cells: " << numberOfcellsXdirection << std::endl;
  std::cout << "Y-cells: " << numberOfcellsYdirection << std::endl;
  double angle = params.get("angle");
  double radians = angle * M_PI / 180; // pls always use fucking radians

  // ToDo: it seems like the algorithm doesn't quite iterate over *all* cells
  // ToDo: it also tries to acces some cells that don't even exist
  // iterate over all the beams that hit the x-plane from the y direction:
  //toDo: take care of 'shadows'
  // toDo: create rough surface
  for (int i = 0; i < numberOfcellsXdirection; i++){
      NumericType initialX = i*gridDelta - xLength / 2 + gridDelta;
      NumericType initialY = yLength - gridDelta;
      std::array<NumericType, 3> initialCoords{initialX, initialY, 0};
      int initialIndex;
      numberOfcellsYdirection = yLength / gridDelta;
      do{
          initialCoords[0] = initialX;
          initialCoords[1] = initialY;
          initialIndex = cellSet_->getIndex(initialCoords);
          if(initialIndex == -1){
              initialY -= gridDelta * std::cos(radians);
              initialX -= gridDelta * std::sin(radians);
              numberOfcellsYdirection -= 1;
              if (std::abs(initialCoords[1]) > yLength){
                  break;
              }
          } else {
              if((*material)[initialIndex] == 0.){
                  break;
              } else {
                  // iterate over all the cells in y direction (depth), and then
                  // iterate over all the cells in x direction to get the lateral displacement
                  for (int j = 0;j < numberOfcellsYdirection + 2; j++){
                      for (int k = 0;k < numberOfcellsXdirection + 1; k++){
                          NumericType yCord = j * gridDelta;
                          NumericType xCord = k * gridDelta;
                          NumericType shifted_xCord = xCord - xLength / 2;
                          NumericType shifted_yCord = initialY - yCord;
                          //std::cout << "coord: [" << shifted_xCord << ", " << shifted_yCord << "]" <<std::endl;
                          NumericType depth = std::cos(radians) * yCord + std::sin(radians) * (initialX - shifted_xCord);
                          NumericType lateralDisplacement = std::abs(std::cos(radians)*(initialX - shifted_xCord) - std::sin(radians) * yCord);

                          std::array<NumericType, 3> coords{shifted_xCord, shifted_yCord, 0};
                          auto index = cellSet_->getIndex(coords);
                          if (index != -1){
                              (*concentration)[index] += model_->getDepthProfile(depth, params_) * model_->getLateralProfile(lateralDisplacement, depth, params_);
                          } else {
                              std::cout << "index miss @ [" << coords[0] << ", " << coords[1] << "] " << std::endl;
                          }
                      }
                  }
              }
          }
      } while (initialIndex == -1);
  }
  }
};

} // namespace viennacs