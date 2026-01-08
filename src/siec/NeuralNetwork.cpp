#include "NeuralNetwork.h"
#include "utils/MultiplyMatrix.h"

// Destruktor. Odpowiada za zwolnienie pamięci (usuwa wskaźniki na warstwy i macierze wag), aby zapobiec wyciekom pamięci
NeuralNetwork::~NeuralNetwork()
{
    for (auto l : layers)
        delete l;
    for (auto m : weightMatrices)
        delete m;
}

// Realizuje przepływ sygnału przez sieć (od wejścia do wyjścia). Dla każdej warstwy:
// 1. Pobiera wartości neuronów jako macierz.
// 2. Mnoży je przez macierz wag (używając MultiplyMatrix).
// 3. Ustawia wynik jako wartości neuronów w kolejnej warstwie. Zarządza również dynamiczną alokacją pamięci dla tymczasowych macierzy.
void NeuralNetwork::feedForward()
{
    for (int i = 0; i < (this->layers.size() - 1); i++)
    {
        Matrix *a = this->getNeuronMatrix(i);

        if (i != 0)
        {
            Matrix *temp = a;
            a = this->getActivatedNeuronMatrix(i);
            delete temp;
        }

        Matrix *b = this->getWeightMatrix(i);
        Matrix *c = (new utils::MultiplyMatrix(a, b))->execute();

        // Sprzątanie po macierzy a (jeśli była alokowana dynamicznie w pętli)
        delete a;

        for (int c_index = 0; c_index < c->getNumCols(); c_index++)
        {
            this->setNeuronValue(i + 1, c_index, c->getValue(0, c_index));
        }
        delete c; // Sprzątamy wynik mnożenia
    }
}

// Wstawia dane wejściowe (np raycasty) do neuronów pierwszej warstwy (warstwy wejściowej)
void NeuralNetwork::setCurrentInput(std::vector<double> input)
{
    this->input = input;
    for (int i = 0; i < input.size(); i++)
    {
        this->layers.at(0)->setValue(i, input.at(i));
    }
}

// Konstruktor sieci. Na podstawie wektora topology (np. {3, 5, 2} oznacza 3 wejścia, 5 ukrytych, 2 wyjścia) tworzy odpowiednie warstwy oraz macierze wag pomiędzy nimi.
NeuralNetwork::NeuralNetwork(std::vector<int> topology)
{
    this->topology = topology;
    this->topologySize = topology.size();
    for (int i = 0; i < topology.size(); i++)
    {
        Layer *l = new Layer(topology.at(i));
        this->layers.push_back(l);
    }
    for (int i = 0; i < (topology.size() - 1); i++)
    {
        Matrix *m = new Matrix(topology.at(i), topology.at(i + 1), true);
        this->weightMatrices.push_back(m);
    }
}

// --- IMPLEMENTACJA GA ---

// (Dla Algorytmu Genetycznego) Pobiera wszystkie wagi ze wszystkich macierzy i spłaszcza je do jednego długiego wektora. Służy do stworzenia "genotypu" sieci.
std::vector<double> NeuralNetwork::getWeights() const
{
    std::vector<double> weights;
    for (Matrix *m : weightMatrices)
    {
        for (int r = 0; r < m->getNumRows(); r++)
        {
            for (int c = 0; c < m->getNumCols(); c++)
            {
                weights.push_back(m->getValue(r, c));
            }
        }
    }
    return weights;
}

// (Dla Algorytmu Genetycznego) Przypisuje wagi z podanego wektora z powrotem do odpowiednich miejsc w macierzach wag sieci. Pozwala "wgrać" mózg wyewoluowanego osobnika.
void NeuralNetwork::setWeights(const std::vector<double> &weights)
{
    int k = 0;
    for (Matrix *m : weightMatrices)
    {
        for (int r = 0; r < m->getNumRows(); r++)
        {
            for (int c = 0; c < m->getNumCols(); c++)
            {
                if (k < weights.size())
                {
                    m->setValue(r, c, weights[k]);
                    k++;
                }
            }
        }
    }
}

// Zwraca wektor z wartościami neuronów ostatniej warstwy. Jest to ostateczna decyzja sieci (np obrot w prawo).
std::vector<double> NeuralNetwork::getOutputs()
{
    std::vector<double> ret;
    Layer *outputLayer = layers.at(layers.size() - 1);
    for (auto neuron : outputLayer->getNeurons())
    {
        ret.push_back(neuron->getActiveValue());
    }
    return ret;
}