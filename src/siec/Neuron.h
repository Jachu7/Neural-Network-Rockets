#ifndef CMAKESFMLPROJECT_NEURON_H
#define CMAKESFMLPROJECT_NEURON_H

#include <iostream>
#include <math.h>

class Neuron {
    public:
        Neuron(double value);
        void setValue(double value);
        void activate();
        void derive();


        //getters
        double getValue() {
            return this->value;
        }
        double getActiveValue() {
            return this->activatedValue;
        }
        double getDerivativeValue() {
            return this->derivedValue;
        }

    private:
        double value;
        double activatedValue;
        double derivedValue;
};


#endif //CMAKESFMLPROJECT_NEURON_H