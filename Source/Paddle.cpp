#include "Paddle.h"
#include "System/Graphics.h"
#include <Windows.h> 

using namespace DirectX;

Paddle::Paddle()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    model = std::make_shared<Model>(device, "Data/Model/Character/PLACEHOLDER_mdl_Paddle.glb");

    movement->SetPosition({ 0.0f, 0.0f, -4.0f });
}

Paddle::~Paddle()
{
}

void Paddle::Update(float elapsedTime, Camera* camera)
{
    HandleInput();

    movement->SetRotationY(0.0f);

    XMFLOAT3 pos = movement->GetPosition();
    XMFLOAT3 vel = movement->GetVelocity();

    pos.x += vel.x * elapsedTime;

    if (pos.x > xLimit) pos.x = xLimit;
    if (pos.x < -xLimit) pos.x = -xLimit;

    pos.y = 0.0f;
    pos.z = -4.0f;

    movement->SetPosition(pos);

    SyncData();
}

void Paddle::HandleInput()
{
    float xInput = 0.0f;

    if (GetAsyncKeyState('D') & 0x8000) xInput += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) xInput -= 1.0f;

    float velocityX = xInput * paddleSpeed;

    movement->SetMoveInput(xInput, 0.0f);
}