#include "Enemy.h"
#include "EnemyManager.h"

using namespace DirectX;

EnemyManager::EnemyManager() {}

EnemyManager::~EnemyManager() { m_enemies.clear(); }

void EnemyManager::Initialize(ID3D11Device* device)
{
    for (const auto& config : EnemyLevelData::Spawns)
    {
        SpawnEnemy(config);
    }
}

void EnemyManager::SpawnEnemy(const EnemySpawnConfig& config)
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    const char* modelPath = "";

    // Model Selection based on EnemyType
    if (config.Type == EnemyType::Ball)
    {
        modelPath = "Data/Model/Character/PLACEHOLDER_mdl_Ball.glb";
    }
    else if (config.Type == EnemyType::Pentagon)
    {
        // [GAMEBEYOND] Pentagon has its own model
        modelPath = "Data/Model/Character/PLACEHOLDER_mdl_Pentagon.glb";
    }
    else
    {
        // Default to Paddle
        modelPath = "Data/Model/Character/PLACEHOLDER_mdl_Paddle.glb";
    }

    auto newEnemy = std::make_unique<Enemy>(
        device,
        modelPath,
        config.Position,
        config.Rotation,
        config.Color,
        config.Type,
        config.AttackBehavior,
        config.MinX,
        config.MaxX,
        config.MinZ,
        config.MaxZ,
        config.Direction
    );

    // [GAMEBEYOND] Apply scale if specified
    newEnemy->SetScale(config.Scale);

    m_enemies.push_back(std::move(newEnemy));
}

void EnemyManager::Update(float elapsedTime, Camera* camera, const DirectX::XMFLOAT3& playerPos, bool allowAttack)
{
    for (auto& enemy : m_enemies)
    {
        enemy->Update(elapsedTime, camera);

        // Pass allowAttack parameter to UpdateTracking
        // Default is true, so GameBeyond (which doesn't use this param) will work fine
        enemy->UpdateTracking(elapsedTime, camera, playerPos, allowAttack);
    }
}

void EnemyManager::Render(ModelRenderer* renderer, Camera* camera)
{
    for (auto& enemy : m_enemies)
    {
        // =========================================================
        // FRUSTUM CULLING FOR ENEMY BODY
        // =========================================================
        bool isBodyVisible = true;

        if (camera)
        {
            DirectX::XMFLOAT3 pos = enemy->GetPosition();

            // [GAMEBEYOND] Scale-aware culling radius
            DirectX::XMFLOAT3 scale = enemy->GetScale();
            float maxScale = max(scale.x, max(scale.y, scale.z));

            // Base radius 1.5f, scaled appropriately
            // For Pentagon (scale 150), this becomes 225.0f
            float cullingRadius = 1.5f * maxScale;

            if (!camera->CheckSphere(pos.x, pos.y, pos.z, cullingRadius))
            {
                isBodyVisible = false;
            }
        }

        // =========================================================
        // RENDER ENEMY BODY (only if visible)
        // =========================================================
        if (isBodyVisible)
        {
            renderer->Draw(ShaderId::Phong, enemy->GetModel(), enemy->color);
        }

        // =========================================================
        // RENDER PROJECTILES (always render, even if enemy is off-screen)
        // =========================================================
        // IMPORTANT: Projectiles must always be rendered because they can
        // travel outside the screen boundaries while the enemy is culled
        enemy->RenderProjectiles(renderer);
    }
}

void EnemyManager::RenderDebug(ShapeRenderer* renderer)
{
    for (auto& enemy : m_enemies)
    {
        enemy->RenderDebugProjectiles(renderer);
    }
}

// =========================================================
// [GAMEBREAKER] RESPAWN FUNCTIONALITY
// =========================================================
void EnemyManager::RespawnEnemyAs(size_t index, AttackType attack, MoveDir dir, float minX, float maxX, float minZ, float maxZ)
{
    if (index >= m_enemies.size()) return;

    auto& e = m_enemies[index];

    EnemySpawnConfig config;
    config.Position = e->GetPosition();
    config.Rotation = e->GetRotation();
    config.Color = e->color;
    config.Type = e->GetType();
    config.AttackBehavior = attack;
    config.Direction = dir;
    config.MinX = minX;
    config.MaxX = maxX;
    config.MinZ = minZ;
    config.MaxZ = maxZ;

    SpawnEnemy(config);

    std::swap(m_enemies[index], m_enemies.back());
    m_enemies.pop_back();
}