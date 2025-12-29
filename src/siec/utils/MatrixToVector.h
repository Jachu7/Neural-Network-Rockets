#ifndef CMAKESFMLPROJECT_MATRIXTOVECTOR_H
#define CMAKESFMLPROJECT_MATRIXTOVECTOR_H

#include <iostream>
#include <vector>
#include <assert.h>
#include "../Matrix.h"

namespace utils {
    class MatrixToVector {
    public:
    MatrixToVector(Matrix *a);

    std::vector<double> execute();

    private:
        Matrix *a;
    };
}

#endif //CMAKESFMLPROJECT_MATRIXTOVECTOR_H