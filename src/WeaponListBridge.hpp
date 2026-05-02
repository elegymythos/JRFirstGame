/**
 * @file WeaponListBridge.hpp
 * @brief C++桥接层 - 封装C语言武器链表，提供RAII和类型安全接口
 *
 * 设计目的：
 * 武器系统的底层数据结构用纯C语言实现（WeaponList.c），
 * 使用单链表管理武器节点。此桥接层将C接口封装为C++类，
 * 提供：
 * - RAII：构造时创建链表，析构时自动销毁
 * - 类型安全：C的char[]转为std::string，int布尔转为bool
 * - 移动语义：允许转移链表所有权，禁止拷贝
 * - 装备管理：记录当前装备的武器数据
 *
 * 三种默认武器：
 * - 铁剑（Iron Sword）：近战，伤害15，范围80，攻速2.5
 * - 火焰法器（Flame Artifact）：远程，伤害20，范围300，攻速0.8
 * - 暗影刺刀（Shadow Dagger）：近战，伤害8，范围50，攻速4.0
 */

#pragma once

#include "WeaponList.h"   // C语言武器链表接口
#include <string>
#include <vector>
#include <optional>
#include <functional>

/**
 * @struct WeaponData
 * @brief C++武器数据结构
 *
 * 与C语言的 Weapon 结构体一一对应，但使用 std::string 替代 char[]，
 * bool 替代 int。由 cToCpp() 转换函数桥接。
 */
struct WeaponData {
    std::string name;      ///< 武器名称
    WeaponType type;       ///< 武器类型（WEAPON_SWORD/WEAPON_ARTIFACT/WEAPON_DAGGER）
    int damage;            ///< 基础伤害
    float range;           ///< 攻击范围（像素）
    float attackSpeed;     ///< 每秒攻击次数
    bool isRanged;         ///< 是否远程武器
};

/**
 * @class WeaponListBridge
 * @brief 武器链表C++桥接类
 *
 * RAII管理C链表生命周期，提供类型安全的C++接口。
 * 禁止拷贝（C链表指针不应共享），允许移动。
 */
class WeaponListBridge {
public:
    WeaponListBridge();
    ~WeaponListBridge();

    /* 禁止拷贝（C链表指针唯一） */
    WeaponListBridge(const WeaponListBridge&) = delete;
    WeaponListBridge& operator=(const WeaponListBridge&) = delete;

    /* 允许移动（转移链表所有权） */
    WeaponListBridge(WeaponListBridge&& other) noexcept;
    WeaponListBridge& operator=(WeaponListBridge&& other) noexcept;

    /**
     * @brief 添加武器到链表
     * @param data 武器数据
     * @return true=添加成功
     *
     * 将C++ WeaponData 转为 C Weapon 节点，头插法插入链表。
     */
    bool addWeapon(const WeaponData& data);

    /**
     * @brief 从链表中移除武器
     * @param name 武器名称
     * @return true=移除成功
     *
     * 如果移除的是当前装备的武器，自动取消装备。
     */
    bool removeWeapon(const std::string& name);

    /**
     * @brief 按名称查找武器
     * @param name 武器名称
     * @return 找到的武器数据，或 std::nullopt
     */
    std::optional<WeaponData> findWeapon(const std::string& name) const;

    /**
     * @brief 按类型查找首个匹配武器
     * @param type 武器类型
     * @return 找到的武器数据，或 std::nullopt
     */
    std::optional<WeaponData> findWeaponByType(WeaponType type) const;

    /**
     * @brief 装备武器（按名称）
     * @param name 武器名称
     * @return true=装备成功
     *
     * 在链表中查找并记录装备状态，不修改链表结构。
     */
    bool equipWeapon(const std::string& name);

    /**
     * @brief 获取当前装备的武器
     * @return 装备武器指针，或 nullptr（未装备时）
     */
    const WeaponData* getEquipped() const;

    /**
     * @brief 获取链表中所有武器
     * @return 武器数据列表
     *
     * 使用 weapon_list_foreach 遍历C链表，逐个转为C++ WeaponData。
     */
    std::vector<WeaponData> getAllWeapons() const;

    /**
     * @brief 获取武器数量
     */
    int getCount() const;

    /**
     * @brief 初始化默认武器
     *
     * 添加三种武器：铁剑、火焰法器、暗影刺刀。
     * 每个角色创建时调用一次。
     */
    void initDefaultWeapons();

private:
    WeaponList* list_;          ///< C语言链表指针
    WeaponData equipped_;       ///< 当前装备的武器数据副本
    bool hasEquipped_;          ///< 是否有装备武器

    /**
     * @brief C Weapon 节点转 C++ WeaponData
     * @param weapon C语言武器节点指针
     * @return C++武器数据
     */
    static WeaponData cToCpp(const Weapon* weapon);
};
