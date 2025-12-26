#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <optional>
#include <limits> // Potrzebne do nieskończoności
#include <iostream>

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

int main()
{
    // config
    bool startowy = true;
    bool nauka = false;
    auto window = sf::RenderWindow(sf::VideoMode({ 1000u, 1000u }), "Rakietowy algorytm genetyczny");
    window.setFramerateLimit(144);
    sf::Font font("../../src/Roboto_Condensed-Medium.ttf"); // Upewnij się, że ścieżka jest poprawna

    // fizyka
    float velocityY = 0.f;
    float velocityX = 0.f;
    const float gravity = 0.02f;
    const float thrust = -0.1f;
    const float rotationSpeed = 1.0f;

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
    if (!texture.loadFromFile("../../src/rakieta.png")) {
        // Fallback jeśli brak tekstury (zastępczy kształt)
        // return -1;
    }

    sf::Sprite sprite(texture);
    sf::FloatRect spriteBounds = sprite.getLocalBounds();
    sprite.setOrigin(spriteBounds.getCenter());
    sprite.setPosition({ 900.f, 900.f });
    sprite.setScale({ 2.0f, 2.0f });

    sf::Texture fireTexture;
    bool hasFireTexture = fireTexture.loadFromFile("../../src/ogien.png");
    sf::Sprite fireSprite(fireTexture);

    if (hasFireTexture) {
        fireSprite.setOrigin({ 8.f, 2.f });
        fireSprite.setScale({ 2.0f, 2.0f });
    }

    // KONFIGURACJA LASERÓW
    // Kąty względem rakiety (0 to prosto, -90 to lewo, 90 to prawo)
    std::vector<float> laserAngles = { -90.f, -45.f, 0.f, 45.f, 90.f };
    const float maxLaserDist = 400.0f; // Maksymalny zasięg lasera
    std::vector<LaserReading> laserReadings(laserAngles.size()); // Tu trzymamy wyniki

    while (window.isOpen())
    {
        for (int i = 0; i < laserReadings.size(); i++) {
            std::cout << laserReadings[i].distance << std::endl;
        }
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

        bool drawFire = false;

        if (nauka) {
            // Sterowanie
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                sprite.rotate(sf::degrees(-rotationSpeed));
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                sprite.rotate(sf::degrees(rotationSpeed));
            }
            velocityY += gravity;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
                // SFML 0 stopni to prawo, Twoja grafika 0 stopni to góra, stąd -90
                float angleInRadians = (sprite.getRotation().asDegrees() - 90.f) * 3.14159f / 180.f;
                velocityX += std::cos(angleInRadians) * std::abs(thrust);
                velocityY += std::sin(angleInRadians) * std::abs(thrust);

                drawFire = true;
                float offsetDist = 32.0f;
                float fireX = sprite.getPosition().x - std::cos(angleInRadians) * offsetDist;
                float fireY = sprite.getPosition().y - std::sin(angleInRadians) * offsetDist;
                fireSprite.setPosition({ fireX, fireY });
                fireSprite.setRotation(sprite.getRotation());
            }
            sprite.move({ velocityX, velocityY });

            velocityX *= 0.99f;
            velocityY *= 0.99f;

            // Kolizja gracza
            for (const auto& p : przeszkody) {
                if (sprite.getGlobalBounds().findIntersection(p.getGlobalBounds())) {
                    sprite.setPosition({ 900.f, 900.f });
                    sprite.setRotation(sf::degrees(0.f));
                    velocityX = 0.f;
                    velocityY = 0.f;
                }
            }

            // --- OBLICZANIE LASERÓW ---
            sf::Vector2f origin = sprite.getPosition();
            float baseAngle = sprite.getRotation().asDegrees() - 90.f; // Kąt przodu rakiety

            for (size_t i = 0; i < laserAngles.size(); ++i) {
                float rad = (baseAngle + laserAngles[i]) * 3.14159f / 180.f;

                // Domyślny koniec lasera (maksymalny zasięg)
                sf::Vector2f rayEnd;
                rayEnd.x = origin.x + std::cos(rad) * maxLaserDist;
                rayEnd.y = origin.y + std::sin(rad) * maxLaserDist;

                float closestDist = maxLaserDist;
                sf::Vector2f closestPoint = rayEnd;
                bool hitSomething = false;

                // Sprawdź każdą przeszkodę
                for (const auto& p : przeszkody) {
                    sf::FloatRect b = p.getGlobalBounds();

                    // Definicja 4 krawędzi prostokąta przeszkody
                    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> walls = {
                        {{b.position.x, b.position.y}, {b.position.x + b.size.x, b.position.y}}, // Góra
                        {{b.position.x + b.size.x, b.position.y}, {b.position.x + b.size.x, b.position.y + b.size.y}}, // Prawa
                        {{b.position.x + b.size.x, b.position.y + b.size.y}, {b.position.x, b.position.y + b.size.y}}, // Dół
                        {{b.position.x, b.position.y + b.size.y}, {b.position.x, b.position.y}} // Lewa
                    };

                    sf::Vector2f hitPoint;
                    for (const auto& wall : walls) {
                        if (getLineIntersection(origin, rayEnd, wall.first, wall.second, hitPoint)) {
                            // Oblicz dystans do trafienia
                            float dist = std::sqrt(std::pow(hitPoint.x - origin.x, 2) + std::pow(hitPoint.y - origin.y, 2));
                            if (dist < closestDist) {
                                closestDist = dist;
                                closestPoint = hitPoint;
                                hitSomething = true;
                            }
                        }
                    }
                }

                // Zapisz wynik dla tego lasera
                laserReadings[i] = { closestPoint, closestDist, hitSomething };
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
            window.draw(sprite);
            window.draw(c);
            if (drawFire && hasFireTexture) {
                window.draw(fireSprite);
            }
            for (const auto& p : przeszkody) window.draw(p);

            // Rysowanie linii laserów
            for (const auto& laser : laserReadings) {
                sf::Color color = laser.hit ? sf::Color::Red : sf::Color(200, 200, 200); // Czerwony jak trafi, szary jak nie

                sf::Vertex line[] = {
                    sf::Vertex{sprite.getPosition(), color},
                    sf::Vertex{laser.endPoint, color}
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }

        window.display();
    }
}