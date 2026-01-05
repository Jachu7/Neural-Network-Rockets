#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <optional>
#include <limits>
#include <iostream>
#include <random>
#include <algorithm>
#include <numeric>

#include "siec/Neuron.h"
#include "siec/Matrix.h"
#include "siec/NeuralNetwork.h"
#include "siec/utils/MultiplyMatrix.h"

// Konfiguracja
const int POPULATION_SIZE = 100;      // ilość rakiet
const int MUTATION_RATE = 5;          // 5%
const double MUTATION_STRENGTH = 0.1; // 0-1
const int LIFETIME = 10000;           // Czas trwania rundy

const double M_PI_VAL = 3.14159265358979323846;

struct LaserReading
{
    sf::Vector2f endPoint;
    float distance;
    bool hit;
};

// Oblicza punkt przecięcia dwóch odcinków. Jest to funkcja matematyczna niezbędna do działania "oczu" (laserów) rakiety – sprawdza, czy promień lasera przecina ścianę przeszkody
bool getLineIntersection(sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4, sf::Vector2f &intersection)
{
    float s1_x = p2.x - p1.x;
    float s1_y = p2.y - p1.y;
    float s2_x = p4.x - p3.x;
    float s2_y = p4.y - p3.y;

    float denom = (-s2_x * s1_y + s1_x * s2_y);
    if (std::abs(denom) < 0.0001f)
        return false;

    float s = (-s1_y * (p1.x - p3.x) + s1_x * (p1.y - p3.y)) / denom;
    float t = (s2_x * (p1.y - p3.y) - s2_y * (p1.x - p3.x)) / denom;

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        intersection.x = p1.x + (t * s1_x);
        intersection.y = p1.y + (t * s1_y);
        return true;
    }
    return false;
}

// konstruktor rakiety
struct Rocket
{
    sf::Vector2f velocity = {0.f, 0.f};
    sf::Sprite sprite;
    sf::Sprite fireSprite;
    std::vector<LaserReading> lasers;

    NeuralNetwork *brain = nullptr;
    double fitness = 0.0;
    bool dead = false;
    bool completed = false;
    bool isThrusting = false;

    int timeAlive = 0;
    std::vector<bool> visitedCheckpoints;
    float bestDistanceToTarget = 999999.f; // Resetowane przy każdym checkpoincie
    sf::Vector2f lastPosition = {0.f, 0.f};
    int stuckCounter = 0;

    const float gravity = 0.02f;
    const float thrustPower = 0.1f;
    const float rotationSpeed = 3.0f;
    const float maxLaserDist = 400.0f;
    std::vector<float> laserAngles = {-90.f, -45.f, -20.f, 0.f, 20.f, 45.f, 90.f, 180.f};

    // topologia sieci neuronowej, 13 wejsc, 8 warstw ukrytych i 3 wyjscia
    const std::vector<int> topology = {13, 8, 3};

    // skalowanie raycastów (laserów) oraz tekstur rakiety (i inicjacja sieci)
    Rocket(const sf::Texture &shipTexture, const sf::Texture &fireTexture, bool initBrain = true)
        : sprite(shipTexture), fireSprite(fireTexture)
    {
        lasers.resize(laserAngles.size());
        fireSprite.setOrigin({8.f, 2.f});
        fireSprite.setScale({2.0f, 2.0f});
        sf::FloatRect spriteBounds = sprite.getLocalBounds();
        sprite.setOrigin(spriteBounds.getCenter());
        sprite.setScale({2.0f, 2.0f});

        if (initBrain)
        {
            brain = new NeuralNetwork(topology);
        }
    }

    // konstruktor kopiujacy zeby tworzyc nowa rakiete
    Rocket(const Rocket &other) : sprite(other.sprite), fireSprite(other.fireSprite), topology(other.topology)
    {
        lasers = other.lasers;
        velocity = other.velocity;
        fitness = other.fitness;
        dead = other.dead;
        completed = other.completed;
        isThrusting = other.isThrusting;
        visitedCheckpoints = other.visitedCheckpoints;
        timeAlive = other.timeAlive;
        bestDistanceToTarget = other.bestDistanceToTarget;
        stuckCounter = other.stuckCounter;
        lastPosition = other.lastPosition;

        if (other.brain)
        {
            brain = new NeuralNetwork(topology);
            brain->setWeights(other.brain->getWeights());
        }
        else
        {
            brain = nullptr;
        }
    }

