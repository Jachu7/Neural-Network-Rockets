#ifndef CMAKESFMLPROJECT_MATRIX_H
#define CMAKESFMLPROJECT_MATRIX_H


#include <iostream>
#include <vector>

class Matrix {
public:
    Matrix(int numRows, int numCols, bool isRandom);
    Matrix *transpose();
    double generateRandomNumber();

    void setValue(int r, int c, double v) {
        this->values.at(r).at(c) = v;
    };
    double getValue(int r, int c) {
        return this->values.at(r).at(c);
    };

    void printToConsole();

    int getNumRows() {return this->numRows;}
    int getNumCols() {return this->numCols;}

private:
    int numRows;
    int numCols;
    bool isRandom;
    std::vector<std::vector<double>> values;
};


#endif //CMAKESFMLPROJECT_MATRIX_H