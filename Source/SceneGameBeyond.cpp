#include <SDL3/SDL.h>
#include <cmath>
#include <windows.h>
#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGameBeyond.h"
#include "WindowManager.h"
#include "Framework.h"
#include "Enemy.h"
#include <unordered_set>
#include <imgui.h>

using namespace DirectX;

// Definisi Konstanta Lokal
#define FIELD_OF_VIEW 60.0f
#define DEFERRED_INIT_TIME 0.2f
#define CACHE_REFRESH_INTERVAL 1.0f
#define PRIORITY_ENFORCE_INTERVAL 2.0f
#define PIXEL_TO_UNIT_RATIO 40.0f

// =========================================================
// CONSTRUCTOR
// =========================================================
SceneGameBeyond::SceneGameBeyond()
{
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    float screenW = 1920.0f;
    float screenH = 1080.0f;

    // 0. Setup Window System
    m_windowSystem = std::make_unique<WindowTrackingSystem>();
    m_windowSystem->SetPixelToUnitRatio(PIXEL_TO_UNIT_RATIO);
    m_windowSystem->SetFOV(FIELD_OF_VIEW);

    float unifiedHeight = m_windowSystem->GetUnifiedCameraHeight();

    // 1. Setup Main Camera
    m_mainCamera = std::make_shared<Camera>();
    m_mainCamera->SetPerspectiveFov(XMConvertToRadians(FIELD_OF_VIEW), screenW / screenH, 0.1f, 1000.0f);
    m_mainCamera->SetPosition(0.0f, unifiedHeight, 0.0f);
    m_mainCamera->LookAt({ 0, 0, 0 });

    CameraController::Instance().SetActiveCamera(m_mainCamera);
    CameraController::Instance().SetControlMode(CameraControlMode::FixedStatic);
    CameraController::Instance().SetFixedSetting(XMFLOAT3(0.0f, unifiedHeight, 0.0f));

    // 2. Setup Assets
    m_player = std::make_unique<Player>();
    m_player->SetInvertControls(true);
    m_player->SetPosition(0.0f, 0.0f, -8.0f);

    m_blockManager = std::make_unique<BlockManager>();
    m_blockManager->Initialize(m_player.get());
    m_blockManager->ClearBlocks();
    m_blockManager->ActivateFormationMode();

    m_blockManager->shieldSettings.Enabled = true;
    m_blockManager->shieldSettings.MaxTetherDistance = 8.0f;
    m_blockManager->shootSettings.ProjectileSpeed = 20.0f;

    for (int i = 0; i < 20; ++i) m_blockManager->SpawnAllyBlock(m_player.get());

    m_boss = std::make_unique<Boss>();

    if (m_boss && m_player)
    {
        m_boss->SetPlayer(m_player.get());

        // [BARU] Trigger window shatter on first boss damage
        m_boss->SetOnFirstDamageCallback([this]() {
            if (!m_shatterTriggered)
            {
                m_shatterTriggered = true;
                m_gameStarted = true;

                // Trigger window shatter effect
                WindowShatterManager::Instance().TriggerShatter();

                // [BARU] Start boss AI - transition from Intro to Idle state
                m_boss->TriggerIdle();

                // Log untuk debugging
                m_boss->AddTerminalLog(">> BATTLE START <<");
            }
            });
    }

    m_enemyManager = std::make_unique<EnemyManager>();

    // 3. Sub Camera
    m_subCamera = std::make_shared<Camera>();
    m_subCamera->SetPerspectiveFov(XMConvertToRadians(60), 4.0f / 3.0f, 0.1f, 1000.0f);
    m_subCamera->SetPosition(5, 5, 5);
    m_subCamera->LookAt({ 0, 0, 0 });

    // 4. Setup Primitive Renderer
    auto device = Graphics::Instance().GetDevice();
    m_primitive2D = std::make_unique<Primitive>(device);

    if (m_boss && m_enemyManager)
    {
        m_boss->SetEnemyManager(m_enemyManager.get());
    }

    // =========================================================
    //  MATIKAN ITEM MANAGER INITIALIZATION
    // =========================================================
    m_itemManager = std::make_unique<ItemManager>();
    m_itemManager->Initialize(Graphics::Instance().GetDevice(), true);

    // [BARU] 2. Setup Collision Manager
    m_collisionManager = std::make_unique<CollisionManager>();

    // [UPDATE] Tambahkan m_boss.get() di parameter terakhir
    // CollisionManager butuh ItemManager pointer buat SpawnHealClusterAt
    m_collisionManager->Initialize(
        m_player.get(),
        nullptr,             // Stage null
        m_blockManager.get(),
        m_enemyManager.get(),
        m_itemManager.get(),  // ItemManager pointer buat item drop
        m_boss.get()
    );

    // [OPSIONAL] Callback efek visual saat Player kena Hit
    m_collisionManager->SetOnPlayerHitCallback([this]() {
        if (m_player) {
            auto pPos = m_player->GetPosition();
            WindowShatterManager::Instance().TriggerExplosion({ pPos.x, pPos.z }, 4);
        }
        });

    // =========================================================
    // [BARU] INITIALIZE SHATTER POOL
    // Create pooled shatter windows di belakang main window
    // =========================================================
    WindowShatterManager::Instance().InitializeShatterPool(8);

    // Set reference ke main window untuk destroy nanti
    GameWindow* mainWin = Framework::Instance()->GetMainWindow();
    if (mainWin)
    {
        WindowShatterManager::Instance().SetMainWindow(mainWin);
    }
}

// =========================================================
// DESTRUCTOR
// =========================================================
SceneGameBeyond::~SceneGameBeyond()
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    WindowShatterManager::Instance().Clear();
    CameraController::Instance().ClearCamera();
}

