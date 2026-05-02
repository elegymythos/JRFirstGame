/**
 * @file MapSystem.hpp
 * @brief 地图区域与摄像机系统（无限地图）
 *
 * 基于区域网格实现无限地图：
 * - 世界被划分为 800x800 像素的区域（MapRegion）
 * - 玩家进入新区域时，自动检查周围3x3范围并生成敌人
 * - 每个区域的敌人配置由其到原点的曼哈顿距离决定，越远越难
 * - 已探索区域数影响全局难度缩放因子
 * - 摄像机始终跟随玩家位置
 * - 背景绘制半透明网格线标识区域边界
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <utility>
#include <unordered_map>
#include "GameLogic.hpp"  // EnemyType

/**
 * @struct MapRegion
 * @brief 地图区域
 *
 * 每个区域有唯一的 regionId（由 col + row*100000 计算），
 * 包含敌人配置（类型+数量）和是否已刷新标记。
 */
struct MapRegion {
    int regionId;                                       ///< 区域唯一ID
    int col, row;                                       ///< 网格坐标
    std::vector<std::pair<EnemyType, int>> enemyConfig; ///< 敌人类型+数量配置
    bool spawned = false;                               ///< 是否已刷新敌人
};

/**
 * @struct EnemySpawnInfo
 * @brief 敌人生成信息
 *
 * 由 checkAndSpawn() 返回，告诉调用者需要在哪里生成什么类型的敌人。
 */
struct EnemySpawnInfo {
    EnemyType type;              ///< 敌人类型
    sf::Vector2f position;       ///< 生成位置（世界坐标）
};

/**
 * @class MapSystem
 * @brief 无限地图区域管理与摄像机跟随
 *
 * 核心方法：
 * - checkAndSpawn()：检查玩家所在区域及相邻区域，返回需要生成的敌人列表
 * - updateCamera()：将摄像机中心设为玩家位置
 * - drawBackground()：绘制区域网格线
 * - getDifficultyScale()：返回基于已探索区域数的难度缩放因子
 */
class MapSystem {
public:
    MapSystem();

    /// 更新摄像机位置（跟随玩家）
    void updateCamera(sf::View& view, sf::Vector2f playerPos) const;
    /**
     * @brief 检查并生成敌人
     * @param playerPos 玩家当前位置
     * @return 需要生成的敌人列表
     *
     * 当玩家进入新区域时，检查周围3x3范围，
     * 对未刷新的区域按配置生成敌人，位置在区域内随机分布。
     */
    std::vector<EnemySpawnInfo> checkAndSpawn(sf::Vector2f playerPos);

    /// 绘制地图背景网格线（基于摄像机可见范围）
    void drawBackground(sf::RenderWindow& window) const;

    /**
     * @brief 获取当前难度缩放因子
     * @return 1.0 + exploredCount * 0.05
     *
     * 每探索一个新区域，难度增加5%。
     */
    float getDifficultyScale() const;

private:
    static constexpr float REGION_W = 800.f;  ///< 区域宽度（像素）
    static constexpr float REGION_H = 800.f;  ///< 区域高度（像素）

    std::unordered_map<int, MapRegion> regions_;  ///< 所有已创建的区域（key=regionId）
    int lastRegionKey_ = -1;                      ///< 上次玩家所在区域ID
    int exploredCount_ = 0;                       ///< 已探索区域总数

    /// 计算区域ID：col + row * 100000
    int regionKey(int col, int row) const { return col + row * 100000; }
    /// 获取或创建区域（不存在则创建并分配敌人配置）
    MapRegion& getOrCreateRegion(int col, int row);
    /// 根据区域到原点的距离分配敌人配置
    void assignEnemyConfig(MapRegion& region, int col, int row);
};
