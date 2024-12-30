#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PLAYER_SPEED = 300.0f;
const float BULLET_SPEED = 500.0f;
const float POWERUP_SPEED = 150.0f;
float ENEMY_SPEED = 100.0f;
const float SHOOT_COOLDOWN = 0.5f;
int ENEMY_ROWS = 4;
const int ENEMY_COLUMNS = 8;
const float ENEMY_SPACING = 60.0f;

// Game states
enum class GameState { Menu, Playing, GameOver, Exit };

// Power-up types
enum class PowerUpType { None, RapidFire, Shield };

// Structures for bullets, enemies, and power-ups
struct Bullet {
    sf::RectangleShape shape;
};

struct Enemy {
    sf::Sprite sprite;
};

struct PowerUp {
    sf::Sprite sprite;
    PowerUpType type;
    float timer; // Timer to track how long the power-up lasts
};

// Function prototypes
void createMenu(sf::RenderWindow& window, sf::Font& font, GameState& gameState, bool isGameOver = false);
void runGame(sf::RenderWindow& window, sf::Font& font, GameState& gameState);

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Space Invaders");
    window.setFramerateLimit(60);

    GameState gameState = GameState::Menu;
    sf::Font font;

    // Load font
    if (!font.loadFromFile("alien.ttf")) {
        return -1; // Exit if font cannot be loaded
    }

    while (window.isOpen()) {
        switch (gameState) {
        case GameState::Menu:
            createMenu(window, font, gameState);
            break;
        case GameState::Playing:
            runGame(window, font, gameState);
            break;
        case GameState::GameOver:
            createMenu(window, font, gameState, true);
            break;
        case GameState::Exit:
            window.close();
            break;
        }
    }

    return 0;
}

// Function to create the menu
void createMenu(sf::RenderWindow& window, sf::Font& font, GameState& gameState, bool isGameOver) {
    sf::Text title(isGameOver ? "Game Over!" : "Space Invaders", font, 50);
    title.setPosition(WINDOW_WIDTH / 2 - title.getGlobalBounds().width / 2, 100);
    title.setFillColor(sf::Color::White);

    sf::Text startOption(isGameOver ? "Restart" : "Start Game", font, 30);
    startOption.setPosition(WINDOW_WIDTH / 2 - startOption.getGlobalBounds().width / 2, 200);
    startOption.setFillColor(sf::Color::Green);

    sf::Text exitOption("Exit", font, 30);
    exitOption.setPosition(WINDOW_WIDTH / 2 - exitOption.getGlobalBounds().width / 2, 250);
    exitOption.setFillColor(sf::Color::Red);

    while (gameState == GameState::Menu || gameState == GameState::GameOver) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                gameState = GameState::Exit;
                return;
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (startOption.getGlobalBounds().contains(mousePos)) {
                    gameState = GameState::Playing;
                    return;
                }
                if (exitOption.getGlobalBounds().contains(mousePos)) {
                    gameState = GameState::Exit;
                    return;
                }
            }
        }

        window.clear(sf::Color::Black);
        window.draw(title);
        window.draw(startOption);
        window.draw(exitOption);
        window.display();
    }
}