// =========================================================
// INITIALIZE SUB WINDOWS
// =========================================================
void SceneGameBeyond::InitializeSubWindows()
{
    if (!m_windowSystem || !m_player || !m_boss) return;

    // 1. Player Tracking Window
    m_windowSystem->AddTrackedWindow(
        { "player", "Player View", 300, 300, 1, { 0.0f, 0.0f, 0.0f } },

        // 1. LAMBDA POSISI (Center)
        [this]() -> DirectX::XMFLOAT3 {
            if (!m_player) return DirectX::XMFLOAT3(0, 0, 0);

            auto pPos = m_player->GetPosition();

            // Safe Zone 150px
            float safeMarginPixel = 150.0f;
            float safeMarginWorld = safeMarginPixel / PIXEL_TO_UNIT_RATIO;

            float minX = pPos.x - safeMarginWorld;
            float maxX = pPos.x + safeMarginWorld;
            float minZ = pPos.z - safeMarginWorld;
            float maxZ = pPos.z + safeMarginWorld;

            if (m_blockManager && m_blockManager->IsShieldActive())
            {
                const auto& blocks = m_blockManager->GetBlocks();
                for (const auto& block : blocks)
                {
                    // TAMBAHAN: && !block->IsProjectile()
                    // Jangan hitung blok yang sedang ditembakkan!
                    if (block && block->IsActive() && !block->IsProjectile())
                    {
                        auto bPos = block->GetMovement()->GetPosition();
                        if (bPos.x < minX) minX = bPos.x;
                        if (bPos.x > maxX) maxX = bPos.x;
                        if (bPos.z < minZ) minZ = bPos.z;
                        if (bPos.z > maxZ) maxZ = bPos.z;
                    }
                }
            }

            float centerX = (minX + maxX) * 0.5f;
            float centerZ = (minZ + maxZ) * 0.5f;

            return DirectX::XMFLOAT3(centerX, 0.0f, centerZ);
        },

        // 2. LAMBDA UKURAN (Size)
        [this]() -> DirectX::XMFLOAT2 {
            if (!m_player) return DirectX::XMFLOAT2(300, 300);

            auto pPos = m_player->GetPosition();
            float safeMarginPixel = 150.0f;
            float safeMarginWorld = safeMarginPixel / PIXEL_TO_UNIT_RATIO;

            float minX = pPos.x - safeMarginWorld;
            float maxX = pPos.x + safeMarginWorld;
            float minZ = pPos.z - safeMarginWorld;
            float maxZ = pPos.z + safeMarginWorld;

            if (m_blockManager && m_blockManager->IsShieldActive())
            {
                const auto& blocks = m_blockManager->GetBlocks();
                for (const auto& block : blocks)
                {
                    // TAMBAHAN: && !block->IsProjectile()
                    // Hanya hitung blok yang menempel di shield/formasi
                    if (block && block->IsActive() && !block->IsProjectile())
                    {
                        auto bPos = block->GetMovement()->GetPosition();
                        if (bPos.x < minX) minX = bPos.x;
                        if (bPos.x > maxX) maxX = bPos.x;
                        if (bPos.z < minZ) minZ = bPos.z;
                        if (bPos.z > maxZ) maxZ = bPos.z;
                    }
                }
            }

            float rangeX = maxX - minX;
            float rangeZ = maxZ - minZ;
            float extraPadding = 50.0f;

            float w = (rangeX * PIXEL_TO_UNIT_RATIO) + extraPadding;
            float h = (rangeZ * PIXEL_TO_UNIT_RATIO) + extraPadding;

            return DirectX::XMFLOAT2(w, h);
        }
    );

    // 2. Boss Monitor 1 (Main Head)
    if (m_boss->HasPart("monitor1"))
    {
        m_windowSystem->AddTrackedWindow(
            { "monitor1", "Boss Monitor", 340, 340, 0, { -0.3f, 0.0f, 2.1f } },
            [this]() -> DirectX::XMFLOAT3 {
                if (m_boss) {
                    auto pos = m_boss->GetMonitorVisualPos();
                    return XMFLOAT3{ pos.x, 0.0f, pos.z };
                }
                return DirectX::XMFLOAT3(0, 0, 0);
            }
        );
    }

    // 3. CPU (Body)
    if (m_boss->HasPart("cpu"))
    {
        m_windowSystem->AddTrackedWindow(
            { "cpu", "System Unit", 186, 370, 0, { -8.2f, 0.0f, 4.0f } },
            [this]() -> DirectX::XMFLOAT3 {
                if (m_boss) {
                    auto pos = m_boss->GetCPUVisualPos();
                    return XMFLOAT3{ pos.x, 0.0f, pos.z };
                }
                return DirectX::XMFLOAT3(0, 0, 0);
            }
        );
    }

    // 4. Monitor 2 (Side Left)
    if (m_boss->HasPart("monitor2"))
    {
        m_windowSystem->AddTrackedWindow(
            { "monitor2", "Side Monitor L", 240, 210, 4, { 0.5f, 0.0f, -0.3f } },
            [this]() -> DirectX::XMFLOAT3 {
                if (m_boss) {
                    auto pos = m_boss->GetMonitor2VisualPos();
                    return XMFLOAT3{ pos.x, 0.0f, pos.z };
                }
                return DirectX::XMFLOAT3(0, 0, 0);
            }
        );
    }

    // 5. Monitor 3 (Side Right)
    if (m_boss->HasPart("monitor3"))
    {
        m_windowSystem->AddTrackedWindow(
            { "monitor3", "Side Monitor R", 200, 200, 3, { 0.8f, 0.0f, 1.2f } },
            [this]() -> DirectX::XMFLOAT3 {
                if (m_boss) {
                    auto pos = m_boss->GetMonitor3VisualPos();
                    return XMFLOAT3{ pos.x, 0.0f, pos.z };
                }
                return DirectX::XMFLOAT3(0, 0, 0);
            }
        );
    }

    if (m_boss->HasPart("antenna"))
    {
        m_windowSystem->AddTrackedWindow(
            // Config Dasar (Size dasar 220x500)
            { "antenna", "Signal Uplink", 220, 500, 5, { 0.0f, 0.0f, 0.0f } },

            // 1. LAMBDA POSISI (Model Pos + Offset ImGui)
            [this]() -> DirectX::XMFLOAT3 {
                if (!m_boss) return { 0,0,0 };

                // Ambil posisi asli model (yang sedang animasi slide)
                auto pos = m_boss->GetAntennaVisualPos();

                bool isMirrored = (pos.x < -1.0f);

                if (isMirrored)
                {
                    // === SETTING KHUSUS MIRRORED (KIRI) ===
                    // Geser ke kanan dikit (X +) agar pas
                    pos.x += 1.8f; // <--- UBAH ANGKA INI (Geser Kanan)

                    // Kamu juga bisa atur Y atau Z beda kalau perlu
                    pos.y += m_antennaTrackOffset.y;
                    pos.z += m_antennaTrackOffset.z;
                }
                else
                {
                    // === SETTING NORMAL (KANAN) ===
                    // Pakai settingan standar (bisa dari Slider ImGui)
                    pos.x += m_antennaTrackOffset.x;
                    pos.y += m_antennaTrackOffset.y;
                    pos.z += m_antennaTrackOffset.z;
                }
                return { pos.x, pos.y, pos.z };
            },

            // 2. LAMBDA SIZE (Base Size + Offset ImGui)
            [this]() -> DirectX::XMFLOAT2 {
                float baseW = 220.0f;
                float baseH = 500.0f;

                // Tambahkan offset, tapi jangan sampai negatif
                float w = max(50.0f, baseW + m_antennaSizeOffset.x);
                float h = max(50.0f, baseH + m_antennaSizeOffset.y);

                return DirectX::XMFLOAT2(w, h);
            }
        );
    }



    WindowManager::Instance().EnforceWindowPriorities();
    m_isWindowsInitialized = true;
}

