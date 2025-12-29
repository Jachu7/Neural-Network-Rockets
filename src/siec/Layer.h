#ifndef CMAKESFMLPROJECT_LAYER_H
#define CMAKESFMLPROJECT_LAYER_H

#include <iostream>
#include <vector>
#include "Neuron.h"
#include "Matrix.h"

class Layer {
public:
    Layer(int size);
    void setValue(int i, double v);

    Matrix *matrixifyVals();
    Matrix *matrixifyActivatedVals();
    Matrix *matrixifyDerivedVals();

    std::vector<double> getActivatedVals();

    std::vector<Neuron *> getNeurons() { return this->neurons; };
    void setNeuron(std::vector<Neuron *> neurons) { this->neurons = neurons; }

private:
    int size;
    std::vector<Neuron *> neurons;
};


#endif //CMAKESFMLPROJECT_LAYER_H