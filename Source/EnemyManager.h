#pragma once

#include <DirectXMath.h> 
#include <memory>
#include <vector>
#include "System/Graphics.h"

class Enemy;
class ShapeRenderer;

enum class EnemyType
{
    Paddle,
    Ball,
    Pentagon // [GAMEBEYOND] Added for special enemy type
};

enum class AttackType
{
    None,
    Static,
    Tracking,
    TrackingHorizontal,
    TrackingRandom,
    RadialBurst // [GAMEBEYOND] Added for Pentagon's 360-degree attack
};

enum class MoveDir
{
    None,
    Left,
    Right
};

struct EnemySpawnConfig
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Rotation;
    DirectX::XMFLOAT4 Color;
    EnemyType Type = EnemyType::Paddle;
    AttackType AttackBehavior = AttackType::None;
    MoveDir Direction = MoveDir::None;
    float MinX = 0.0f;
    float MaxX = 0.0f;
    float MinZ = 0.0f;
    float MaxZ = 0.0f;

    DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f }; // [GAMEBEYOND] Scale support
};

namespace EnemyLevelData
{
    // ==========================================
    // COLOR PRESETS
    // ==========================================
    static const DirectX::XMFLOAT4 Blue = { 0.0f, 0.0f, 0.8f, 1.0f };
    static const DirectX::XMFLOAT4 Yellow = { 0.96f, 0.80f, 0.23f, 1.0f };

    // ==========================================
    // ROTATION PRESETS
    // ==========================================
    namespace Rot
    {
        static const DirectX::XMFLOAT3 Backward = { 0.0f, 0.0f, 0.0f };
        static const DirectX::XMFLOAT3 Forward = { 0.0f, DirectX::XM_PI, 0.0f };
        static const DirectX::XMFLOAT3 Left = { 0.0f, DirectX::XM_PIDIV2, 0.0f };
        static const DirectX::XMFLOAT3 Right = { 0.0f, -DirectX::XM_PIDIV2, 0.0f };
    }

    // ==========================================
    // MASTER SPAWN LIST
    // ==========================================
    // NOTE: This is scene-specific. Each scene should define its own spawn list.
    // GameBreaker has its own configuration in its branch
    // GameBeyond has its own configuration in its branch
    static const std::vector<EnemySpawnConfig> Spawns =
    {
        // Example spawn - actual spawns should be defined per-scene
        { { 0.0f, 0.0f, -50.0f }, Rot::Backward, Blue, EnemyType::Paddle, AttackType::Tracking },
    };
}

class EnemyManager
{
public:
    EnemyManager();
    ~EnemyManager();

    void Initialize(ID3D11Device* device);

    // [COMPATIBILITY] Support both signatures
    // GameBreaker uses: Update(elapsedTime, camera, playerPos, allowAttack)
    // GameBeyond uses: Update(elapsedTime, camera, playerPos)
    void Update(float elapsedTime, Camera* camera, const DirectX::XMFLOAT3& playerPos, bool allowAttack = true);

    void Render(ModelRenderer* renderer, Camera* camera = nullptr);
    void RenderDebug(ShapeRenderer* renderer);
    void SpawnEnemy(const EnemySpawnConfig& config);

    // [GAMEBREAKER] Additional functionality
    void RespawnEnemyAs(size_t index, AttackType attack, MoveDir dir, float minX, float maxX, float minZ, float maxZ);

    std::vector<std::unique_ptr<Enemy>>& GetEnemies() { return m_enemies; }

private:
    std::vector<std::unique_ptr<Enemy>> m_enemies;
};