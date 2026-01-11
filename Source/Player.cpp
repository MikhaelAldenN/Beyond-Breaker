#include "Player.h"
#include "System/Graphics.h" 
#include "Camera.h"
#include "System/Input.h"

// PENTING: Include Header State Konkret di sini agar 'PlayerIdle' dikenali
#include "PlayerStates.h" 
#include "AnimationController.h"
#include "StateMachine.h"

using namespace DirectX;

Player::Player()
{
    // 1. Init Model
    ID3D11Device* device = Graphics::Instance().GetDevice();
    model = std::make_shared<Model>(device, "Data/Model/Character/PLACEHOLDER_mdl_Block.glb");
    scale = { 3.0f, 3.0f, 3.0f };

    // 2. Init Animation Controller
    animator = new AnimationController();
    animator->Initialize(model);

    // 3. Init State Machine
    stateMachine = new StateMachine();
    // Memulai dengan State Idle
    stateMachine->Initialize(new PlayerIdle(), this);
}

Player::~Player()
{
    if (stateMachine) delete stateMachine;
    if (animator) delete animator;
}

void Player::Update(float elapsedTime, Camera* camera)
{
    // Simpan referensi camera frame ini
    SetCamera(camera);

    // 2. Physics 
    if (isInputEnabled)
    {
        if (stateMachine) stateMachine->Update(this, elapsedTime);
        movement->Update(elapsedTime);
    }

    // 3. Visuals (Animator hitung pose)
    if (animator) animator->Update(elapsedTime);

    // 4. Sync (Physics -> Graphics)
    SyncData();
}

// Perbaikan nama fungsi agar sesuai header: HandleMovementInput
// Hapus parameter Camera* karena kita pakai this->activeCamera
void Player::HandleMovementInput()
{
    float x = 0.0f;
    float z = 0.0f;

    // --- Keyboard Input ---
    if (GetAsyncKeyState('W') & 0x8000) z += 1.0f;
    if (GetAsyncKeyState('S') & 0x8000) z -= 1.0f;
    if (GetAsyncKeyState('D') & 0x8000) x += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) x -= 1.0f;

    // --- Camera Relative Movement ---
    if (activeCamera)
    {
        XMFLOAT3 camFront = activeCamera->GetFront();
        XMFLOAT3 camRight = activeCamera->GetRight();

        // Flatten Y (biar tidak terbang/menukik saat nunduk/dongak)
        camFront.y = 0;
        camRight.y = 0;

        XMVECTOR vFront = XMLoadFloat3(&camFront);
        XMVECTOR vRight = XMLoadFloat3(&camRight);
        vFront = XMVector3Normalize(vFront);
        vRight = XMVector3Normalize(vRight);

        // Calculate Direction
        XMVECTOR vDir = (vRight * x) + (vFront * z);

        // Normalize vector (biar jalan miring tidak lebih cepat)
        if (XMVectorGetX(XMVector3LengthSq(vDir)) > 1.0f)
        {
            vDir = XMVector3Normalize(vDir);
        }

        XMFLOAT3 finalDir;
        XMStoreFloat3(&finalDir, vDir);

        movement->SetMoveInput(finalDir.x, finalDir.z);
    }
    else
    {
        // Fallback: World Space movement
        movement->SetMoveInput(x, z);
    }

    // PENTING: JANGAN panggil movement->Jump() di sini!
    // Itu tugas State PlayerJump::Enter().
}

// Implementasi fungsi cek lompat
bool Player::CheckJumpInput()
{
    // Cek apakah spasi ditekan
    return (GetAsyncKeyState(VK_SPACE) & 0x8000);
}