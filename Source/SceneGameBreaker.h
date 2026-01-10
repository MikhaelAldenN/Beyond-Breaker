#pragma once

#include <memory>
#include <vector>
#include "GameWindow.h"
#include "Scene.h"
#include "Camera.h"
#include "CameraController.h"
#include "Player.h"
#include "Paddle.h"
#include "Ball.h"
#include "BlockManager.h"

class SceneGameBreaker : public Scene
{
public:
    SceneGameBreaker();
    ~SceneGameBreaker() override;

    void Update(float elapsedTime) override;
    void Render(float elapsedTime, Camera* camera = nullptr) override;
    void DrawGUI() override;
    void OnResize(int width, int height) override;

    Camera* GetMainCamera() const { return mainCamera; }
    Camera* GetSubCamera() const { return subCamera; }

private:
    void RenderScene(float elapsedTime, Camera* camera);
    void GenerateBlocks();

    // --- Main Assets ---
    Camera* mainCamera = nullptr;
    Camera* subCamera = nullptr;
    Player* player = nullptr;
    Paddle* paddle = nullptr;
    Ball* ball = nullptr;
    std::unique_ptr<BlockManager> blockManager;
    std::vector<Camera*> additionalCameras;

    // --- Tracking Window (Auto-follows player) ---
    GameWindow* trackingWindow = nullptr;
    Camera* trackingCamera = nullptr;

    // --- Lens Window (Draggable by user) ---
    GameWindow* lensWindow = nullptr;
    Camera* lensCamera = nullptr;
};