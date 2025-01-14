#include "game.hpp"
#include <iostream>
#include <fstream>

Game::Game() {
    music = LoadMusicStream("Sounds/music.ogg");
    explosion_sound = LoadSound("Sounds/explosion.ogg");
    PlayMusicStream(music);
    InitGame();
}

Game::~Game() {
    Alien::UnloadImages();
    UnloadMusicStream(music);
    UnloadSound(explosion_sound);
}

void Game::Draw() {
    spaceship.Draw();

    for (auto& laser: spaceship.lasers) {
        laser.Draw();
    }

    for (auto& obstacle: obstacles) {
        obstacle.Draw();
    }

    for (auto& alien: aliens) {
        alien.Draw();
    }

    for (auto& laser: alien_lasers) {
        laser.Draw();
    }

    mystery_ship.Draw();
}

void Game::Update() {

        if (run) {
        double current_time = GetTime();
        if (current_time - time_last_spawn > mystery_ship_spawn_interval) {
            mystery_ship.Spawn();
            time_last_spawn = GetTime();
            mystery_ship_spawn_interval = GetRandomValue(10, 20);
        }

        for (auto& laser: spaceship.lasers) {
            laser.Update();
        }

        MoveAliens();
        AlienShootLaser();

        for (auto& laser: alien_lasers) {
            laser.Update();
        }

        DeleteInactiveLasers();

        mystery_ship.Update();

        CheckForCollisions();
    } else {
        if (IsKeyDown(KEY_ENTER)) {
            Reset();
            InitGame();
        }
    }
}

void Game::HandleInput() {

    if (run) {
        if (IsKeyDown(KEY_LEFT)) {
            spaceship.MoveLeft();
        } else if (IsKeyDown(KEY_RIGHT)) {
            spaceship.MoveRight();
        } else if (IsKeyDown(KEY_SPACE)) {
            spaceship.FireLaser();
        }
    }
}