// Function to run the main game loop
void runGame(sf::RenderWindow& window, sf::Font& font, GameState& gameState) {
    // Load textures
    sf::Texture playerTexture, enemyTexture, powerUpTexture, backgroundTexture;
    if (!playerTexture.loadFromFile("player.png") ||
        !enemyTexture.loadFromFile("enemy.png") ||
        !powerUpTexture.loadFromFile("powerup.png") ||
        !backgroundTexture.loadFromFile("background.png")) {
        gameState = GameState::Exit;
        return;
    }

    // Load sounds
    sf::SoundBuffer shootBuffer, destroyBuffer, gameOverBuffer;
    if (!shootBuffer.loadFromFile("shoot.mp3") ||
        !destroyBuffer.loadFromFile("hit.mp3") ||
        !gameOverBuffer.loadFromFile("gameover.mp3")) {
        gameState = GameState::Exit;
        return;
    }
    sf::Sound shootSound(shootBuffer), destroySound(destroyBuffer), gameOverSound(gameOverBuffer);

    // Load background music
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("music.mp3")) {
        gameState = GameState::Exit;
        return;
    }
    backgroundMusic.setLoop(true);
    backgroundMusic.play();

    sf::Sprite background(backgroundTexture);
    sf::Sprite player(playerTexture);
    player.setPosition(WINDOW_WIDTH / 2 - player.getGlobalBounds().width / 2,
        WINDOW_HEIGHT - player.getGlobalBounds().height - 10);

    sf::Text scoreText("Score: 0", font, 20);
    scoreText.setPosition(10, 10);
    int score = 0;

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;

    // Function to initialize enemies
    auto initializeEnemies = [&]() {
        enemies.clear();
        float enemyStartX = 100.0f, enemyStartY = 50.0f;
        for (int row = 0; row < ENEMY_ROWS; ++row) {
            for (int col = 0; col < ENEMY_COLUMNS; ++col) {
                Enemy enemy;
                enemy.sprite.setTexture(enemyTexture);
                enemy.sprite.setPosition(enemyStartX + col * ENEMY_SPACING, enemyStartY + row * 40);
                enemies.push_back(enemy);
            }
        }
        };
    initializeEnemies();  // Initialize the first level of enemies

    sf::Clock clock;
    float shootTimer = 0.0f;
    float enemyDirection = 1.0f;
    bool isRapidFireActive = false;
    float rapidFireDuration = 0.0f;

    // Function to spawn power-ups at random intervals
    auto spawnPowerUp = [&]() {
        if (rand() % 4000 < 10) {  // 1% chance to spawn power-up
            PowerUp powerUp;
            powerUp.sprite.setTexture(powerUpTexture);
            powerUp.sprite.setPosition(rand() % (WINDOW_WIDTH - 50), 0);
            powerUp.type = static_cast<PowerUpType>(rand() % 2 + 1); // Random power-up
            powerUp.timer = 0.0f;
            powerUps.push_back(powerUp);
        }
        };

    while (window.isOpen() && gameState == GameState::Playing) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                gameState = GameState::Exit;
                return;
            }
        }

        float dt = clock.restart().asSeconds();
        shootTimer -= dt;
        rapidFireDuration -= dt;

        // Player movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player.getPosition().x > 0) {
            player.move(-PLAYER_SPEED * dt, 0);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) &&
            player.getPosition().x < WINDOW_WIDTH - player.getGlobalBounds().width) {
            player.move(PLAYER_SPEED * dt, 0);
        }

        // Player shooting
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootTimer <= 0) {
            Bullet bullet;
            bullet.shape.setSize({ 5, 20 });
            bullet.shape.setFillColor(sf::Color::Red);
            bullet.shape.setPosition(player.getPosition().x + player.getGlobalBounds().width / 2 - 2.5f,
                player.getPosition().y);
            bullets.push_back(bullet);
            shootSound.play();
            shootTimer = SHOOT_COOLDOWN;
            if (isRapidFireActive) {
                shootTimer = SHOOT_COOLDOWN / 2;  // Faster shooting with rapid fire
            }
        }

        // Update bullets
        for (size_t i = 0; i < bullets.size(); ++i) {
            bullets[i].shape.move(0, -BULLET_SPEED * dt);
            if (bullets[i].shape.getPosition().y < 0) {
                bullets.erase(bullets.begin() + i);
                --i;
            }
        }

        // Update enemies and check for screen bounds
        bool reverseDirection = false;
        for (auto& enemy : enemies) {
            enemy.sprite.move(enemyDirection * ENEMY_SPEED * dt, 0);
            if (enemy.sprite.getPosition().x <= 0 ||
                enemy.sprite.getPosition().x >= WINDOW_WIDTH - enemy.sprite.getGlobalBounds().width) {
                reverseDirection = true;
            }
            // Game Over if an enemy reaches the bottom of the screen
            if (enemy.sprite.getPosition().y >= WINDOW_HEIGHT - enemy.sprite.getGlobalBounds().height - 10) {
                gameState = GameState::GameOver;
                gameOverSound.play();
                return;
            }
        }
        if (reverseDirection) {
            enemyDirection *= -1.0f;
            for (auto& enemy : enemies) {
                enemy.sprite.move(0, 20); // Move down after bouncing
            }
        }

        // Check for bullet-enemy collisions
        for (size_t i = 0; i < bullets.size(); ++i) {
            for (size_t j = 0; j < enemies.size(); ++j) {
                if (bullets[i].shape.getGlobalBounds().intersects(enemies[j].sprite.getGlobalBounds())) {
                    bullets.erase(bullets.begin() + i);
                    enemies.erase(enemies.begin() + j);
                    destroySound.play();
                    score += 10;
                    scoreText.setString("Score: " + std::to_string(score));
                    --i;
                    break;
                }
            }
        }

        // Power-up movement and collision
        spawnPowerUp();
        for (size_t i = 0; i < powerUps.size(); ++i) {
            powerUps[i].sprite.move(0, POWERUP_SPEED * dt);
            if (powerUps[i].sprite.getGlobalBounds().intersects(player.getGlobalBounds())) {
                // Apply power-up
                if (powerUps[i].type == PowerUpType::RapidFire) {
                    isRapidFireActive = true;
                    rapidFireDuration = 10.0f; // Active for 10 seconds
                }
                powerUps.erase(powerUps.begin() + i);
                --i;
            }
        }

        // Deactivate rapid fire after the duration
        if (rapidFireDuration <= 0.0f) {
            isRapidFireActive = false;
        }

        // Check for win condition and level progression
        if (enemies.empty()) {
            ENEMY_SPEED += 50;  // Increase speed with each level
            ENEMY_ROWS++;
            initializeEnemies();  // Initialize next level of enemies
            score += 100;  // Bonus points for clearing the level
            scoreText.setString("Score: " + std::to_string(score));
        }

        // Rendering
        window.clear();
        window.draw(background);
        window.draw(player);
        for (auto& bullet : bullets) {
            window.draw(bullet.shape);
        }
        for (auto& enemy : enemies) {
            window.draw(enemy.sprite);
        }
        for (auto& powerUp : powerUps) {
            window.draw(powerUp.sprite);
        }
        window.draw(scoreText);
        window.display();
    }
}
