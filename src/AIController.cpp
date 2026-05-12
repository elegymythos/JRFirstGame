/**
 * @file AIController.cpp
 * @brief AI控制器实现 - 观测向量构建与动作解码
 *
 * 实现AIObservation::build()将游戏状态归一化为神经网络输入向量，
 * 以及AIObservation::decodeAction()将网络输出动作ID解码为AIAction。
 */

#include "AIController.hpp"
#include <numeric>

std::array<float, AIObservation::OBS_DIM> AIObservation::build(const AIGameState& state) {
    std::array<float, OBS_DIM> obs{};
    int idx = 0;
    const auto& p = state.player;

    obs[idx++] = clamp(p.x / 10000.f, -1.f, 1.f);
    obs[idx++] = clamp(p.y / 10000.f, -1.f, 1.f);
    obs[idx++] = static_cast<float>(p.health) / static_cast<float>(p.maxHealth);
    obs[idx++] = clamp(p.vx / 400.f, -1.f, 1.f);
    obs[idx++] = clamp(p.vy / 400.f, -1.f, 1.f);
    obs[idx++] = p.isRolling ? 1.f : 0.f;
    obs[idx++] = 1.f - clamp(p.attackCooldown / std::max(p.attackCooldownMax, 1e-6f), 0.f, 1.f);
    obs[idx++] = 1.f - clamp(p.rollCooldown / std::max(p.rollCooldownMax, 1e-6f), 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(p.totalScore) / 10000.f, 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(p.level) / 50.f, 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(state.currentVictoryStage) / 5.f, 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(p.killCount) / 100.f, 0.f, 1.f);
    obs[idx++] = clamp(p.attackRange / 300.f, 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(p.attackDamage) / 100.f, 0.f, 1.f);
    obs[idx++] = p.isInvincible ? 1.f : 0.f;
    obs[idx++] = clamp(p.damageBonus / 3.f, 0.f, 1.f);
    obs[idx++] = clamp(static_cast<float>(p.strength) / 50.f, 0.f, 1.f);
    obs[idx++] = clamp(p.critChance / 1.f, 0.f, 1.f);

    float nearestEnemyDist = 1e9f;
    int nearestEnemyIdx = -1;
    std::vector<std::pair<int, float>> enemyDists;
    for (int i = 0; i < static_cast<int>(state.enemies.size()); ++i) {
        if (state.enemies[i].alive) {
            float dx = state.enemies[i].x - p.x;
            float dy = state.enemies[i].y - p.y;
            float d2 = dx*dx + dy*dy;
            enemyDists.emplace_back(i, d2);
            if (d2 < nearestEnemyDist) {
                nearestEnemyDist = d2;
                nearestEnemyIdx = i;
            }
        }
    }
    std::sort(enemyDists.begin(), enemyDists.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    if (enemyDists.size() > MAX_ENEMIES)
        enemyDists.resize(MAX_ENEMIES);

    float nearestDist = (nearestEnemyIdx >= 0) ? std::sqrt(nearestEnemyDist) : 1e9f;
    obs[idx++] = (nearestDist <= p.attackRange) ? 1.f : 0.f;

    int nearbyCount = 0;
    for (const auto& e : state.enemies) {
        if (e.alive) {
            float dx = e.x - p.x;
            float dy = e.y - p.y;
            if (std::sqrt(dx*dx + dy*dy) < 300.f) ++nearbyCount;
        }
    }
    obs[idx++] = clamp(static_cast<float>(nearbyCount) / 10.f, 0.f, 1.f);

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (i < static_cast<int>(enemyDists.size())) {
            const auto& e = state.enemies[enemyDists[i].first];
            obs[idx++] = static_cast<float>(e.entityType) / 3.f;
            obs[idx++] = clamp((e.x - p.x) / 800.f, -1.f, 1.f);
            obs[idx++] = clamp((e.y - p.y) / 800.f, -1.f, 1.f);
            obs[idx++] = static_cast<float>(e.health) / std::max(e.maxHealth, 1);
            float dist = std::sqrt((e.x - p.x) * (e.x - p.x) + (e.y - p.y) * (e.y - p.y));
            obs[idx++] = clamp(dist / 800.f, 0.f, 1.f);
            obs[idx++] = e.isRanged ? 1.f : 0.f;
            float evx, evy;
            normalizeAngle(e.vx, e.vy, evx, evy);
            obs[idx++] = evx;
            obs[idx++] = evy;
            obs[idx++] = clamp(e.attackCooldown / std::max(e.attackCooldownMax, 1e-6f), 0.f, 1.f);
            obs[idx++] = (dist <= p.attackRange + e.radius) ? 1.f : 0.f;
        } else {
            idx += ENEMY_DIM;
        }
    }

    std::vector<std::pair<int, float>> projDists;
    for (int i = 0; i < static_cast<int>(state.projectiles.size()); ++i) {
        if (state.projectiles[i].alive && !state.projectiles[i].fromPlayer) {
            float dx = state.projectiles[i].x - p.x;
            float dy = state.projectiles[i].y - p.y;
            projDists.emplace_back(i, dx*dx + dy*dy);
        }
    }
    std::sort(projDists.begin(), projDists.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    if (projDists.size() > MAX_PROJECTILES)
        projDists.resize(MAX_PROJECTILES);

    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (i < static_cast<int>(projDists.size())) {
            const auto& pr = state.projectiles[projDists[i].first];
            obs[idx++] = clamp((pr.x - p.x) / 800.f, -1.f, 1.f);
            obs[idx++] = clamp((pr.y - p.y) / 800.f, -1.f, 1.f);
            float pdx, pdy;
            normalizeAngle(pr.dx * pr.speed, pr.dy * pr.speed, pdx, pdy);
            obs[idx++] = pdx;
            obs[idx++] = pdy;
            obs[idx++] = pr.fromPlayer ? 1.f : 0.f;
            float projVx = pr.dx * pr.speed;
            float projVy = pr.dy * pr.speed;
            float projSpd = std::sqrt(projVx * projVx + projVy * projVy);
            if (projSpd > 1e-6f && !pr.fromPlayer) {
                float toDx = p.x - pr.x;
                float toDy = p.y - pr.y;
                float dot = toDx * projVx + toDy * projVy;
                if (dot > 0.f) {
                    float ttr = std::sqrt(toDx * toDx + toDy * toDy) / projSpd;
                    obs[idx++] = clamp(ttr / 60.f, 0.f, 1.f);
                } else {
                    obs[idx++] = 1.f;
                }
            } else {
                obs[idx++] = 1.f;
            }
        } else {
            idx += PROJ_DIM;
        }
    }

    std::vector<std::pair<int, float>> dropDists;
    for (int i = 0; i < static_cast<int>(state.drops.size()); ++i) {
        if (state.drops[i].alive) {
            float dx = state.drops[i].x - p.x;
            float dy = state.drops[i].y - p.y;
            dropDists.emplace_back(i, dx*dx + dy*dy);
        }
    }
    std::sort(dropDists.begin(), dropDists.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    if (dropDists.size() > MAX_DROPS)
        dropDists.resize(MAX_DROPS);

    for (int i = 0; i < MAX_DROPS; ++i) {
        if (i < static_cast<int>(dropDists.size())) {
            const auto& d = state.drops[dropDists[i].first];
            obs[idx++] = static_cast<float>(d.dropType) / 5.f;
            obs[idx++] = clamp((d.x - p.x) / 800.f, -1.f, 1.f);
            obs[idx++] = clamp((d.y - p.y) / 800.f, -1.f, 1.f);
            float dist = std::sqrt((d.x - p.x) * (d.x - p.x) + (d.y - p.y) * (d.y - p.y));
            obs[idx++] = clamp(dist / 800.f, 0.f, 1.f);
        } else {
            idx += DROP_DIM;
        }
    }

    std::array<float, DANGER_GRID_DIM> dangerDirs{};
    for (const auto& e : state.enemies) {
        if (!e.alive) continue;
        float edx = e.x - p.x;
        float edy = e.y - p.y;
        float edist = std::sqrt(edx*edx + edy*edy);
        if (edist > 400.f || edist < 1e-6f) continue;
        float angle = std::atan2(edy, edx);
        int sector = static_cast<int>((angle + 3.14159265f) / (2.f * 3.14159265f / DANGER_GRID_DIM)) % DANGER_GRID_DIM;
        float weight = (1.f - edist / 400.f) * (static_cast<float>(e.attackDamage) / 30.f);
        dangerDirs[sector] += weight;
    }
    for (int i = 0; i < DANGER_GRID_DIM; ++i) {
        obs[idx++] = clamp(dangerDirs[i] / 5.f, 0.f, 1.f);
    }

    obs[idx++] = clamp(state.gameTime / 600.f, 0.f, 1.f);
    obs[idx++] = clamp(state.difficultyScale / 10.f, 0.f, 1.f);

    return obs;
}

AIAction AIObservation::decodeAction(int action) {
    AIAction result;

    if (action < 9) {
        result.moveX = MOVE_DIRS[action].first;
        result.moveY = MOVE_DIRS[action].second;
    } else if (action == 9) {
        result.attack = true;
        result.attackTarget = 1;
    } else if (action == 10) {
        result.roll = true;
    } else {
        result.pickup = true;
    }

    return result;
}
