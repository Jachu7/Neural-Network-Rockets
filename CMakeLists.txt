cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(main src/main.cpp
        src/siec/Neuron.cpp
        src/siec/Neuron.h
        src/siec/Layer.h
        src/siec/Layer.cpp
        src/siec/Matrix.h
        src/siec/Matrix.cpp
        src/siec/NeuralNetwork.h
        src/siec/NeuralNetwork.cpp
        src/siec/utils/MatrixToVector.h
        src/siec/utils/MultiplyMatrix.h
        src/siec/utils/MultiplyMatrix.cpp
        src/siec/utils/MatrixToVector.cpp)
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics)
