#pragma once

#include "../Screen.h"
#include <glm/glm.hpp>

class GameplayScreen : public Screen {
public:
    GameplayScreen();
    ~GameplayScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;

private:
    // Any gameplay specific data
};