void SceneGameBeyond::UpdateEnemyWindows()
{
    if (!m_enemyManager || !m_windowSystem) return;

    const auto& enemies = m_enemyManager->GetEnemies();
    size_t enemyCount = enemies.size();

    // === PASS 1: SPAWN WINDOW BARU ===
    for (size_t i = 0; i < enemyCount; ++i)
    {
        std::string winID = "enemy_view_" + std::to_string(i);

        if (m_windowSystem->GetTrackedWindow(winID) == nullptr)
        {
            const size_t idx = i;
            EnemyManager* rawEM = m_enemyManager.get(); // Capture raw pointer (aman karena scene masih hidup)

            m_windowSystem->AddTrackedWindow(
                { winID, "Enemy Signal " + std::to_string(i), 150, 150, 2, { 0.0f, 0.0f, 0.0f } },

                [rawEM, idx, this]() -> DirectX::XMFLOAT3 {
                    const auto& curEnemies = rawEM->GetEnemies();
                    if (idx >= curEnemies.size() || !curEnemies[idx])
                        return { 0, 0, 0 };

                    auto pos = curEnemies[idx]->GetPosition();
                    pos.x += m_enemyTrackOffset.x;
                    pos.y += m_enemyTrackOffset.y;
                    pos.z += m_enemyTrackOffset.z;
                    return pos;
                },

                [this]() -> DirectX::XMFLOAT2 {
                    float w = 150 + m_enemySizeOffset.x;
                    float h = 150 + m_enemySizeOffset.y;
                    return DirectX::XMFLOAT2(max(50.0f, w), max(50.0f, h));
                }
            );
        }
    }

    // === PASS 2: CLEANUP ===
    static size_t lastEnemyCount = 0;

    if (enemyCount < lastEnemyCount)
    {
        for (size_t i = enemyCount; i < lastEnemyCount + 2; ++i)
        {
            std::string winID = "enemy_view_" + std::to_string(i);
            if (m_windowSystem->GetTrackedWindow(winID) != nullptr)
            {
                m_windowSystem->RemoveTrackedWindow(winID);
            }
        }
    }

    lastEnemyCount = enemyCount;
}
void SceneGameBeyond::UpdateItemWindows()
{
    if (!m_itemManager || !m_windowSystem) return;

    const auto& items = m_itemManager->GetItems();
    const auto& clusterMap = m_itemManager->GetClusterMap();

    // =========================================================
    // STEP 1: Bangun set dari cluster ID yang MASIH punya item aktif
    // =========================================================
    std::unordered_set<int> activeClusters;

    for (const auto& [itemIdx, cid] : clusterMap)
    {
        if (itemIdx < (int)items.size() && items[itemIdx] && items[itemIdx]->IsActive())
        {
            activeClusters.insert(cid);
        }
    }

    // =========================================================
    // STEP 2: Spawn window untuk cluster baru yang belum punya window
    // =========================================================
    for (int cid : activeClusters)
    {
        std::string winID = "item_cluster_" + std::to_string(cid);

        if (m_windowSystem->GetTrackedWindow(winID) != nullptr) continue; // Sudah ada

        int capturedCid = cid;

        m_windowSystem->AddTrackedWindow(
            { winID, "ITEM", 150, 150, 1, { 0.0f, 0.0f, 0.0f } },

            // === POSISI: Centroid dari semua item aktif di cluster ini ===
            [this, capturedCid]() -> DirectX::XMFLOAT3 {
                if (!m_itemManager) return { 0, 0, 0 };

                const auto& items = m_itemManager->GetItems();
                const auto& clusterMap = m_itemManager->GetClusterMap();

                float sumX = 0, sumZ = 0;
                int   count = 0;

                for (const auto& [idx, cid] : clusterMap)
                {
                    if (cid != capturedCid) continue;
                    if (idx >= (int)items.size() || !items[idx] || !items[idx]->IsActive()) continue;

                    auto pos = items[idx]->GetPosition();
                    sumX += pos.x;
                    sumZ += pos.z;
                    count++;
                }

                if (count == 0) return { 0, -1000, 0 }; // Semua item sudah diambil

                return { sumX / count, 0.0f, sumZ / count };
            },

            // === UKURAN: Tetap ===
            []() -> DirectX::XMFLOAT2 { return { 150, 150 }; }
        );
    }

    // =========================================================
    // STEP 3: Cleanup — hapus window dari cluster yang sudah habis
    // =========================================================
    std::vector<std::string> toRemove;
    const auto& windows = m_windowSystem->GetWindows();

    for (const auto& win : windows)
    {
        if (win->name.find("item_cluster_") != 0) continue;

        try {
            int cid = std::stoi(win->name.substr(13)); // "item_cluster_" = 13 char

            if (activeClusters.find(cid) == activeClusters.end())
            {
                toRemove.push_back(win->name);
            }
        }
        catch (...) {}
    }

    for (const auto& name : toRemove)
    {
        m_windowSystem->RemoveTrackedWindow(name);
    }
}

