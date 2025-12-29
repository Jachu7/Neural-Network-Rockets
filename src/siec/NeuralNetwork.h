#ifndef CMAKESFMLPROJECT_NEURAL_NETWORK_H
#define CMAKESFMLPROJECT_NEURAL_NETWORK_H

#include <iostream>
#include <vector>
#include "Matrix.h"
#include "Layer.h"

class NeuralNetwork {
public:
    NeuralNetwork(std::vector<int> topology);
    void setCurrentInput(std::vector<double> input);
    void setCurrentTarget(std::vector<double> target) { this->target = target; };
    void printToConsole();
    void feedForward();
    void setErrors();

    Matrix *getNeuronMatrix(int index){return this->layers.at(index)->matrixifyVals();};
    Matrix *getActivatedNeuronMatrix(int index){return this->layers.at(index)->matrixifyActivatedVals();};
    Matrix *getDerivedNeuronMatrix(int index){return this->layers.at(index)->matrixifyDerivedVals();};
    Matrix *getWeightMatrix(int index){return this->weightMatrices.at(index);};

    void setNeuronValue(int indexLayer, int indexNeuron, double val){this->layers.at(indexLayer)->setValue(indexNeuron, val);};
    double getTotalError(){return this->error;};
    std::vector<double> getErrors(){return this->errors;};

private:
    int topologySize;
    std::vector<int> topology;
    std::vector<Layer *> layers;
    std::vector<Matrix *> weightMatrices;
    std::vector<Matrix *> gradientMatrices;
    std::vector<double> input;
    std::vector<double> target;
    double error;
    std::vector<double> errors;
    std::vector<double> historicalErrors;
};


#endif //CMAKESFMLPROJECT_NEURAL_NETWORK_H