    // operator przypisania kopiującego
    Rocket &operator=(const Rocket &other)
    {
        if (this == &other)
            return *this;

        sprite = other.sprite;
        fireSprite = other.fireSprite;
        lasers = other.lasers;
        velocity = other.velocity;
        fitness = other.fitness;
        dead = other.dead;
        completed = other.completed;
        isThrusting = other.isThrusting;
        visitedCheckpoints = other.visitedCheckpoints;
        timeAlive = other.timeAlive;
        bestDistanceToTarget = other.bestDistanceToTarget;
        stuckCounter = other.stuckCounter;
        lastPosition = other.lastPosition;

        delete brain;
        if (other.brain)
        {
            brain = new NeuralNetwork(topology);
            brain->setWeights(other.brain->getWeights());
        }
        else
        {
            brain = nullptr;
        }
        return *this;
    }

    ~Rocket()
    {
        if (brain)
            delete brain;
    }

    // Przygotowuje rakietę do nowej rundy (pokolenia). Resetuje pozycję, prędkość i flagi życia, ale zachowuje wytrenowany "mózg"
    void reset(sf::Vector2f startPosition, int checkpointsCount)
    {
        dead = false;
        completed = false;
        isThrusting = false;
        sprite.setPosition(startPosition);
        sprite.setRotation(sf::degrees(0.f));
        velocity = {0.f, 0.f};
        fitness = 0.0;
        timeAlive = 0;
        bestDistanceToTarget = 999999.f;
        stuckCounter = 0;
        lastPosition = startPosition;
        visitedCheckpoints.assign(checkpointsCount, false);
    }

    // Odpowiada za podstawową fizykę: dodaje grawitację do prędkości, przesuwa obiekt i wyhamowuje go (tarcie). Zawiera też mechanizm wykrywający "utknięcie" w miejscu (jeśli rakieta się nie rusza, zostaje uśmiercona)
    void updatePhysics()
    {
        if (dead || completed)
            return;

        velocity.y += gravity;
        sprite.move(velocity);
        velocity *= 0.99f;

        // Wykrywanie utknięcia
        if (timeAlive % 100 == 0)
        {
            float distMoved = std::sqrt(
                std::pow(sprite.getPosition().x - lastPosition.x, 2) +
                std::pow(sprite.getPosition().y - lastPosition.y, 2));

            if (distMoved < 20.0f)
            {
                stuckCounter++;
                if (stuckCounter >= 3)
                {
                    dead = true;
                }
            }
            else
            {
                stuckCounter = 0;
            }
            lastPosition = sprite.getPosition();
        }
    }