// =========================================================
// UPDATE LOOP
// =========================================================
void SceneGameBeyond::Update(float elapsedTime)
{
    m_startupTimer += elapsedTime;
    // 2. Deferred window initialization
    if (!m_isWindowsInitialized)
    {
        if (m_startupTimer > DEFERRED_INIT_TIME)
        {
            InitializeSubWindows();
        }
        return;
    }

    // 3. Update Managers
    WindowShatterManager::Instance().Update(elapsedTime);

    // 4. Game Logic
    Camera* activeCam = CameraController::Instance().GetActiveCamera().get();

    DirectX::XMFLOAT3 mousePos = GetMouseOnGround(activeCam);


    // One-time: force spawn position before first Update overwrites it
    static bool s_playerSpawnApplied = false;
    if (!s_playerSpawnApplied && m_player)
    {
        m_player->SetPosition(0.0f, 0.0f, -8.0f);
        s_playerSpawnApplied = true;
    }
    if (m_player)
    {
        m_player->Update(elapsedTime, activeCam);

        DirectX::XMFLOAT3 pos = m_player->GetPosition();

        // --- SCREEN BOUNDARY COLLISION (DYNAMIC) ---
        // Gunakan variabel member m_screenLimitX dan m_screenLimitZ

        // Clamp X (Kiri Kanan)
        if (pos.x > m_screenLimitX)  pos.x = m_screenLimitX;
        if (pos.x < -m_screenLimitX) pos.x = -m_screenLimitX;

        // Clamp Z (Atas Bawah)
        if (pos.z > m_screenLimitZ)  pos.z = m_screenLimitZ;
        if (pos.z < -m_screenLimitZ) pos.z = -m_screenLimitZ;

        // Anti-Fall
        if (pos.y < 0.0f) pos.y = 0.0f;

        m_player->SetPosition(pos.x, pos.y, pos.z);

        if (m_subCamera) m_subCamera->LookAt(m_player->GetPosition());
    }

    // 1. Pre-shatter: detect first hit on monitor1 (block OR projectile) -> trigger shatter
    // Gated behind m_startupTimer so positions are valid before we start checking.
    if (!m_gameStarted && m_boss && m_blockManager && m_startupTimer > 1.5f)
    {
        BossPart* mon1 = m_boss->GetPart("monitor1");
        if (mon1)
        {
            XMFLOAT3 mPos = mon1->visualPosition;
            const float hitRadius = 2.5f;
            bool hit = false;

            // --- A. Check shield/formation blocks (non-projectile only) ---
            for (const auto& block : m_blockManager->GetBlocks())
            {
                if (!block || !block->IsActive() || block->IsProjectile()) continue;

                XMFLOAT3 bPos = block->GetMovement()->GetPosition();
                float dx = bPos.x - mPos.x;
                float dy = bPos.y - mPos.y;
                float dz = bPos.z - mPos.z;

                if ((dx * dx + dy * dy + dz * dz) < hitRadius * hitRadius)
                {
                    hit = true;
                    break;
                }
            }

            // --- B. Check shot projectiles (the blocks with IsProjectile == true) ---
            if (!hit)
            {
                for (const auto& block : m_blockManager->GetBlocks())
                {
                    if (!block || !block->IsActive() || !block->IsProjectile()) continue;

                    XMFLOAT3 bPos = block->GetMovement()->GetPosition();
                    float dx = bPos.x - mPos.x;
                    float dy = bPos.y - mPos.y;
                    float dz = bPos.z - mPos.z;

                    if ((dx * dx + dy * dy + dz * dz) < hitRadius * hitRadius)
                    {
                        hit = true;
                        break;
                    }
                }
            }

            // --- C. On hit: shatter + SDL resize + wake boss ---
            if (hit)
            {
                WindowShatterManager::Instance().TriggerExplosion({ mPos.x, mPos.z }, 8);

                GameWindow* mainWin = Framework::Instance()->GetMainWindow();
                if (mainWin)
                {
                    SDL_Window* sdlWin = mainWin->GetSDLWindow();
                    if (sdlWin)
                    {
                        SDL_SetWindowTitle(sdlWin, "DEBUG CONSOLE");
                        SDL_SetWindowSize(sdlWin, 450, 600);
                        SDL_SetWindowPosition(sdlWin, 20, 20);
                        SDL_SetWindowBordered(sdlWin, true);
                        SDL_SetWindowResizable(sdlWin, true);
                    }
                }

                m_gameStarted = true;
                m_shatterTriggered = true;
                m_boss->TriggerIdle();

                OutputDebugStringA("SHATTER TRIGGERED: hit monitor1!\n");
            }
        }
    }

    // 1b. Pre-shatter: keep main window on top of all sub-windows while fullscreen
    if (!m_gameStarted)
    {
        GameWindow* mainWin = Framework::Instance()->GetMainWindow();
        if (mainWin)
        {
            SDL_Window* sdlWin = mainWin->GetSDLWindow();
            if (sdlWin) SDL_RaiseWindow(sdlWin);
        }
    }

    // --- INTEGRASI SHOOT & SHIELD ---
    if (m_blockManager && m_player)
    {
        // Deteksi Input (Menggunakan WinAPI GetKeyState agar responsif untuk hold)
        // VK_LSHIFT = Tameng, VK_SPACE = Tembak
        bool isShielding = (GetKeyState(VK_LSHIFT) & 0x8000);
        bool isShooting = (GetKeyState(VK_SPACE) & 0x8000);

        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

        // Panggil fungsi logika di BlockManager
        m_blockManager->UpdateShieldLogic(isShielding, mousePos, playerPos, elapsedTime);
        m_blockManager->UpdateShootLogic(isShooting, mousePos, playerPos, elapsedTime);

        // Update biasa
        m_blockManager->Update(elapsedTime, activeCam, m_player.get());

        // Anti-fall untuk blocks (Kode sebelumnya)
        auto& blocks = m_blockManager->GetBlocks();
        for (auto& block : blocks) {
            if (block && block->IsActive()) {
                auto movement = block->GetMovement();
                DirectX::XMFLOAT3 bPos = movement->GetPosition();

                // JIKA JATUH ATAU BERADA DI BAWAH 0
                if (bPos.y < 0.0f)
                {
                    // 1. Reset Posisi ke 0
                    movement->SetPosition({ bPos.x, 0.0f, bPos.z });

                    // 2. HENTIKAN KECEPATAN JATUH (PENTING BIAR GAK JITTER)
                    DirectX::XMFLOAT3 vel = movement->GetVelocity();
                    if (vel.y < 0) {
                        // Nol-kan kecepatan Y, tapi biarkan X dan Z (biar tetap bisa geser)
                        movement->SetVelocity({ vel.x, 0.0f, vel.z });
                    }
                }
            }
        }

        // Debug Spawn manual (tombol R)
        if (Input::Instance().GetKeyboard().IsTriggered('R'))
            m_blockManager->SpawnAllyBlock(m_player.get());
    }
    // --------------------------------

    if (m_boss) m_boss->Update(elapsedTime);

    UpdateProjectileWindows();

    if (m_enemyManager && m_player)
    {
        // Enemy butuh Camera active dan Posisi Player untuk tracking
        Camera* activeCam = CameraController::Instance().GetActiveCamera().get();
        m_enemyManager->Update(elapsedTime, activeCam, m_player->GetPosition());

        UpdateEnemyWindows();
    }

    if (m_itemManager)
    {
        UpdateItemWindows(); // Handle Item Windows
    }

    // 5. Update Window System
    if (m_windowSystem)
    {
        m_windowSystem->Update(elapsedTime);
    }

    // 6. Throttled priority enforcement
    m_priorityEnforceTimer += elapsedTime;
    if (m_priorityEnforceTimer >= PRIORITY_ENFORCE_INTERVAL)
    {
        WindowManager::Instance().EnforceWindowPriorities();
        m_priorityEnforceTimer = 0.0f;
    }

    if (m_boss && m_player)
    {
        auto& projectiles = m_boss->GetProjectiles();
        DirectX::XMFLOAT3 pPos = m_player->GetPosition();

        for (auto& proj : projectiles)
        {
            if (!proj.active) continue;

            // Cek Jarak (Hitbox)
            float dx = pPos.x - proj.position.x;
            float dy = pPos.y - proj.position.y;
            float dz = pPos.z - proj.position.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            // Jarak < 1.5 unit berarti kena
            if (distSq < (1.5f * 1.5f))
            {
                // 1. Hapus Projectile (Biar gak kena berkali-kali)
                proj.active = false;

                // 2. Efek Visual: Camera Shake (Guncangan)
                // Ini memberikan impact tanpa menghancurkan layar
                //CameraController::Instance().ApplyShake(0.3f, 0.2f);

                // 3. (Opsional) Log ke Debug Output
                OutputDebugStringA("ouch! player hit.\n");

                // JANGAN PANGGIL WindowShatterManager::Instance().TriggerExplosion(...) DI SINI!
                // Kecuali nyawa player sudah 0 (Game Over).
            }
        }
    }

    HandleDebugInput();
    CameraController::Instance().Update(elapsedTime);

    if (m_itemManager && m_player)
    {
        m_itemManager->SetTrackTarget(m_player->GetPosition());
        m_itemManager->Update(elapsedTime, activeCam);
    }

    // [BARU] Update Collision
    if (m_collisionManager && m_player)
    {
        // FORCE Stage 3: CollisionManager hanya jalan jika stage == 3
        // (Sesuai logika di CollisionManager.cpp: if (m_player->GetGameStage() != 3) return;)
        if (m_player->GetGameStage() != 3)
        {
            m_player->SetGameStage(3);
        }

        m_collisionManager->Update(elapsedTime);
    }

    if (m_boss && m_blockManager)
    {
        auto& projectiles = m_boss->GetProjectiles();

        // Kita ambil referensi vector blocks biar cepat
        // (Pastikan BlockManager punya getter GetBlocks() yang public)
        const auto& blocks = m_blockManager->GetBlocks();

        // LOOP 1: Untuk setiap Peluru
        for (auto& proj : projectiles)
        {
            if (!proj.active) continue; // Skip peluru mati

            // LOOP 2: Cek terhadap setiap Block
            for (auto& block : blocks)
            {
                // PENTING: Jangan cek block yang sudah hancur!
                // Kalau ini lupa, peluru akan meledak kena "angin" (bekas tempat block)
                if (!block || !block->IsActive()) continue;

                // Ambil posisi
                DirectX::XMFLOAT3 bPos = block->GetMovement()->GetPosition();

                // Hitung Jarak (Squared Distance check lebih cepat daripada sqrt)
                float dx = bPos.x - proj.position.x;
                float dy = bPos.y - proj.position.y;
                float dz = bPos.z - proj.position.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                // Radius Block ~0.8f, Radius File ~0.5f -> Total ~1.3f
                // 1.3 * 1.3 = 1.69f (Kita bulatkan jadi 2.0f biar gampang kena)
                if (distSq < 2.0f)
                {
                    // === TABRAKAN TERJADI ===

                    // 1. Hancurkan Block
                    block->SetActive(false); // Atau panggil fungsi Damage block

                    // 2. Hancurkan Peluru
                    proj.active = false;

                    // 3. Efek Partikel/Suara (Opsional)
                    // WindowShatterManager::Instance().SpawnSmallShard(bPos); 

                    // 4. [SANGAT PENTING] BREAK INNER LOOP
                    // Karena peluru ini SUDAH MATI, dia tidak boleh mengecek block lain lagi.
                    // Jika tidak di-break, peluru mati ini bisa membunuh block lain di frame yang sama.
                    break;
                }
            }
        }
    }
}

