#pragma once
#include <vector>
#include <memory>
#include "Ball.h"
#include "Block.h"
#include "Camera.h"
#include "Player.h"
#include "System/ModelRenderer.h"

class BlockManager
{
public:
    BlockManager();
    ~BlockManager();

    void Initialize(Player* player);
    void Update(float elapsedTime, Camera* camera);
    void Render(ModelRenderer* renderer);
    void CheckCollision(Ball* ball);

    const std::vector<std::unique_ptr<Block>>& GetBlocks() const { return blocks; }

private:
    std::vector<std::unique_ptr<Block>> blocks;
};