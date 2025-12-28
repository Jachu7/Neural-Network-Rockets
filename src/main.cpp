#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <optional>
#include <limits>
#include <iostream>

#include "siec/Neuron.h"

struct LaserReading {
    sf::Vector2f endPoint; // Gdzie laser się kończy (do rysowania)
    float distance;        // Odległość (dla algorytmu genetycznego)
    bool hit;              // Czy trafił w przeszkodę
};

// Matematyka: Funkcja sprawdzająca przecięcie dwóch odcinków (A-B) i (C-D)
// Zwraca punkt przecięcia w parametrze 'intersection' i true jeśli się przecinają
bool getLineIntersection(sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4, sf::Vector2f& intersection) {
    float s1_x = p2.x - p1.x;
    float s1_y = p2.y - p1.y;
    float s2_x = p4.x - p3.x;
    float s2_y = p4.y - p3.y;

    float s = (-s1_y * (p1.x - p3.x) + s1_x * (p1.y - p3.y)) / (-s2_x * s1_y + s1_x * s2_y);
    float t = (s2_x * (p1.y - p3.y) - s2_y * (p1.x - p3.x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        intersection.x = p1.x + (t * s1_x);
        intersection.y = p1.y + (t * s1_y);
        return true;
    }
    return false;
}

struct Rocket {
    sf::Vector2f velocity = {0.f, 0.f};
    sf::Sprite sprite;
    sf::Sprite fireSprite;
    std::vector<LaserReading> lasers;   // Rakieta trzyma SWOJE odczyty
    bool dead = false;
    bool isThrusting = false;


    // Konfiguracja
    const float gravity = 0.02f;
    const float thrustPower = 0.1f;
    const float rotationSpeed = 1.0f;
    const float maxLaserDist = 400.0f;
    std::vector<float> laserAngles = { -90.f, -45.f, 0.f, 45.f, 90.f, 180.f, 135.f, -135.f };

    Rocket(const sf::Texture& shipTexture, const sf::Texture& fireTexture)
        : sprite(shipTexture), fireSprite(fireTexture)
    {
        lasers.resize(laserAngles.size());

        // Konfiguracja ognia (raz, przy tworzeniu)
        fireSprite.setOrigin({ 8.f, 2.f }); // Środek ognia
        fireSprite.setScale({ 2.0f, 2.0f });

        sf::FloatRect spriteBounds = sprite.getLocalBounds();
        sprite.setOrigin(spriteBounds.getCenter());
        sprite.setScale({ 2.0f, 2.0f });
    }

    void reset(sf::Vector2f startPosition) {
        dead = false;
        isThrusting = false;
        sprite.setPosition(startPosition);
        sprite.setRotation(sf::degrees(0.f));
        velocity = {0.f, 0.f};
    }

    // Funkcja ruchu i fizyki
    void update(bool rotateLeft, bool rotateRight, bool thrust) {
        if (dead) return;

        isThrusting = thrust;

        // 1. Obroty
        if (rotateLeft) sprite.rotate(sf::degrees(-rotationSpeed));
        if (rotateRight) sprite.rotate(sf::degrees(rotationSpeed));

        // 2. Grawitacja
        velocity.y += gravity;

        // 3. Ciąg (Silnik)
        if (thrust) {
            float angleRad = (sprite.getRotation().asDegrees() - 90.f) * 3.14159f / 180.f;
            velocity.x += std::cos(angleRad) * thrustPower; // thrust jest ujemny w Twoim kodzie, tutaj zakładam power jako dodatni wektor
            velocity.y += std::sin(angleRad) * thrustPower;

            // --- OBLICZANIE POZYCJI OGNIA (Wewnątrz rakiety!) ---
            float offsetDist = 32.0f; // Jak daleko za rakietą ma być ogień
            // Odejmujemy cos/sin, żeby ogień był Z TYŁU (przeciwnie do zwrotu rakiety)
            float fireX = sprite.getPosition().x - std::cos(angleRad) * offsetDist;
            float fireY = sprite.getPosition().y - std::sin(angleRad) * offsetDist;

            fireSprite.setPosition({ fireX, fireY });
            fireSprite.setRotation(sprite.getRotation());
        }

        // 4. Aplikacja ruchu
        sprite.move(velocity);

        // 5. Tłumienie (opór powietrza)
        velocity *= 0.99f;
    }

    // Funkcja "patrzenia" (Lasery)
    void sense(const std::vector<sf::RectangleShape>& obstacles) {
        if (dead) return;

        sf::Vector2f origin = sprite.getPosition();
        float baseAngle = sprite.getRotation().asDegrees() - 90.f;

        for (size_t i = 0; i < laserAngles.size(); ++i) {
            float rad = (baseAngle + laserAngles[i]) * 3.14159f / 180.f;

            sf::Vector2f rayEnd;
            rayEnd.x = origin.x + std::cos(rad) * maxLaserDist;
            rayEnd.y = origin.y + std::sin(rad) * maxLaserDist;

            float closestDist = maxLaserDist;
            sf::Vector2f closestPoint = rayEnd;
            bool hitSomething = false;

            // Sprawdzanie przeszkód (Twoja logika przeniesiona tutaj)
            for (const auto& p : obstacles) {
                sf::FloatRect b = p.getGlobalBounds();
                // ... (Tutaj wstawiasz logikę getLineIntersection dla 4 ścian) ...
                // Dla czytelności skróciłem ten fragment, ale kopiujesz go z maina
                // Używając getLineIntersection zdefiniowanego globalnie
                std::vector<std::pair<sf::Vector2f, sf::Vector2f>> walls = {
                     {{b.position.x, b.position.y}, {b.position.x + b.size.x, b.position.y}},
                     {{b.position.x + b.size.x, b.position.y}, {b.position.x + b.size.x, b.position.y + b.size.y}},
                     {{b.position.x + b.size.x, b.position.y + b.size.y}, {b.position.x, b.position.y + b.size.y}},
                     {{b.position.x, b.position.y + b.size.y}, {b.position.x, b.position.y}}
                };

                sf::Vector2f hitPoint;
                for(auto& wall : walls){
                    if(getLineIntersection(origin, rayEnd, wall.first, wall.second, hitPoint)){
                         float dist = std::sqrt(std::pow(hitPoint.x - origin.x, 2) + std::pow(hitPoint.y - origin.y, 2));
                         if(dist < closestDist){
                             closestDist = dist;
                             closestPoint = hitPoint;
                             hitSomething = true;
                         }
                    }
                }
            }
            // Zapisujemy wynik W STRUKTURZE
            lasers[i] = { closestPoint, closestDist, hitSomething };
        }
    }

    // Funkcja sprawdzania kolizji (śmierć)
    void checkCollision(const std::vector<sf::RectangleShape>& obstacles) {
        if (dead) return;
        for (const auto& p : obstacles) {
            if (sprite.getGlobalBounds().findIntersection(p.getGlobalBounds())) {
                dead = true; // Rakieta oznacza samą siebie jako martwą
            }
        }
    }

    // Helper do rysowania laserów
    void drawLasers(sf::RenderWindow& win) {
        for (const auto& laser : lasers) {
             sf::Color color = laser.hit ? sf::Color::Red : sf::Color(200, 200, 200);
             sf::Vertex line[] = {
                sf::Vertex{sprite.getPosition(), color},
                sf::Vertex{laser.endPoint, color}
             };
             win.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
    void draw(sf::RenderWindow& win) {
        // 1. Rysuj ogień (tylko jak wciśnięty gaz i nie martwa)
        if (isThrusting && !dead) {
            win.draw(fireSprite);
        }

        // 2. Rysuj rakietę
        win.draw(sprite);

        // 3. Rysuj lasery
        for (const auto& laser : lasers) {
            sf::Color color = laser.hit ? sf::Color::Red : sf::Color(200, 200, 200);
            sf::Vertex line[] = {
                sf::Vertex{sprite.getPosition(), color},
                sf::Vertex{laser.endPoint, color}
            };
            win.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
};

int main()
{
    // config
    bool startowy = true;
    bool nauka = false;
    auto window = sf::RenderWindow(sf::VideoMode({ 1000u, 1000u }), "Rakietowy algorytm genetyczny");
    window.setFramerateLimit(144);
    sf::Font font("../../src/Roboto_Condensed-Medium.ttf"); // Upewnij się, że ścieżka jest poprawna

    // tekst
    sf::Text text(font);
    text.setString("Rakietowy algorytm genetyczny");
    text.setCharacterSize(36);
    text.setFillColor(sf::Color::Black);
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(textBounds.getCenter());
    text.setPosition({ 500.f, 400.f });

    // guzik
    sf::RectangleShape button({ 300.f, 60.f });
    button.setFillColor(sf::Color(70, 130, 180));
    button.setOrigin(button.getLocalBounds().getCenter());
    button.setPosition({ 500.f, 480.f });
    sf::Text buttonText(font);
    buttonText.setString(L"Rozpocznij naukę latania!");
    buttonText.setCharacterSize(28);
    buttonText.setFillColor(sf::Color::White);
    buttonText.setOrigin(buttonText.getLocalBounds().getCenter());
    buttonText.setPosition(button.getPosition());

    // cel
    sf::CircleShape c({95.f});
    c.setFillColor(sf::Color(0,255,0));
    c.setPosition({20.f, 20.f});

    // przeszkody
    std::vector<sf::RectangleShape> przeszkody;
    auto dodajPrzeszkode = [&](sf::Vector2f size, sf::Vector2f pos) {
        sf::RectangleShape p(size);
        p.setFillColor(sf::Color::Black);
        p.setPosition(pos);
        przeszkody.push_back(p);
    };

    dodajPrzeszkode({ 400.f, 30.f }, { 600.f, 420.f });
    dodajPrzeszkode({ 600.f, 30.f }, { 0.f, 220.f });
    dodajPrzeszkode({ 600.f, 30.f }, { 0.f, 720.f });
    dodajPrzeszkode({ 1000.f, 10.f }, { 0.f, 0.f });     // Sufit
    dodajPrzeszkode({ 1000.f, 10.f }, { 0.f, 990.f });   // Podłoga
    dodajPrzeszkode({ 10.f, 1000.f }, { 0.f, 0.f });     // Lewa ściana
    dodajPrzeszkode({ 10.f, 1000.f }, { 990.f, 0.f });   // Prawa ściana

    sf::Texture texture;
    if (!texture.loadFromFile("../../src/rakieta.png")) {}
    sf::Texture fireTexture;
    if (!fireTexture.loadFromFile("../../src/ogien.png")) {}

    Rocket gracz(texture, fireTexture);
    sf::Vector2f startPos = { 900.f, 900.f };
    gracz.reset(startPos);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            if (event->is<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (button.getGlobalBounds().contains(mousePos)) {
                    startowy = false;
                    nauka = true;
                }
            }
        }

        if (nauka) {
            // 1. Pobieranie wejścia (w przyszłości tutaj wepniesz sieć neuronową)
            bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
            bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
            bool space = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

            // 2. Aktualizacja rakiety (cała logika w środku!)
            gracz.update(left, right, space);
            gracz.checkCollision(przeszkody);
            gracz.sense(przeszkody); // Rakieta "rozgląda się"

            if (gracz.dead) {
                gracz.reset(startPos);
            }
        }

        // rysowanie
        window.clear(sf::Color::White);
        if (startowy) {
            window.draw(text);
            window.draw(button);
            window.draw(buttonText);
        }
        if (nauka) {
            window.draw(c);
            gracz.draw(window);
            for (const auto& p : przeszkody) window.draw(p);
        }
        window.display();
    }

    Neuron *n = new Neuron(20);
    std::cout << "Value: " << n->getValue() << std::endl;
    std::cout << "Activated value: " << n->getActiveValue() << std::endl;
    std::cout << "Derived value: " << n->getDerivativeValue() << std::endl;


}