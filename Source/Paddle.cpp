#include "Paddle.h"
#include "System/Graphics.h"
#include <algorithm>
#include <cmath>
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
    if (!isAIEnabled)
    {
        HandleInput();
    }

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

void Paddle::UpdateAI(float elapsedTime, Ball* ball)
{
    if (!isAIEnabled || !ball) return;

    if (!ball->IsActive())
    {
        launchTimer += elapsedTime;

        if (launchTimer >= 1.0f)
        {
            float randX = ((float)(rand() % 100) / 100.0f) * 2.0f - 1.0f; 

            ball->SetVelocity({ randX, 0.0f, 8.0f });
            ball->SetActive(true);

            launchTimer = 0.0f; 
        }
        return;
    }
    else
    {
        launchTimer = 0.0f;
    }

    XMFLOAT3 ballPos = ball->GetPosition();
    XMFLOAT3 myPos = movement->GetPosition();

    float diffX = ballPos.x - myPos.x;
    float velocityX = 0.0f;
    float safeZone = 0.5f;

    if (fabs(diffX) > safeZone)
    {
        if (diffX > 0) velocityX = paddleSpeed;
        else velocityX = -paddleSpeed;
    }
    movement->SetMoveInput(velocityX, 0.0f);
}

void Paddle::HandleInput()
{
    if (isAIEnabled) return;

    float xInput = 0.0f;
    if (GetAsyncKeyState('D') & 0x8000) xInput += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) xInput -= 1.0f;

    float velocityX = xInput * paddleSpeed;
    movement->SetMoveInput(velocityX, 0.0f);
}

void Paddle::CheckCollision(Ball* ball)
{
    if (!ball) return;

    auto ballPos = ball->GetPosition();
    auto padPos = movement->GetPosition();

    // Define Hitboxes
    float ballRadius = 0.1f;
    float paddleWidthHalf = 0.8f;
    float paddleDepthHalf = 0.1f;

    // Collision Logic 
    float zDist = fabs(ballPos.z - padPos.z);
    if (zDist < (paddleDepthHalf + ballRadius))
    {
        float xDist = fabs(ballPos.x - padPos.x);
        if (xDist < (paddleWidthHalf + ballRadius))
        {
            XMVECTOR vVel = XMLoadFloat3(&ball->GetVelocity());
            float speed = XMVectorGetX(XMVector3Length(vVel));
            float relativeIntersectX = (ballPos.x - padPos.x) / paddleWidthHalf;
            float maxBounceAngle = 1.3f;
            float bounceAngle = relativeIntersectX * maxBounceAngle;

            float newVx = speed * sinf(bounceAngle);
            float newVz = speed * cosf(bounceAngle);

            newVz = fabsf(newVz);

            ball->SetVelocity({ newVx, 0.0f, newVz });
        }
    }
}