static void DrawTransformControl(const char* label, DirectX::XMFLOAT3* pos)
{
    if (ImGui::TreeNode(label))
    {
        if (pos) ImGui::DragFloat3("Position", &pos->x, 0.1f);
        ImGui::TreePop();
    }
}

// ======================================================== =
// DRAW GUI - MAIN ENTRY
// =========================================================
void SceneGameBeyond::DrawGUI()
{
    // Setup Window Style
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("BEYOND INSPECTOR", nullptr))
    {
        if (ImGui::BeginTabBar("BeyondTabs"))
        {
            if (ImGui::BeginTabItem("General")) { DrawTabGeneral();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Objects")) { DrawTabObjects();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Window System")) { DrawTabWindowSystem(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Camera")) { DrawTabCamera();       ImGui::EndTabItem(); }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

// =========================================================
// GUI HELPER FUNCTIONS
// =========================================================

void SceneGameBeyond::DrawTabGeneral()
{
    ImGui::Spacing();

    // --- SECTION 1: GAME STATE ---
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "GAME STATE");
    ImGui::Checkbox("Game Started", &m_gameStarted);
    ImGui::Checkbox("Shatter Triggered", &m_shatterTriggered);
    ImGui::Separator();

    // --- SECTION 2: BOUNDARY ---
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "PLAYER BOUNDARY (1920x1080)");
    ImGui::TextDisabled("Ratio: 1 Unit = 40 Pixels");

    ImGui::Indent();
    ImGui::DragFloat("Limit X (Width)", &m_screenLimitX, 0.1f, 10.0f, 50.0f);
    ImGui::DragFloat("Limit Z (Height)", &m_screenLimitZ, 0.1f, 5.0f, 30.0f);
    ImGui::Unindent();
    ImGui::Separator();

    // --- SECTION 3: SHATTER FX ---
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "WINDOW SHATTER FX");

    // Info Stats
    auto& shards = WindowShatterManager::Instance().GetShatters();
    ImGui::Text("Active Shards: %d", (int)shards.size());

    // Controls
    if (ImGui::Button("TRIGGER EXPLOSION", ImVec2(-1, 40)))
    {
        if (m_player) {
            auto pPos = m_player->GetPosition();
            WindowShatterManager::Instance().TriggerExplosion({ pPos.x, pPos.z }, 12);
            m_shatterTriggered = true;
        }
    }

    if (ImGui::Button("Clear All Shards", ImVec2(-1, 0))) {
        WindowShatterManager::Instance().Clear();
    }
}

