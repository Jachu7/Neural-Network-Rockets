#include "Neuron.h"

// konstruktor
Neuron::Neuron(double value) {
    this->value = value;
    activate();
    derive();
}
// funkcja softsign
void Neuron::activate() {
    this->activatedValue = this->value / (1 + abs(this->value));
}
// pochodna sigmoid
void Neuron::derive() {
    this->derivedValue = this->activatedValue * (1 - this->activatedValue);
}


