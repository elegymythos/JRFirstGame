/**
 * @file MapSystem.hpp
 * @brief 地图区域与摄像机系统（无限地图）
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <utility>
#include <unordered_map>
#include "GameLogic.hpp"  // EnemyType

/**
 * @brief 地图区域结构体
 */
struct MapRegion {
    int regionId;
    int col, row;                                             // 网格坐标
    std::vector<std::pair<EnemyType, int>> enemyConfig;      // 敌人类型+数量
    bool spawned = false;                                    // 是否已刷新敌人
};

/**
 * @brief 敌人生成信息
 */
struct EnemySpawnInfo {
    EnemyType type;
    sf::Vector2f position;
};

/**
 * @class MapSystem
 * @brief 无限地图区域管理与摄像机跟随
 */
class MapSystem {
public:
    MapSystem();

    void updateCamera(sf::View& view, sf::Vector2f playerPos) const;
    std::vector<EnemySpawnInfo> checkAndSpawn(sf::Vector2f playerPos);

    /**
     * @brief 绘制地图背景（网格线，基于摄像机可见范围）
     */
    void drawBackground(sf::RenderWindow& window) const;

    /**
     * @brief 获取当前难度缩放因子（基于已探索区域数）
     */
    float getDifficultyScale() const;

private:
    static constexpr float REGION_W = 800.f;
    static constexpr float REGION_H = 800.f;

    std::unordered_map<int, MapRegion> regions_;  // key = col + row * 10000
    int lastRegionKey_ = -1;
    int exploredCount_ = 0;

    int regionKey(int col, int row) const { return col + row * 10000; }
    MapRegion& getOrCreateRegion(int col, int row);
    void assignEnemyConfig(MapRegion& region, int col, int row);
};