void SceneGameBeyond::DrawTabObjects()
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "SCENE HIERARCHY");
    ImGui::Separator();

    // --- 1. PLAYER ---
    if (m_player)
    {
        if (ImGui::TreeNode("Player"))
        {
            XMFLOAT3 pos = m_player->GetPosition();
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                m_player->SetPosition(pos.x, pos.y, pos.z);
            }
            ImGui::TreePop();
        }
    }

    // --- 2. BOSS ---
// --- 2. BOSS ---
    if (m_boss)
    {
        if (ImGui::TreeNode("Boss Controller"))
        {
            // [BARU] MONITOR1 DEBUG SECTION
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "MONITOR1 DEBUG");

            ImGui::Text("HP: %d / 5", m_boss->m_monitor1Health);
            ImGui::Text("Destroyed: %s", m_boss->m_monitor1Destroyed ? "YES" : "NO");

            // Row 1: Damage buttons
            if (ImGui::Button("DAMAGE (-1 HP)", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 40)))
            {
                if (!m_boss->m_monitor1Destroyed && m_boss->m_monitor1Health > 0)
                {
                    m_boss->m_monitor1Health--;
                    std::string msg = "MANUAL_DAMAGE: " + std::to_string(m_boss->m_monitor1Health) + " HP";
                    m_boss->AddTerminalLog(msg);
                    OutputDebugStringA(("MANUAL DAMAGE! HP: " + std::to_string(m_boss->m_monitor1Health) + "\n").c_str());

                    if (m_boss->m_monitor1Health <= 0)
                    {
                        m_boss->m_monitor1Destroyed = true;
                        BossPart* monitor1 = m_boss->GetPart("monitor1");
                        if (monitor1)
                        {
                            monitor1->useFloating = false;
                            monitor1->position = { -500.0f, -500.0f, -500.0f };
                        }
                        m_boss->AddTerminalLog("MONITOR1_DESTROYED !!!CRITICAL!!!");
                    }
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("DESTROY", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
            {
                m_boss->m_monitor1Health = 0;
                m_boss->m_monitor1Destroyed = true;
                BossPart* monitor1 = m_boss->GetPart("monitor1");
                if (monitor1)
                {
                    monitor1->useFloating = false;
                    monitor1->position = { -500.0f, -500.0f, -500.0f };
                }
                m_boss->AddTerminalLog("MONITOR1_DESTROYED VIA CONSOLE");
                OutputDebugStringA("MONITOR1 DESTROYED VIA CONSOLE!\n");
            }

            ImGui::Separator();

            // Row 2: Heal/Reset buttons
            if (ImGui::Button("RESET HP", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 30)))
            {
                m_boss->m_monitor1Health = 5;
                m_boss->m_monitor1Destroyed = false;
                BossPart* monitor1 = m_boss->GetPart("monitor1");
                if (monitor1)
                {
                    monitor1->useFloating = true;
                    monitor1->position = { 0.f, 0.6f, 6.5f }; // Original position
                }
                m_boss->AddTerminalLog("MONITOR1_RESET");
                OutputDebugStringA("MONITOR1 RESET!\n");
            }

            ImGui::SameLine();

            if (ImGui::Button("TEST COLLISION", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
            {
                // Move player ke monitor1 untuk test
                BossPart* monitor1 = m_boss->GetPart("monitor1");
                if (m_player && monitor1)
                {
                    DirectX::XMFLOAT3 testPos = monitor1->visualPosition;
                    testPos.z += 1.0f; // Geser sedikit biar collision terdeteksi
                    m_player->SetPosition(testPos.x, testPos.y, testPos.z);
                    OutputDebugStringA("PLAYER TELEPORTED TO MONITOR1 FOR TESTING!\n");
                }
            }

            ImGui::Separator();

            // Existing Boss Debug GUI
            m_boss->DrawDebugGUI();
            ImGui::TreePop();
        }
    }

    ImGui::Separator();

    // --- 3. ENEMY MANAGER ---
    if (m_enemyManager)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "ENEMY MANAGER");

        size_t count = m_enemyManager->GetEnemies().size();
        ImGui::Text("Active Enemies: %d", (int)count);

        // A. GLOBAL SETTINGS (Collapsing Header biar rapi)
        if (ImGui::CollapsingHeader("Global Window Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            ImGui::Text("Camera Offset:");
            ImGui::DragFloat3("##TrackOffset", &m_enemyTrackOffset.x, 0.05f);

            ImGui::Text("Size Offset (Base 250):");
            ImGui::DragFloat2("##SizeOffset", &m_enemySizeOffset.x, 1.0f);

            if (ImGui::Button("Reset All Offsets")) {
                m_enemyTrackOffset = { 0.0f, 0.0f, 0.0f };
                m_enemySizeOffset = { 0.0f, 0.0f };
            }
            ImGui::Unindent();
        }

        // B. INDIVIDUAL LIST
        if (count > 0)
        {
            if (ImGui::TreeNode("Individual Enemy Control"))
            {
                auto& enemies = m_enemyManager->GetEnemies();
                for (int i = 0; i < count; ++i)
                {
                    auto& enemy = enemies[i];
                    if (!enemy) continue;

                    ImGui::PushID(i); // Wajib agar ID unik

                    std::string label = "Enemy " + std::to_string(i);
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        DirectX::XMFLOAT3 pos = enemy->GetPosition();
                        if (ImGui::DragFloat3("Pos", &pos.x, 0.1f)) enemy->SetPosition(pos);

                        DirectX::XMFLOAT3 rot = enemy->GetRotation();
                        if (ImGui::DragFloat3("Rot", &rot.x, 1.0f)) enemy->SetRotation(rot);

                        ImGui::TextDisabled("Type ID: %d", (int)enemy->GetType());
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TextDisabled("No enemies. Spawn one via Boss!");
        }

        // --- ANTENNA WINDOW SETTINGS ---
        if (ImGui::TreeNode("Antenna Window Settings"))
        {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "Tracking Offset (World Units)");
            ImGui::DragFloat3("Pos Offset", &m_antennaTrackOffset.x, 0.1f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "Size Adjustment (Pixels)");
            ImGui::DragFloat2("Size (+/-)", &m_antennaSizeOffset.x, 1.0f);

            // Tombol Reset biar gampang kalau kekecilan/kejauhan
            if (ImGui::Button("Reset Antenna Config"))
            {
                m_antennaTrackOffset = { 0.0f, 4.0f, 0.0f };
                m_antennaSizeOffset = { 0.0f, 0.0f };
            }

            ImGui::TreePop();
        }
    }
}

void SceneGameBeyond::DrawTabWindowSystem()
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "TRACKED WINDOWS SYSTEM");
    ImGui::Separator();

    if (m_windowSystem)
    {
        ImGui::Text("Unified FOV: %.1f", FIELD_OF_VIEW);

        // Bisa ditambah info lain, misal jumlah total window di manager
        // ImGui::Text("Total Windows: %d", ...);

        ImGui::Spacing();
        ImGui::TextWrapped("System is active. New windows will spawn automatically based on logic.");
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "CRITICAL: Window System is NULL");
    }
}

