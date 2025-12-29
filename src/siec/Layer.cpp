#include "Layer.h"

Matrix *Layer::matrixifyVals() {
    Matrix *m = new Matrix(1, this->neurons.size(), false);
    for (int i = 0; i < this->neurons.size(); i++) {
        m->setValue(
            0,
            i,
            this->neurons.at(i)->getValue());
    }
    return m;
}

Matrix *Layer::matrixifyActivatedVals() {
    Matrix *m = new Matrix(1, this->neurons.size(), false);
    for (int i = 0; i < this->neurons.size(); i++) {
        m->setValue(
            0,
            i,
            this->neurons.at(i)->getActiveValue());
    }
    return m;
}

Matrix *Layer::matrixifyDerivedVals() {
    Matrix *m = new Matrix(1, this->neurons.size(), false);
    for (int i = 0; i < this->neurons.size(); i++) {
        m->setValue(
            0,
            i,
            this->neurons.at(i)->getDerivativeValue());
    }
    return m;
}

Layer::Layer(int size) {
    this->size = size;
    for (int i = 0; i <size; i++) {
        Neuron *n = new Neuron(0.00);
        this->neurons.push_back(n);
    }
}

void Layer::setValue(int i, double v) {
    this->neurons.at(i)->setValue(v);
}
std::vector<double> Layer::getActivatedVals() {
    std::vector<double> ret;

    for(int i = 0; i < this->neurons.size(); i++) {
        double v = this->neurons.at(i)->getActiveValue();

        ret.push_back(v);
    }

    return ret;
}

