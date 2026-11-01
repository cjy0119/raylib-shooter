#include <raylib.h>
#include <raymath.h>
#include <ctime>

// ---------------- 常量 ----------------
const int   SCREEN_W = 800;
const int   SCREEN_H = 600;
const int   MAX_BULLETS = 64;       // 弹药池上限
const float BULLET_SPEED = 400.0f;
const float BULLET_R = 4.0f;
const float ENEMY_R = 6.0f;
const float ENEMY_SPEED = 120.0f;
const int   MAX_ENEMIES = 8;

// ---------------- 数据结构 ----------------
struct Bullet {
    Vector2 position;
    Vector2 velocity;
};
struct Enemy {
    Vector2 position;
    Vector2 velocity;
    float hitFlash;     // 受击闪烁剩余时间（秒）
    bool  isDead;       // 已受击，倒计时结束后删除
};

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "bullet pool");
    SetTargetFPS(60);

    // 玩家
    Vector2 playerPos = { SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    float   playerR = 8.0f;
    float   playerSpeed = 240.0f;

    // 子弹池
    Bullet bullets[MAX_BULLETS];
    int    bulletCount = 0;

    // 敌人生成（随机位置）
    SetRandomSeed((unsigned int)time(NULL));
    Enemy enemies[MAX_ENEMIES] = {};
    int enemyCount = 0;
    for (int i = 0; i < 5; i++) {
        enemies[enemyCount].position.x = (float)GetRandomValue(20, SCREEN_W - 20);
        enemies[enemyCount].position.y = (float)GetRandomValue(40, 200);
        enemyCount++;
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 玩家移动
        if (IsKeyDown(KEY_W)) playerPos.y -= playerSpeed * dt;
        if (IsKeyDown(KEY_A)) playerPos.x -= playerSpeed * dt;
        if (IsKeyDown(KEY_S)) playerPos.y += playerSpeed * dt;
        if (IsKeyDown(KEY_D)) playerPos.x += playerSpeed * dt;
        playerPos.x = Clamp(playerPos.x, playerR, SCREEN_W - playerR);
        playerPos.y = Clamp(playerPos.y, playerR, SCREEN_H - playerR);

        // 鼠标方向（单位向量）
        Vector2 mouse = GetMousePosition();
        Vector2 toMouse = mouse - playerPos;
        Vector2 bulletDir = Vector2Normalize(toMouse);

        // 发射：占用下一个空槽，太近不开火（144 = 12^2）
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && bulletCount < MAX_BULLETS && Vector2LengthSqr(toMouse) > 144) {
            bullets[bulletCount].position = playerPos;
            bullets[bulletCount].velocity = bulletDir * BULLET_SPEED;
            bulletCount++;
        }

        // 更新子弹，出屏的用 swap-and-pop 回收
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

        // 敌人受击倒计时
        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i].hitFlash > 0) enemies[i].hitFlash -= dt;
        }

        // 命中检测：圆心距 <= 半径之和（两边都用平方，避免开根号）
        for (int i = 0; i < bulletCount; i++) {
            for (int j = 0; j < enemyCount; j++) {
                if (Vector2DistanceSqr(bullets[i].position, enemies[j].position)
                    <= (BULLET_R + ENEMY_R) * (BULLET_R + ENEMY_R)&&!enemies[j].isDead) {
                    enemies[j].hitFlash = 0.3f;
                    enemies[j].isDead = true;
                    break;
                }
            }
        }

        // 删除倒计时结束的敌人（swap-and-pop）
        for (int i = 0; i < enemyCount;) {
            if (enemies[i].isDead && enemies[i].hitFlash <= 0) {
                enemies[i] = enemies[enemyCount - 1];
                enemyCount--;
            }
            else i++;
        }

        // 活着的敌人追踪玩家（受击中的停住不动）
        for (int i = 0; i < enemyCount; i++) {
            if (!enemies[i].isDead) {
                Vector2 toPlayer = playerPos - enemies[i].position;
                Vector2 enemyDir = Vector2Normalize(toPlayer);
                enemies[i].velocity = enemyDir * ENEMY_SPEED;
                enemies[i].position.y += enemies[i].velocity.y * dt;
                enemies[i].position.x += enemies[i].velocity.x * dt;
            }
        }

        // ---------------- 绘制 ----------------
        BeginDrawing();
        ClearBackground(WHITE);

        DrawCircleV(playerPos, playerR, RED);

        // 敌人（受击时变红），先画敌人再画子弹
        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i].hitFlash > 0) DrawCircleV(enemies[i].position, ENEMY_R, RED);
            else DrawCircleV(enemies[i].position, ENEMY_R, GREEN);
        }

        // 子弹
        for (int i = 0; i < bulletCount; i++) {
            DrawCircleV(bullets[i].position, BULLET_R, BLUE);
        }

        // HUD
        DrawText(TextFormat("bullets: %d / %d", bulletCount, MAX_BULLETS), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("enemies: %d / %d", enemyCount, MAX_ENEMIES), 10, 30, 20, DARKGRAY);
        DrawText(TextFormat("mouse %.0f,%.0f", mouse.x, mouse.y), 10, 40, 20, DARKGRAY);
        DrawText(TextFormat("player  %.0f,%.0f", playerPos.x, playerPos.y), 10, 60, 20, DARKGRAY);
        DrawText(TextFormat("toMouse %.0f,%.0f", toMouse.x, toMouse.y), 10, 80, 20, DARKGRAY);
        DrawText(TextFormat("len %.1f", Vector2Length(toMouse)), 10, 100, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