void SceneGameBeyond::DrawTabCamera()
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "MAIN CAMERA DEBUG");
    ImGui::Separator();

    if (m_mainCamera)
    {
        XMFLOAT3 pos = m_mainCamera->GetPosition();
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            m_mainCamera->SetPosition(pos);
        }

        ImGui::Spacing();
        if (ImGui::Button("Reset Camera Pos")) {
            m_mainCamera->SetPosition(0, 5, 0); // Atur default sesuai kebutuhan
        }
    }
}

// =========================================================
// RENDER
// =========================================================
void SceneGameBeyond::Render(float elapsedTime, Camera* camera)
{
    Camera* targetCam = camera ? camera : m_mainCamera.get();
    auto dc = Graphics::Instance().GetDeviceContext();
    auto rs = Graphics::Instance().GetRenderState();

    dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullBack));

    // Pre-shatter: Player, Blocks, and Monitor1 are visible (no black overlay).

    RenderScene(elapsedTime, targetCam);
    if (targetCam == m_mainCamera.get())
    {
        Graphics::Instance().GetShapeRenderer()->Render(dc, targetCam->GetView(), targetCam->GetProjection());
    }
}

void SceneGameBeyond::RenderScene(float elapsedTime, Camera* camera)
{
    if (!camera) return;

    auto dc = Graphics::Instance().GetDeviceContext();
    auto modelRenderer = Graphics::Instance().GetModelRenderer();
    RenderContext rc{ dc, Graphics::Instance().GetRenderState(), camera, nullptr };

    // --- 1. RENDER PLAYER (CULLING) ---
    if (m_player)
    {
        XMFLOAT3 pPos = m_player->GetPosition();
        // Radius player kira-kira 1.0f
        if (camera->CheckSphere(pPos.x, pPos.y, pPos.z, 1.5f))
        {
            m_player->Render(modelRenderer);
        }
    }

    // --- 2. RENDER BOSS ---
    if (m_boss)
    {
        if (m_gameStarted)
            m_boss->Render(modelRenderer, camera);       // Full boss after shatter
        else
            m_boss->RenderPreShatter(modelRenderer, camera); // Only monitor1 before shatter
    }







    // --- 3. RENDER ENEMY (CULLING) ---
    if (m_enemyManager && m_gameStarted)
    {
        // Panggil fungsi render manager, tapi kirim camera untuk culling
        m_enemyManager->Render(modelRenderer, camera);
    }

    // --- 4. RENDER BLOCKS (CULLING) ---
    if (m_blockManager)
    {
        for (const auto& block : m_blockManager->GetBlocks())
        {
            if (block->IsActive())
            {
                // Ambil posisi dari movement component
                XMFLOAT3 bPos = block->GetMovement()->GetPosition();

                // Block kecil, radius 0.5f cukup
                if (camera->CheckSphere(bPos.x, bPos.y, bPos.z, 0.8f))
                {
                    block->Render(modelRenderer, m_blockManager->globalBlockColor);
                }
            }
        }
    }

    if (m_itemManager)
    {
        // ItemManager butuh ModelRenderer, ambil dari Graphics Instance
        auto modelRenderer = Graphics::Instance().GetModelRenderer();
        m_itemManager->Render(modelRenderer);
    }

    modelRenderer->Render(rc);

    // Virtual Shatters (Overlay 2D)
    dc->OMSetDepthStencilState(Graphics::Instance().GetRenderState()->GetDepthStencilState(DepthState::NoTestNoWrite), 0);

    const auto& shatters = WindowShatterManager::Instance().GetShatters();
    for (const auto& shatter : shatters)
    {
        if (!shatter) continue;

        if (!shatter->IsNativeWindow())
        {
            DirectX::XMFLOAT3 worldPos = shatter->GetVirtualWorldPos();
            DirectX::XMFLOAT2 size = shatter->GetSize();

            float screenX, screenY;
            if (m_windowSystem)
            {
                m_windowSystem->WorldToScreenPos(worldPos, screenX, screenY);

                m_primitive2D->Rect(
                    screenX, screenY,
                    size.x, size.y,
                    size.x * 0.5f, size.y * 0.5f,
                    0.0f,
                    1.0f, 1.0f, 1.0f, 1.0f
                );
            }
        }
    }

    m_primitive2D->Render(dc);

    dc->OMSetDepthStencilState(Graphics::Instance().GetRenderState()->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void SceneGameBeyond::OnResize(int width, int height)
{
    if (m_mainCamera) m_mainCamera->SetAspectRatio((float)width / max(1, height));
}

void SceneGameBeyond::HandleDebugInput()
{
    if (Input::Instance().GetKeyboard().IsTriggered('N'))
    {
        GameWindow* addWin = WindowManager::Instance().CreateGameWindow("Debug Cam", 300, 300);
        auto addCam = std::make_shared<Camera>();
        addCam->SetPerspectiveFov(XMConvertToRadians(60), 1.0f, 0.1f, 1000.0f);
        addWin->SetCamera(addCam.get());
        m_additionalCameras.push_back(addCam);
    }
}

// GANTI FUNGSI INI DI SceneGameBeyond.cpp
DirectX::XMFLOAT3 SceneGameBeyond::GetMouseOnGround(Camera* camera)
{
    // 1. Ambil Posisi Mouse Global (Koordinat Layar Monitor)
    POINT pt;
    if (!GetCursorPos(&pt)) return { 0,0,0 }; // WinAPI function

    // 2. Ambil Ukuran Layar Monitor Utama (Resolusi Asli)
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // 3. Hitung Titik Tengah Layar (Center of World 0,0,0)
    float centerX = screenW / 2.0f;
    float centerY = screenH / 2.0f;

    // 4. Hitung Jarak Mouse dari Tengah Layar (dalam Pixel)
    float dx = (float)pt.x - centerX;
    float dy = (float)pt.y - centerY;

    // 5. Konversi Pixel ke World Unit
    // Kita gunakan Ratio yang sama dengan WindowTrackingSystem (40.0f)
    // Rumus: Jarak Pixel / Ratio = Jarak World

    float worldX = dx / PIXEL_TO_UNIT_RATIO;

    // Note: Screen Y positif ke bawah, tapi World Z positif ke atas (tergantung kamera).
    // Biasanya untuk top-down view desktop: Y layar turun = Z world mundur.
    float worldZ = -dy / PIXEL_TO_UNIT_RATIO;

    return DirectX::XMFLOAT3(worldX, 0.0f, worldZ);
}

void SceneGameBeyond::UpdateProjectileWindows()
{
    if (!m_boss || !m_windowSystem) return;

    const auto& projectiles = m_boss->GetProjectiles();
    int windowFrequency = 4;

    // =========================================================
    // TAHAP 1: DATA GATHERING (MARK)
    // Catat semua ID Projectile yang "Berhak" punya window saat ini.
    // =========================================================
    std::unordered_set<int> activeProjectileIDs;
    for (const auto& p : projectiles)
    {
        if (p.active && (p.id % windowFrequency == 0))
        {
            activeProjectileIDs.insert(p.id);
        }
    }

    // =========================================================
    // TAHAP 2: CLEANUP ORPHANS (SWEEP)
    // Cek semua window yang ada. Jika ID-nya tidak ada di daftar aktif, HAPUS.
    // =========================================================

    // Kita butuh list sementara agar tidak merusak iterator saat looping
    std::vector<std::string> windowsToRemove;
    const auto& currentWindows = m_windowSystem->GetWindows();

    for (const auto& win : currentWindows)
    {
        // Cek apakah ini window milik projectile (prefix "file_proj_")
        if (win->name.find("file_proj_") == 0)
        {
            // Ambil ID dari nama string. "file_proj_" panjangnya 10 karakter.
            // Contoh: "file_proj_105" -> diambil "105"
            try {
                std::string idStr = win->name.substr(10);
                int id = std::stoi(idStr);

                // Jika ID ini TIDAK ada di daftar projectile aktif -> Masukkan ke tong sampah
                if (activeProjectileIDs.find(id) == activeProjectileIDs.end())
                {
                    windowsToRemove.push_back(win->name);
                }
            }
            catch (...) { /* Safety jika parsing gagal */ }
        }
    }

    // Eksekusi pembersihan (Release ke Pool)
    for (const auto& name : windowsToRemove)
    {
        m_windowSystem->ReleasePooledWindow(name);
    }

    // =========================================================
    // TAHAP 3: SPAWN NEW WINDOWS
    // Buat window untuk projectile baru yang belum punya window
    // =========================================================
    for (const auto& p : projectiles)
    {
        if (p.active && (p.id % windowFrequency == 0))
        {
            std::string winName = "file_proj_" + std::to_string(p.id);

            // Jika belum ada window-nya, bikin baru (ambil dari pool)
            if (m_windowSystem->GetTrackedWindow(winName) == nullptr)
            {
                int targetID = p.id;
                Boss* targetBoss = m_boss.get();

                m_windowSystem->AddPooledTrackedWindow(
                    {
                        winName,
                        "DOWNLOADING...",
                        120,
                        120,
                        10,
                        { 0.0f, 0.0f, 0.0f },
                        20.0f // FPS Limit
                    },
                    [targetBoss, targetID]() -> DirectX::XMFLOAT3 {
                        DirectX::XMFLOAT3 pos = { 0,0,0 };
                        if (targetBoss->GetProjectileData(targetID, pos)) return pos;
                        return { 0, -1000, 0 };
                    }
                );
            }
        }
    }
}