#include <raylib.h>
#include <raymath.h>
#include <ctime>
#include <algorithm>
using namespace std;

// ---------------- 常量 ----------------
const int   SCREEN_W = 800;
const int   SCREEN_H = 600;
const int   MAX_BULLETS = 64;       // 弹药池上限
const float BULLET_SPEED = 400.0f;
const float BULLET_R = 4.0f;
const float ENEMY_R = 6.0f;
const float ENEMY_SPEED = 180.0f;
const int   MAX_ENEMIES = 1024;
const float PLAYER_MAX_HP = 100.0f;
const float ENEMY_DAMAGE = 10.0f;
const int   WAVE_BASE = 5;
const int   WAVE_STEP = 2;
const float FIRE_INTERVAL = 0.12f;  // 两发之间的最小间隔（秒），0.12 约等于 8 发/秒


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

    int   point = 0;
    int   wave = 0;
    float fireTimer = 0.0f;         // 距离下一发还要等多久（秒）
    bool  isGameOver = false;

    // 玩家
    Vector2 playerPos = { SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    float   playerR = 8.0f;
    float   playerSpeed = 240.0f;
    float   playerHP = PLAYER_MAX_HP;

    // 子弹池
    Bullet bullets[MAX_BULLETS];
    int    bulletCount = 0;

    // 敌人
    SetRandomSeed((unsigned int)time(NULL));
    Enemy enemies[MAX_ENEMIES] = {};
    int enemyCount = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 死亡后不再更新任何逻辑，画面冻结，按 ESC 退出
        if (!isGameOver) {
            // 一波清空：波数 +1，按新波数生成
            if (enemyCount == 0) {
                wave++;
                int enemiesPerWave = min(WAVE_BASE + (wave - 1) * WAVE_STEP, MAX_ENEMIES);
                for (int i = 0; i < enemiesPerWave; i++) {
                    enemies[enemyCount] = {};                       // 先清空这一格，再写坐标
                    enemies[enemyCount].position.x = (float)GetRandomValue(20, SCREEN_W - 20);
                    enemies[enemyCount].position.y = (float)GetRandomValue(40, 200);
                    enemyCount++;
                }
            }

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

            // 发射：受冷却限制，太近不开火（144 = 12^2）
            fireTimer -= dt;
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && fireTimer <= 0.0f &&
                bulletCount < MAX_BULLETS && Vector2LengthSqr(toMouse) > 144) {
                bullets[bulletCount].position = playerPos;
                bullets[bulletCount].velocity = bulletDir * BULLET_SPEED;
                bulletCount++;
                fireTimer = FIRE_INTERVAL;
            }

            // 更新子弹，出屏的用 swap-and-pop 回收
            for (int i = 0; i < bulletCount;) {
                bullets[i].position.y += bullets[i].velocity.y * dt;
                bullets[i].position.x += bullets[i].velocity.x * dt;
                if (bullets[i].position.y < 0 || bullets[i].position.x < 0 ||
                    bullets[i].position.y > SCREEN_H || bullets[i].position.x > SCREEN_W) {
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

            // 命中检测：一颗子弹只打一个敌人，命中后子弹立刻回收（swap-and-pop）
            for (int i = 0; i < bulletCount;) {
                bool isHit = false;
                for (int j = 0; j < enemyCount; j++) {
                    if (!enemies[j].isDead &&
                        Vector2DistanceSqr(bullets[i].position, enemies[j].position)
                        <= (BULLET_R + ENEMY_R) * (BULLET_R + ENEMY_R)) {
                        enemies[j].hitFlash = 0.3f;
                        enemies[j].isDead = true;
                        point += 1;
                        isHit = true;
                        break;
                    }
                }
                if (isHit) {
                    bullets[i] = bullets[bulletCount - 1];
                    bulletCount--;              // i 不前进：换进来的那颗子弹还没检查
                }
                else i++;
            }

            // 删除倒计时结束的敌人（swap-and-pop）
            for (int i = 0; i < enemyCount;) {
                if (enemies[i].isDead && enemies[i].hitFlash <= 0) {
                    enemies[i] = enemies[enemyCount - 1];
                    enemyCount--;
                }
                else i++;
            }

            // 敌人撞到玩家：扣血，敌人自毁
            for (int i = 0; i < enemyCount;) {
                if (Vector2DistanceSqr(playerPos, enemies[i].position)
                    <= (playerR + ENEMY_R) * (playerR + ENEMY_R) && !enemies[i].isDead) {
                    playerHP -= ENEMY_DAMAGE;
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

            // 血量下限与死亡判定
            if (playerHP < 0.0f) playerHP = 0.0f;
            if (playerHP <= 0.0f) isGameOver = true;
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
        DrawText(TextFormat("HP: %.0f / %.0f", playerHP, PLAYER_MAX_HP), 10, 50, 20, DARKGRAY);
        DrawText(TextFormat("Point: %d", point), 10, 70, 20, DARKGRAY);
        DrawText(TextFormat("Wave: %d", wave), 10, 90, 20, DARKGRAY);

        // 结束画面（ESC 由 WindowShouldClose 处理，Raylib 默认 ESC 退出）
        if (isGameOver) {
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(RAYWHITE, 0.75f));
            DrawText("GAME OVER", SCREEN_W / 2 - 105, SCREEN_H / 2 - 70, 40, RED);
            DrawText(TextFormat("Point %d    Wave %d", point, wave), SCREEN_W / 2 - 85, SCREEN_H / 2, 24, DARKGRAY);
            DrawText("Press ESC to exit", SCREEN_W / 2 - 90, SCREEN_H / 2 + 40, 24, DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
