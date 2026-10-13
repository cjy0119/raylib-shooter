#include <raylib.h>
#include <raymath.h>

const int   SCREEN_W = 800;
const int   SCREEN_H = 600;
const int   MAX_BULLETS = 64;      // 弹药池上限，槽位占满时停火
const float BULLET_SPEED = 400.0f;
const float BULLET_R = 4.0f;

struct Bullet {
    Vector2 position;
    Vector2 velocity;
};

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "bullet pool");
    SetTargetFPS(60);

    Vector2 playerPos = { SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    float   playerR = 8.0f;
    float   playerSpeed = 240.0f;

    Bullet bullets[MAX_BULLETS];
    int    bulletCount = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 玩家移动
        if (IsKeyDown(KEY_W)) playerPos.y -= playerSpeed * dt;
        if (IsKeyDown(KEY_A)) playerPos.x -= playerSpeed * dt;
        if (IsKeyDown(KEY_S)) playerPos.y += playerSpeed * dt;
        if (IsKeyDown(KEY_D)) playerPos.x += playerSpeed * dt;
        playerPos.x = Clamp(playerPos.x, playerR, SCREEN_W - playerR);
        playerPos.y = Clamp(playerPos.y, playerR, SCREEN_H - playerR);

        Vector2 mouse = GetMousePosition();
        Vector2 toMouse = mouse - playerPos;
        Vector2 dir = Vector2Normalize(toMouse);

        // 发射：占用下一个空槽
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && bulletCount < MAX_BULLETS && Vector2LengthSqr(toMouse) > 144) {
            bullets[bulletCount].position = playerPos;
            bullets[bulletCount].velocity = dir * BULLET_SPEED;
            bulletCount++;
        }

        // 更新并回收出屏子弹
        // swap-and-pop：与最后一个存活子弹交换后收缩计数。
        // 不推进 i，让循环留在原地检查被换进来的那颗。
        for (int i = 0; i < bulletCount;) {
            bullets[i].position.y += bullets[i].velocity.y * dt;
            bullets[i].position.x += bullets[i].velocity.x * dt;
            if (bullets[i].position.y < 0|| bullets[i].position.x < 0|| 
                bullets[i].position.y > SCREEN_H|| bullets[i].position.x > SCREEN_W) {
                Bullet temp = bullets[bulletCount - 1];
                bullets[bulletCount - 1] = bullets[i];
                bullets[i] = temp;
                bulletCount--;
            }
            else i++;
        }

        BeginDrawing();
        ClearBackground(WHITE);

        DrawCircleV(playerPos, playerR, RED);
        for (int i = 0; i < bulletCount; i++) {
            DrawCircleV(bullets[i].position, BULLET_R, BLUE);
        }
        DrawText(TextFormat("bullets: %d / %d", bulletCount, MAX_BULLETS), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("mouse %.0f,%.0f", mouse.x, mouse.y), 10, 40, 20, DARKGRAY);
        DrawText(TextFormat("player  %.0f,%.0f", playerPos.x, playerPos.y), 10, 60, 20, DARKGRAY);
        DrawText(TextFormat("toMouse %.0f,%.0f", toMouse.x, toMouse.y), 10, 80, 20, DARKGRAY);
        DrawText(TextFormat("len %.1f", Vector2Length(toMouse)), 10, 100, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
