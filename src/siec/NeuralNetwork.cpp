#include "NeuralNetwork.h"
#include "utils/MultiplyMatrix.h"

void NeuralNetwork::setErrors() {
    if(this->target.size() == 0) {
        std::cerr << "No target for this neural network" << std::endl;
        assert(false);
    }

    if(this->target.size() != this->layers.at(this->layers.size() - 1)->getNeurons().size()) {
        std::cerr << "Target size is not the same as output layer size: " << this->layers.at(this->layers.size() - 1)->getNeurons().size() << std::endl;
        assert(false);
    }

    this->error = 0.00;
    int outputLayerIndex = this->layers.size() - 1;
    std::vector<Neuron *> outputNeurons = this->layers.at(outputLayerIndex)->getNeurons();
    for(int i = 0; i < target.size(); i++) {
        double tempErr = (outputNeurons.at(i)->getActiveValue() - target.at(i));
        errors.at(i) = tempErr;
        this->error += tempErr;
    }

    historicalErrors.push_back(this->error);
}



void NeuralNetwork::feedForward() {
    for (int i = 0; i < (this->layers.size() - 1); i++) {
        Matrix *a = this->getNeuronMatrix(i);

        if (i != 0) {
            a = this-> getActivatedNeuronMatrix(i);
        }

        Matrix *b = this->getWeightMatrix(i);
        Matrix *c = (new utils::MultiplyMatrix(a,b))->execute();

        for (int c_index = 0; c_index < c->getNumCols(); c_index++) {
            this->setNeuronValue(i+1, c_index, c->getValue(0, c_index));
        }
    }
}


void NeuralNetwork::printToConsole() {
    for (int i = 0; i < this->layers.size(); i++) {
        std::cout << "Layer " << i << ":\n";
        if (i == 0) {
            Matrix *m = this->layers.at(i)->matrixifyVals();
            m->printToConsole();
        }
        else {
            Matrix *m = this->layers.at(i)->matrixifyActivatedVals();
            m->printToConsole();
        }
        std::cout << "==========="<<std::endl;
        if (i < this->layers.size() - 1) {
            std::cout << "Weight Matrix: "<< i << std::endl;
            this->getWeightMatrix(i)->printToConsole();
        }
        std::cout << "==========="<<std::endl;
    }
}

void NeuralNetwork::setCurrentInput(std::vector<double> input) {
    this-> input = input;
    for (int i = 0; i < input.size(); i++) {
        this-> layers.at(0)->setValue(i, input.at(i));
    }
}

NeuralNetwork::NeuralNetwork(std::vector<int> topology) {
    this->topology = topology;
    this->topologySize = topology.size();
    for (int i = 0; i < topology.size(); i++) {
        Layer *l = new Layer(topology.at(i));
        this->layers.push_back(l);
    }
    for (int i = 0; i < (topology.size() - 1); i++) {
        Matrix *m = new Matrix(topology.at(i), topology.at(i+1), true);
        this->weightMatrices.push_back(m);
    }
}