    // Główna pętla decyzyjna AI. Pobiera dane wejściowe (odczyty laserów, prędkość, kąt do celu), normalizuje je i przepuszcza przez sieć neuronową. Wynik sieci decyduje o obrocie i włączeniu silnika.
    void thinkAndMove(const std::vector<sf::CircleShape> &checkpoints, sf::Vector2f finalTarget)
    {
        if (dead || completed)
            return;

        sf::Vector2f currentTarget = finalTarget;
        for (size_t i = 0; i < checkpoints.size(); ++i)
        {
            if (!visitedCheckpoints[i])
            {
                currentTarget = checkpoints[i].getPosition();
                break;
            }
        }

        std::vector<double> inputs;

        // Lasery [-1, 1]
        for (const auto &l : lasers)
        {
            double normalized = l.distance / maxLaserDist;
            inputs.push_back(2.0 * normalized - 1.0);
        }

        // Prędkość [-1, 1]
        double vx_norm = std::max(-1.0, std::min(1.0, velocity.x / 4.0));
        double vy_norm = std::max(-1.0, std::min(1.0, velocity.y / 4.0));
        inputs.push_back(vx_norm);
        inputs.push_back(vy_norm);

        // Nawigacja
        double dx = currentTarget.x - sprite.getPosition().x;
        double dy = currentTarget.y - sprite.getPosition().y;
        double distToTarget = std::sqrt(dx * dx + dy * dy);

        // KLUCZOWE: Śledzimy najlepszy dystans do AKTUALNEGO celu
        if (distToTarget < bestDistanceToTarget)
        {
            bestDistanceToTarget = distToTarget;
        }

        double dist_normalized = std::min(1.0, distToTarget / 1500.0);
        inputs.push_back(2.0 * dist_normalized - 1.0);

        double angleToTarget = std::atan2(dy, dx);
        double currentAngle = (sprite.getRotation().asDegrees() - 90.0) * (M_PI_VAL / 180.0);
        double angleDiff = angleToTarget - currentAngle;

        while (angleDiff <= -M_PI_VAL)
            angleDiff += 2 * M_PI_VAL;
        while (angleDiff > M_PI_VAL)
            angleDiff -= 2 * M_PI_VAL;

        inputs.push_back(angleDiff / M_PI_VAL);
        inputs.push_back(0.0); // Bias

        brain->setCurrentInput(inputs);
        brain->feedForward();
        std::vector<double> outputs = brain->getOutputs();

        bool rotLeft = outputs[0] > 0.0;
        bool rotRight = outputs[1] > 0.0;
        bool thrust = outputs[2] > 0.0;

        isThrusting = thrust;
        if (rotLeft)
            sprite.rotate(sf::degrees(-rotationSpeed));
        if (rotRight)
            sprite.rotate(sf::degrees(rotationSpeed));

        if (thrust)
        {
            float angleRad = (sprite.getRotation().asDegrees() - 90.f) * 3.14159f / 180.f;
            velocity.x += std::cos(angleRad) * thrustPower;
            velocity.y += std::sin(angleRad) * thrustPower;

            float offsetDist = 32.0f;
            float fireX = sprite.getPosition().x - std::cos(angleRad) * offsetDist;
            float fireY = sprite.getPosition().y - std::sin(angleRad) * offsetDist;
            fireSprite.setPosition({fireX, fireY});
            fireSprite.setRotation(sprite.getRotation());
        }
    }

    // Symuluje działanie czujników odległości. Wypuszcza promienie w różnych kierunkach, sprawdza kolizje z przeszkodami i zapisuje odległość do najbliższej ściany.
    void sense(const std::vector<sf::RectangleShape> &obstacles)
    {
        if (dead || completed)
            return;

        sf::Vector2f origin = sprite.getPosition();
        float baseAngle = sprite.getRotation().asDegrees() - 90.f;

        for (size_t i = 0; i < laserAngles.size(); ++i)
        {
            float rad = (baseAngle + laserAngles[i]) * 3.14159f / 180.f;
            sf::Vector2f rayEnd;
            rayEnd.x = origin.x + std::cos(rad) * maxLaserDist;
            rayEnd.y = origin.y + std::sin(rad) * maxLaserDist;

            float closestDist = maxLaserDist;
            sf::Vector2f closestPoint = rayEnd;
            bool hitSomething = false;

            for (const auto &p : obstacles)
            {
                sf::FloatRect b = p.getGlobalBounds();
                std::vector<std::pair<sf::Vector2f, sf::Vector2f>> walls = {
                    {{b.position.x, b.position.y}, {b.position.x + b.size.x, b.position.y}},
                    {{b.position.x + b.size.x, b.position.y}, {b.position.x + b.size.x, b.position.y + b.size.y}},
                    {{b.position.x + b.size.x, b.position.y + b.size.y}, {b.position.x, b.position.y + b.size.y}},
                    {{b.position.x, b.position.y + b.size.y}, {b.position.x, b.position.y}}};

                sf::Vector2f hitPoint;
                for (auto &wall : walls)
                {
                    if (getLineIntersection(origin, rayEnd, wall.first, wall.second, hitPoint))
                    {
                        float dist = std::sqrt(std::pow(hitPoint.x - origin.x, 2) + std::pow(hitPoint.y - origin.y, 2));
                        if (dist < closestDist)
                        {
                            closestDist = dist;
                            closestPoint = hitPoint;
                            hitSomething = true;
                        }
                    }
                }
            }
            lasers[i] = {closestPoint, closestDist, hitSomething};
        }
    }

