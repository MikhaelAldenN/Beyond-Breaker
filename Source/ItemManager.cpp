#include "ItemManager.h"
#include <cmath>

using namespace DirectX;

ItemManager::ItemManager() {}
ItemManager::~ItemManager() { m_items.clear(); }

void ItemManager::Initialize(ID3D11Device* device, bool skipLevelSpawns)
{
    m_deviceRef = device;
    m_items.clear();

    // =========================================================
    // [BARU] Conditional spawn dari level data
    // =========================================================
    if (!skipLevelSpawns)
    {
        // SceneGameBreaker: Spawn semua item dari level data
        for (const auto& data : ItemLevelData::Spawns)
        {
            SpawnItem(data);
        }
    }
    // SceneGameBeyond: skipLevelSpawns = true, jadi tidak spawn apapun
    // Item hanya spawn on-demand saat enemy mati
}

void ItemManager::SpawnItem(const ItemSpawnData& data)
{
    if (!m_deviceRef) return;

    auto newItem = std::make_unique<Item>(m_deviceRef, data.Position, data.Type);
    newItem->SetRotation(data.Rotation);
    newItem->scale = data.Scale;
    if (data.Type == ItemType::Heal)
    {
        newItem->color = { 1.0f, 0.89f, 0.58f, 1.0f };
    }
    else if (data.Type == ItemType::Invincible)
    {
        newItem->color = { 0.5f, 0.5f, 0.5f, 1.0f };
    }
    m_items.push_back(std::move(newItem));
}

void ItemManager::SpawnHealAt(const DirectX::XMFLOAT3& position)
{
    ItemSpawnData data;
    data.Type = ItemType::Heal;

    data.Position = position;
    data.Position.y = 0.4f;

    data.Rotation = { 0.0f, 0.0f, 0.0f };
    data.Scale = { 2.0f, 2.0f, 2.0f };

    SpawnItem(data);
}

void ItemManager::SpawnHealClusterAt(const DirectX::XMFLOAT3& position, int count)
{
    if (count <= 0) return;

    const float SPREAD_RADIUS = 1.2f;
    int clusterId = m_nextClusterId++;  // Satu ID untuk seluruh batch ini

    for (int i = 0; i < count; ++i)
    {
        float angle = (DirectX::XM_2PI / (float)count) * (float)i;

        ItemSpawnData data;
        data.Type = ItemType::Heal;
        data.Position = position;
        data.Position.x += sinf(angle) * SPREAD_RADIUS;
        data.Position.z += cosf(angle) * SPREAD_RADIUS;
        data.Position.y = 0.4f;
        data.Rotation = { 0.0f, 0.0f, 0.0f };
        data.Scale = { 2.0f, 2.0f, 2.0f };

        SpawnItem(data);

        // Catat index item yang baru di-push ke cluster map
        int newIdx = (int)m_items.size() - 1;
        m_itemClusterMap[newIdx] = clusterId;
    }
}

void ItemManager::AddItem(ItemType type)
{
    ItemSpawnData defaultData;
    defaultData.Type = type;

    if (!m_items.empty())
    {
        defaultData.Position = m_items.back()->GetBasePosition();
        defaultData.Rotation = { 0, 0, 0 };
        defaultData.Scale = m_items.back()->scale;
    }
    else
    {
        defaultData.Position = { 0, 0.4f, 0 };
        defaultData.Rotation = { 0, 0, 0 };
        defaultData.Scale = { 2.0f, 2.0f, 2.0f };
    }
    SpawnItem(defaultData);
}

void ItemManager::Update(float elapsedTime, Camera* camera)
{
    const float TRACK_SPEED = 5.0f;  // unit/detik  seberapa cepat item mendekat

    for (size_t i = 0; i < m_items.size(); ++i)
    {
        auto& item = m_items[i];

        // === TRACKING: Heal item aktif bergerak menuju player ===
        if (m_trackingEnabled && item->IsActive() && item->GetType() == ItemType::Heal)
        {
            DirectX::XMFLOAT3 cur = item->GetPosition();
            float dx = m_trackTarget.x - cur.x;
            float dz = m_trackTarget.z - cur.z;
            float distSq = dx * dx + dz * dz;

            if (distSq > 0.01f)  // Masih jauh dari target
            {
                float dist = sqrtf(distSq);
                float step = TRACK_SPEED * elapsedTime;

                if (step >= dist)
                {
                    // Snap langsung ke target kalau langkah > sisa jarak
                    cur.x = m_trackTarget.x;
                    cur.z = m_trackTarget.z;
                }
               
                {
                    // Geser proporsional ke arah target
                    cur.x += (dx / dist) * step;
                    cur.z += (dz / dist) * step;
                }

                item->SetPosition({ cur.x, 0.4f, cur.z });
            }
        }

        item->Update(elapsedTime, camera);
    }
}

void ItemManager::Render(ModelRenderer* renderer)
{
    for (auto& item : m_items)
    {
        item->Render(renderer);
    }
}

void ItemManager::RenderDebug(ShapeRenderer* renderer)
{
    if (m_debugHighlightIndex >= 0 && m_debugHighlightIndex < m_items.size())
    {
        auto& item = m_items[m_debugHighlightIndex];
        if (item && item->IsActive())
        {
            XMFLOAT3 pos = item->GetPosition();
            XMFLOAT3 boxSize;
            boxSize.x = (item->scale.x * 0.5f) + 0.05f; // Half scale + Tiny Padding
            boxSize.y = (item->scale.y * 0.5f) + 0.05f;
            boxSize.z = (item->scale.z * 0.5f) + 0.05f;

            // [FIXED] Pass 'boxSize', not 'scale'
            renderer->DrawBox(pos, item->GetRotation(), boxSize, { 1.0f, 1.0f, 0.0f, 1.0f });
        }
    }
}

void ItemManager::ResetAllAnimations()
{
    for (auto& item : m_items) { if (item) item->ResetAnimation(); }
}