void Game::DeleteInactiveLasers() {
    for (auto it = spaceship.lasers.begin(); it != spaceship.lasers.end();) {
        if (!it -> active) {
            it = spaceship.lasers.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = alien_lasers.begin(); it != alien_lasers.end();) {
        if (!it -> active) {
            it = alien_lasers.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<Obstacle> Game::CreateObstacles() {
    int obstacle_width = Obstacle::grid[0].size() * 3;
    float gap = (GetScreenWidth() - (4 * obstacle_width)) / 5;

    for (int i = 0; i < 4; i++) {
        float offset_x = (i + 1) * gap + i * obstacle_width;
        obstacles.push_back(Obstacle({offset_x, float(GetScreenHeight() - 200)}));
    }
    return obstacles;
}

std::vector<Alien> Game::CreateAliens() {
    std::vector<Alien> aliens;

    for (int row = 0; row < 5; row++) {
        for (int column = 0; column < 11; column++) {

            int alien_type;
            if (row == 0) {
                alien_type = 3;
            } else if (row == 1 || row == 2) {
                alien_type = 2;
            } else {
                alien_type = 1;
            }

            float x = 75 + column * 55;
            float y = 110 + row * 55;
            aliens.push_back(Alien(alien_type, {x, y}));
        }
    }
    return aliens;
}

void Game::MoveAliens() {
    for (auto& alien: aliens) {

        if (alien.position.x + alien.alien_images[alien.type - 1].width > GetScreenWidth() - 25) {
            aliens_direction = -1;
            MoveDownAliens(4);
        }

        if (alien.position.x < 25) {
            aliens_direction = 1;
            MoveDownAliens(4);
        }

        alien.Update(aliens_direction);
    }
}

void Game::MoveDownAliens(int distance) {
    for (auto& alien: aliens) {
        alien.position.y += distance;
    }
}

void Game::AlienShootLaser() {

    double current_time = GetTime();

    if (current_time - time_last_alien_fired >= alien_laser_shoot_interval && !aliens.empty()) {
        int random_index = GetRandomValue(0, aliens.size() - 1);
        Alien& alien = aliens[random_index];
        alien_lasers.push_back(Laser({alien.position.x + alien.alien_images[alien.type - 1].width / 2,
        alien.position.y + alien.alien_images[alien.type - 1].height}, 6));
        time_last_alien_fired = GetTime();
    }
}

void Game::CheckForCollisions() {

    // Spaceship lasers

    for (auto& laser: spaceship.lasers) {
        auto it = aliens.begin();
        while (it != aliens.end()) {
            if (CheckCollisionRecs(it -> GetRect(), laser.GetRect())) {
                PlaySound(explosion_sound);
                if (it -> type == 1) {
                    score += 100;
                } else if (it -> type == 2) {
                    score += 200;
                } else if (it -> type == 3) {
                    score += 300;
                }
                CheckForHighScore();

                it = aliens.erase(it);
                laser.active = false;
            } else {
                ++it;
            }
        }

        for (auto& obstacle: obstacles) {
            auto it = obstacle.blocks.begin();
            while (it != obstacle.blocks.end()) {
                if (CheckCollisionRecs(it -> GetRect(), laser.GetRect())) {
                    it = obstacle.blocks.erase(it);
                    laser.active = false;
                } else {
                    ++it;
                }
            }
        }

        if (CheckCollisionRecs(mystery_ship.GetRect(), laser.GetRect())) {
            mystery_ship.alive = false;
            laser.active = false;
            score += 500;
            CheckForHighScore();
            PlaySound(explosion_sound);
        }
    }

    // Alien lasers

    for (auto& laser: alien_lasers) {
        if (CheckCollisionRecs(laser.GetRect(), spaceship.GetRect())) {
            laser.active = false;
            lives--;
            if (lives == 0) {
                GameOver();
            }
        }

        for (auto& obstacle: obstacles) {
            auto it = obstacle.blocks.begin();
            while (it != obstacle.blocks.end()) {
                if (CheckCollisionRecs(it -> GetRect(), laser.GetRect())) {
                    it = obstacle.blocks.erase(it);
                    laser.active = false;
                } else {
                    ++it;
                }
            }
        }
    }

    // Alien collision with obstacle

    for (auto& alien: aliens) {
        for (auto& obstacle: obstacles) {
            auto it = obstacle.blocks.begin();
            while (it != obstacle.blocks.end()) {
                if (CheckCollisionRecs(it -> GetRect(), alien.GetRect())) {
                    it = obstacle.blocks.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (CheckCollisionRecs(alien.GetRect(), spaceship.GetRect())) {
            GameOver();
        }
    }
}

void Game::GameOver() {
    run = false;
}

void Game::Reset() {
    spaceship.Reset();
    aliens.clear();
    alien_lasers.clear();
    obstacles.clear();
}

void Game::InitGame() {
    obstacles = CreateObstacles();
    aliens = CreateAliens();
    aliens_direction = 1;
    time_last_alien_fired = 0.0;
    time_last_spawn = 0.0;
    lives = 3;
    score = 0;
    highscore = LoadHighScoreFromFile();
    run = true;
    mystery_ship_spawn_interval = GetRandomValue(10, 20);
}

void Game::CheckForHighScore() {
    if (score > highscore) {
        highscore = score;
        SaveHighScoreToFile(highscore);
    }
}

void Game::SaveHighScoreToFile(int highscore) {
    std::ofstream highscore_file("highscore.txt");
    if (highscore_file.is_open()) {
        highscore_file << highscore;
        highscore_file.close();
    } else {
        std::cerr << "Failed to save highscore to file." << std::endl;
    }
}

int Game::LoadHighScoreFromFile() {
    int loaded_highscore = 0;
    std::ifstream highscore_file("highscore.txt");
    if (highscore_file.is_open()) {
        highscore_file >> loaded_highscore;
        highscore_file.close();
    } else {
        std::cerr << "Failed to load highscore from file." << std::endl;
    }
    return loaded_highscore;
}