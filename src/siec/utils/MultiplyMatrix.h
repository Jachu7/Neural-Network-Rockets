#ifndef CMAKESFMLPROJECT_MULTIPLYMATRIX_H
#define CMAKESFMLPROJECT_MULTIPLYMATRIX_H

#include <iostream>
#include <vector>
#include <assert.h>
#include "../Matrix.h"
namespace utils{
        class MultiplyMatrix{

        public:
        MultiplyMatrix(Matrix *a, Matrix *b);
        Matrix *execute();

        private:
        Matrix *a;
        Matrix *b;
        Matrix *c;
    };
}

#endif //CMAKESFMLPROJECT_MULTIPLYMATRIX_H