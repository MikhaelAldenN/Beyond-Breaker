#include "BlockManager.h"
#include <cmath>

BlockManager::BlockManager()
{
}

BlockManager::~BlockManager()
{
    blocks.clear();
}

void BlockManager::Initialize(Player* player)
{
    blocks.clear();

    int rows = 15;
    int columns = 25;

    float xSpacing = 0.5f;
    float zSpacing = 0.5f;
    float zOffsetWorld = 2.5f;

    float startX = -((columns - 1) * xSpacing) / 2.0f;
    float startZ = -((rows - 1) * zSpacing) / 2.0f;

    int centerCol = columns / 2;
    int centerRow = rows / 2;

    for (int z = 0; z < rows; ++z)
    {
        for (int x = 0; x < columns; ++x)
        {
            // Handle the Hole for the Player
            if (x == centerCol && z == centerRow)
            {
                if (player)
                {
                    float px = startX + (x * xSpacing);
                    float pz = startZ + (z * zSpacing) + zOffsetWorld;
                    player->GetMovement()->SetPosition({ px, 0, pz });
                }
                continue; 
            }

            // Create the Block
            auto newBlock = std::make_unique<Block>();

            float posX = startX + (x * xSpacing);
            float posZ = startZ + (z * zSpacing) + zOffsetWorld;

            newBlock->GetMovement()->SetPosition({ posX, 0.0f, posZ });

            blocks.push_back(std::move(newBlock));
        }
    }
}

void BlockManager::Update(float elapsedTime, Camera* camera)
{
    for (auto& block : blocks)
    {
        if (block->IsActive())
        {
            block->Update(elapsedTime, camera);
        }
    }
}

void BlockManager::Render(ModelRenderer* renderer)
{
    for (auto& block : blocks)
    {
        if (block->IsActive())
        {
            block->Render(renderer);
        }
    }
}

void BlockManager::CheckCollision(Ball* ball)
{
    if (!ball || !ball->IsActive()) return;

    DirectX::XMFLOAT3 ballPos = ball->GetMovement()->GetPosition();
    DirectX::XMFLOAT3 ballVel = ball->GetVelocity();

    float ballRadius = 0.1f;
    float blockHalfSize = 0.3f;

    for (auto& block : blocks)
    {
        // Skip dead blocks
        if (!block->IsActive()) continue;

        DirectX::XMFLOAT3 blockPos = block->GetMovement()->GetPosition();

        float deltaX = ballPos.x - blockPos.x;
        float deltaZ = ballPos.z - blockPos.z;

        float absX = std::abs(deltaX);
        float absZ = std::abs(deltaZ);

        float overlapX = (blockHalfSize + ballRadius) - absX;
        float overlapZ = (blockHalfSize + ballRadius) - absZ;

        if (overlapX > 0 && overlapZ > 0)
        {
            if (overlapX < overlapZ)
            {
                ballVel.x *= -1.0f;
            }
            else
            {
                ballVel.z *= -1.0f;
            }

            ball->SetVelocity(ballVel);
            block->OnHit();

            return;
        }
    }
}