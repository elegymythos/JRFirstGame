#include "MapSystem.hpp"
#include <random>

MapSystem::MapSystem() {}

MapRegion& MapSystem::getOrCreateRegion(int col, int row) {
    int key = regionKey(col, row);
    auto it = regions_.find(key);
    if (it != regions_.end()) return it->second;

    MapRegion region;
    region.regionId = key;
    region.col = col;
    region.row = row;
    region.spawned = false;
    assignEnemyConfig(region, col, row);
    regions_[key] = std::move(region);
    return regions_[key];
}

void MapSystem::assignEnemyConfig(MapRegion& region, int col, int row) {
    // 基于位置决定敌人配置，越远越难
    int dist = std::abs(col) + std::abs(row);

    if (dist == 0) {
        // 出生点：非常简单
        region.enemyConfig = {{EnemyType::Slime, 1}};
    } else if (dist <= 2) {
        // 近处：简单
        region.enemyConfig = {
            {EnemyType::Slime, 1},
            {EnemyType::SkeletonWarrior, 1}
        };
    } else if (dist <= 4) {
        // 中距离：中等
        region.enemyConfig = {
            {EnemyType::Slime, 1},
            {EnemyType::SkeletonWarrior, 1},
            {EnemyType::SkeletonArcher, 1}
        };
    } else if (dist <= 6) {
        // 远距离：较难
        region.enemyConfig = {
            {EnemyType::SkeletonWarrior, 2},
            {EnemyType::SkeletonArcher, 1}
        };
    } else if (dist <= 8) {
        // 更远：困难
        region.enemyConfig = {
            {EnemyType::SkeletonArcher, 2},
            {EnemyType::Giant, 1}
        };
    } else {
        // 极远：非常困难（但增长放缓）
        region.enemyConfig = {
            {EnemyType::Giant, 1 + dist / 8},
            {EnemyType::SkeletonWarrior, 1 + dist / 6},
            {EnemyType::SkeletonArcher, 1 + dist / 8}
        };
    }
}

void MapSystem::updateCamera(sf::View& view, sf::Vector2f playerPos) const {
    view.setCenter(playerPos);
}

std::vector<EnemySpawnInfo> MapSystem::checkAndSpawn(sf::Vector2f playerPos) {
    std::vector<EnemySpawnInfo> result;

    int col = static_cast<int>(std::floor(playerPos.x / REGION_W));
    int row = static_cast<int>(std::floor(playerPos.y / REGION_H));
    int key = regionKey(col, row);

    if (key != lastRegionKey_) {
        lastRegionKey_ = key;

        // 检查当前及相邻区域（3x3范围）
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                MapRegion& region = getOrCreateRegion(col + dc, row + dr);
                if (!region.spawned) {
                    region.spawned = true;
                    exploredCount_++;

                    static std::random_device rd;
                    static std::mt19937 gen(rd());

                    float baseX = region.col * REGION_W;
                    float baseY = region.row * REGION_H;

                    for (const auto& [type, count] : region.enemyConfig) {
                        std::uniform_real_distribution<float> xDist(baseX + 50.f, baseX + REGION_W - 50.f);
                        std::uniform_real_distribution<float> yDist(baseY + 50.f, baseY + REGION_H - 50.f);

                        for (int i = 0; i < count; i++) {
                            result.push_back({type, sf::Vector2f(xDist(gen), yDist(gen))});
                        }
                    }
                }
            }
        }
    }

    return result;
}

float MapSystem::getDifficultyScale() const {
    // 每探索10个区域，难度+0.5，最低1.0
    return 1.0f + exploredCount_ * 0.05f;
}

void MapSystem::drawBackground(sf::RenderWindow& window) const {
    // 基于当前摄像机视口绘制网格线
    sf::View currentView = window.getView();
    sf::Vector2f center = currentView.getCenter();
    sf::Vector2f size = currentView.getSize();

    float left = center.x - size.x / 2.f;
    float right = center.x + size.x / 2.f;
    float top = center.y - size.y / 2.f;
    float bottom = center.y + size.y / 2.f;

    sf::Color gridColor(200, 200, 200, 80);

    // 竖线
    int startCol = static_cast<int>(std::floor(left / REGION_W));
    int endCol = static_cast<int>(std::ceil(right / REGION_W));
    for (int c = startCol; c <= endCol; ++c) {
        float x = c * REGION_W;
        sf::Vertex line[] = {
            sf::Vertex{.position = sf::Vector2f(x, top), .color = gridColor},
            sf::Vertex{.position = sf::Vector2f(x, bottom), .color = gridColor}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    // 横线
    int startRow = static_cast<int>(std::floor(top / REGION_H));
    int endRow = static_cast<int>(std::ceil(bottom / REGION_H));
    for (int r = startRow; r <= endRow; ++r) {
        float y = r * REGION_H;
        sf::Vertex line[] = {
            sf::Vertex{.position = sf::Vector2f(left, y), .color = gridColor},
            sf::Vertex{.position = sf::Vector2f(right, y), .color = gridColor}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}