    // Sprawdza, czy rakieta przeleciała przez niebieski punkt kontrolny. Jeśli tak, zalicza go i zmusza algorytm do celowania w kolejny punkt
    void checkCheckpoints(const std::vector<sf::CircleShape> &checkpoints)
    {
        if (dead || completed)
            return;

        if (visitedCheckpoints.size() != checkpoints.size())
        {
            visitedCheckpoints.resize(checkpoints.size(), false);
        }

        sf::FloatRect myBounds = sprite.getGlobalBounds();
        for (size_t i = 0; i < checkpoints.size(); ++i)
        {
            if (visitedCheckpoints[i])
                continue;
            if (myBounds.findIntersection(checkpoints[i].getGlobalBounds()))
            {
                visitedCheckpoints[i] = true;
                // === KLUCZOWA POPRAWKA ===
                // Resetujemy dystans, aby zmusić rakietę do lotu do NOWEGO celu
                bestDistanceToTarget = 999999.f;
            }
        }
    }

    // Sprawdza kolizje fizyczne. Jeśli rakieta uderzy w ścianę to ginie. Jeśli dotknie zielonego celu końcowego – wygrywa (oznaczana jako completed)
    void checkCollision(const std::vector<sf::RectangleShape> &obstacles, sf::Vector2f targetPos)
    {
        if (dead || completed)
            return;

        for (const auto &p : obstacles)
        {
            if (sprite.getGlobalBounds().findIntersection(p.getGlobalBounds()))
            {
                dead = true;
            }
        }
        // Kolizja z granicami ekranu
        if (sprite.getPosition().x < 0 || sprite.getPosition().x > 1000 ||
            sprite.getPosition().y < 0 || sprite.getPosition().y > 1000)
        {
            dead = true;
        }

        float dist = std::sqrt(std::pow(sprite.getPosition().x - targetPos.x, 2) +
                               std::pow(sprite.getPosition().y - targetPos.y, 2));
        if (dist < 50.0f)
        {
            bool allVisited = true;
            for (auto v : visitedCheckpoints)
                if (!v)
                    allVisited = false;

            if (allVisited)
            {
                completed = true;
                dead = true;
            }
        }
    }

    // Funkcja oceny (Fitness Function). Oblicza wynik rakiety na koniec życia. Punktuje (w kolejności ważności): zdobyte checkpointy, bliskość do aktualnego celu, ukończenie trasy i szybkość przelotu. Decyduje o tym, kto przekaże geny dalej.
    void calcFitness(const std::vector<sf::CircleShape> &checkpoints, sf::Vector2f finalTarget, sf::Vector2f startPos, int maxLifetime)
    {
        fitness = 0.0;

        // 1. Checkpointy (Baza sukcesu) - 10,000 pkt za każdy
        int checkpointCount = 0;
        for (bool visited : visitedCheckpoints)
        {
            if (visited)
                checkpointCount++;
        }
        fitness += checkpointCount * 10000.0;

        // 2. Dystans do AKTUALNEGO celu (Max 2000 pkt)
        // Dzięki temu, że max za dystans (2000) < bonus za checkpoint (10000),
        // rakieta, która zdobyła checkpoint zawsze wygrywa z tą, która jest tylko blisko.
        if (bestDistanceToTarget < 99999.f)
        {
            double distReward = std::max(0.0, 2000.0 - (bestDistanceToTarget * 2.0));
            fitness += distReward;
        }

        // 3. Bonus za ukończenie i szybkość
        if (completed)
        {
            fitness += 20000.0;
            // Im szybciej, tym więcej punktów
            fitness += ((double)maxLifetime / (double)(timeAlive + 1)) * 5000.0;
        }

        // 4. Mikro-nagroda za ruch (żeby nie kręciły się w miejscu)
        float totalMovement = std::sqrt(
            std::pow(sprite.getPosition().x - startPos.x, 2) +
            std::pow(sprite.getPosition().y - startPos.y, 2));
        fitness += totalMovement * 0.1;
    }

    // Rysuje rakietę na ekranie. Jeśli silnik jest włączony, dorysowuje ogień. Najlepsza rakieta w populacji jest wyróżniona kolorem zielonym
    void draw(sf::RenderWindow &win, bool best)
    {
        if (dead && !best)
            return;
        if (isThrusting && !dead)
            win.draw(fireSprite);
        if (best)
            sprite.setColor(sf::Color::Green);
        else
            sprite.setColor(sf::Color(255, 255, 255, 100));
        win.draw(sprite);
    }
};

