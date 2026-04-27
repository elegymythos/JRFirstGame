/**
 * @file WeaponListBridge.hpp
 * @brief C++桥接层 - 封装C语言武器链表，提供RAII和类型安全接口
 */

#pragma once

#include "WeaponList.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

/**
 * @brief C++武器数据结构（与C Weapon对应）
 */
struct WeaponData {
    std::string name;
    WeaponType type;
    int damage;
    float range;
    float attackSpeed;
    bool isRanged;
};

/**
 * @brief 武器链表C++桥接类
 *
 * RAII管理C链表生命周期，提供类型安全的C++接口。
 */
class WeaponListBridge {
public:
    WeaponListBridge();
    ~WeaponListBridge();

    /* 禁止拷贝 */
    WeaponListBridge(const WeaponListBridge&) = delete;
    WeaponListBridge& operator=(const WeaponListBridge&) = delete;

    /* 允许移动 */
    WeaponListBridge(WeaponListBridge&& other) noexcept;
    WeaponListBridge& operator=(WeaponListBridge&& other) noexcept;

    /**
     * @brief 添加武器到链表
     * @param data 武器数据
     * @return true=成功
     */
    bool addWeapon(const WeaponData& data);

    /**
     * @brief 从链表中移除武器
     * @param name 武器名称
     * @return true=成功
     */
    bool removeWeapon(const std::string& name);

    /**
     * @brief 按名称查找武器
     * @param name 武器名称
     * @return 找到的武器数据，或std::nullopt
     */
    std::optional<WeaponData> findWeapon(const std::string& name) const;

    /**
     * @brief 按类型查找首个匹配武器
     * @param type 武器类型
     * @return 找到的武器数据，或std::nullopt
     */
    std::optional<WeaponData> findWeaponByType(WeaponType type) const;

    /**
     * @brief 装备武器（按名称）
     * @param name 武器名称
     * @return true=成功
     */
    bool equipWeapon(const std::string& name);

    /**
     * @brief 获取当前装备的武器
     * @return 装备武器指针，或nullptr
     */
    const WeaponData* getEquipped() const;

    /**
     * @brief 获取链表中所有武器
     * @return 武器数据列表
     */
    std::vector<WeaponData> getAllWeapons() const;

    /**
     * @brief 获取武器数量
     */
    int getCount() const;

    /**
     * @brief 初始化默认武器（铁剑、火焰法器、暗影刺刀）
     */
    void initDefaultWeapons();

private:
    WeaponList* list_;
    WeaponData equipped_;       /* 当前装备的武器数据 */
    bool hasEquipped_;

    /**
     * @brief C Weapon节点转C++ WeaponData
     */
    static WeaponData cToCpp(const Weapon* weapon);
};