double randomDouble()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

double randomRange(double min, double max)
{
    return min + randomDouble() * (max - min);
}

// Ewolucja -
// Implementuje Algorytm Genetyczny.
// Sortuje rakiety od najlepszej do najgorszej.
// Przepisuje najlepsze jednostki bez zmian do nowej populacji (Elityzm).
// Tworzy resztę nowej populacji poprzez mieszanie wag dwóch rodziców (Crossover) i losowe zmiany wag (Mutacja).
std::vector<Rocket> evolve(std::vector<Rocket> &oldPop, sf::Texture &t, sf::Texture &ft, sf::Vector2f startPos, int cpCount)
{
    std::vector<Rocket> newPop;
    newPop.reserve(POPULATION_SIZE);

    std::sort(oldPop.begin(), oldPop.end(), [](const Rocket &a, const Rocket &b)
              { return a.fitness > b.fitness; });

    // 1. ELITYZM - Zwiększony do 8, aby zachować stabilność
    int eliteCount = 8;
    for (int i = 0; i < std::min(eliteCount, (int)oldPop.size()); ++i)
    {
        Rocket elita(t, ft, true);
        elita.brain->setWeights(oldPop[i].brain->getWeights());
        elita.reset(startPos, cpCount);
        newPop.push_back(elita);
    }

    // 2. RESZTA POPULACJI (Crossover + Mutacja)
    while (newPop.size() < POPULATION_SIZE)
    {
        // Selekcja Turniejowa
        int best1 = rand() % oldPop.size();
        for (int i = 0; i < 5; ++i)
        { // Turniej 5 osobników
            int cand = rand() % oldPop.size();
            if (oldPop[cand].fitness > oldPop[best1].fitness)
                best1 = cand;
        }
        int best2 = rand() % oldPop.size();
        for (int i = 0; i < 5; ++i)
        {
            int cand = rand() % oldPop.size();
            if (oldPop[cand].fitness > oldPop[best2].fitness)
                best2 = cand;
        }

        Rocket child(t, ft, true);
        std::vector<double> genes1 = oldPop[best1].brain->getWeights();
        std::vector<double> genes2 = oldPop[best2].brain->getWeights();
        std::vector<double> childGenes;

        for (size_t i = 0; i < genes1.size(); ++i)
        {
            double gene = (rand() % 2 == 0) ? genes1[i] : genes2[i];
            if ((rand() % 100) < MUTATION_RATE)
            {
                gene += randomRange(-MUTATION_STRENGTH, MUTATION_STRENGTH);
                gene = std::max(-1.0, std::min(1.0, gene));
            }
            childGenes.push_back(gene);
        }

        child.brain->setWeights(childGenes);
        child.reset(startPos, cpCount);
        newPop.push_back(child);
    }

    return newPop;
}

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode({1000u, 1000u}), "Symulacja algorytmu genetycznego - Neural Network Rockets C++");
    window.setFramerateLimit(0);
    sf::Font font;
    if (!font.openFromFile("../../src/Roboto_Condensed-Medium.ttf"))
    {
    }

    sf::Text textGen(font);
    textGen.setCharacterSize(24);
    textGen.setFillColor(sf::Color::Black);
    textGen.setPosition({10.f, 10.f});

    std::vector<sf::RectangleShape> przeszkody;
    auto dodajPrzeszkode = [&](sf::Vector2f size, sf::Vector2f pos)
    {
        sf::RectangleShape p(size);
        p.setFillColor(sf::Color::Black);
        p.setPosition(pos);
        przeszkody.push_back(p);
    };

    dodajPrzeszkode({600.f, 20.f}, {400.f, 450.f});
    dodajPrzeszkode({600.f, 20.f}, {0.f, 220.f});
    dodajPrzeszkode({600.f, 20.f}, {0.f, 720.f});
    dodajPrzeszkode({1000.f, 10.f}, {0.f, 0.f});
    dodajPrzeszkode({1000.f, 10.f}, {0.f, 990.f});
    dodajPrzeszkode({10.f, 1000.f}, {0.f, 0.f});
    dodajPrzeszkode({10.f, 1000.f}, {990.f, 0.f});

    std::vector<sf::CircleShape> checkpoints;
    auto dodajCheckpoint = [&](sf::Vector2f pos)
    {
        sf::CircleShape ch({70.f});
        ch.setFillColor(sf::Color(100, 150, 200, 150));
        ch.setPosition(pos);
        checkpoints.push_back(ch);
    };

    dodajCheckpoint({700.f, 620.f});
    dodajCheckpoint({100.f, 430.f});
    dodajCheckpoint({700.f, 130.f});

    sf::CircleShape cel({50.f});
    cel.setFillColor(sf::Color::Green);
    cel.setOrigin({50.f, 50.f});
    cel.setPosition({100.f, 100.f});

    sf::Texture texture;
    if (!texture.loadFromFile("../../src/rakieta.png"))
        return -1;
    sf::Texture fireTexture;
    if (!fireTexture.loadFromFile("../../src/ogien.png"))
        return -1;

    sf::Vector2f startPos = {100.f, 900.f};

    std::vector<Rocket> population;
    for (int i = 0; i < POPULATION_SIZE; ++i)
    {
        Rocket r(texture, fireTexture, true);
        r.reset(startPos, checkpoints.size());
        population.push_back(r);
    }

    int generation = 1;
    int timer = 0;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        bool allDead = true;
        for (auto &rocket : population)
        {
            if (!rocket.dead && !rocket.completed)
            {
                rocket.timeAlive++;
                rocket.sense(przeszkody);
                rocket.checkCheckpoints(checkpoints);
                rocket.thinkAndMove(checkpoints, cel.getPosition());
                rocket.updatePhysics();
                rocket.checkCollision(przeszkody, cel.getPosition());
                allDead = false;
            }
        }

        timer++;

        if (allDead || timer > LIFETIME)
        {
            double maxFit = -std::numeric_limits<double>::infinity();
            double sumFit = 0.0;
            int completedCount = 0;
            int maxCheckpoints = 0;

            for (auto &r : population)
            {
                r.calcFitness(checkpoints, cel.getPosition(), startPos, LIFETIME);
                if (r.fitness > maxFit)
                    maxFit = r.fitness;
                sumFit += r.fitness;
                if (r.completed)
                    completedCount++;

                int cpCount = 0;
                for (bool v : r.visitedCheckpoints)
                    if (v)
                        cpCount++;
                maxCheckpoints = std::max(maxCheckpoints, cpCount);
            }

            std::cout << "=== GEN " << generation << " ===" << std::endl;
            std::cout << "  Max Fitness: " << (long)maxFit << std::endl;
            std::cout << "  Ukończone: " << completedCount << "/" << POPULATION_SIZE << std::endl;
            std::cout << "  Max CP: " << maxCheckpoints << "/3" << std::endl;

            population = evolve(population, texture, fireTexture, startPos, checkpoints.size());
            generation++;
            timer = 0;
        }

        window.clear(sf::Color::White);
        for (const auto &cp : checkpoints)
            window.draw(cp);
        window.draw(cel);
        for (const auto &p : przeszkody)
            window.draw(p);

        // Znajdź najlepszą do rysowania (żywą, a jak nie ma to martwą z najlepszym wynikiem)
        int bestIdx = 0;
        double bestScore = -1.0;

        // Prostym sposobem wizualizacji jest pokazanie tej, która ma najwięcej CP
        // i jest najbliżej celu (liczone na bieżąco dla wizualizacji)
        for (size_t i = 0; i < population.size(); ++i)
        {
            if (population[i].dead)
                continue;

            int cp = 0;
            for (auto v : population[i].visitedCheckpoints)
                if (v)
                    cp++;

            // Prosta heurystyka do wyboru "kamery"
            double score = cp * 10000.0 - population[i].bestDistanceToTarget;
            if (score > bestScore)
            {
                bestScore = score;
                bestIdx = i;
            }
        }

        for (size_t i = 0; i < population.size(); ++i)
        {
            population[i].draw(window, (i == bestIdx));
        }

        textGen.setString("Gen: " + std::to_string(generation) + " | Step: " + std::to_string(timer));
        window.draw(textGen);
        window.display();
    }
    return 0;
}