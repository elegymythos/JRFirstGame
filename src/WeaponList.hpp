/**
 * @file WeaponList.hpp
 * @brief 武器链表系统 - C++单链表实现
 *
 * 从头构建的单链表武器存储与管理，支持插入、删除、遍历、查找操作。
 * 使用C++实现，保持链表数据结构的极致性能。
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

/* ======================== 武器类型 ======================== */

/**
 * @brief 武器类型枚举
 */
enum class WeaponType {
    Sword    = 0,  // 剑 - 近战/中等伤害/中等范围/中等速度
    Artifact = 1,  // 法器 - 远程/魔法伤害/远范围/慢速度
    Dagger   = 2   // 刺刀 - 近战/低伤害/短范围/快速度
};

/* ======================== 武器数据 ======================== */

/**
 * @brief 武器数据结构体
 */
struct WeaponData {
    std::string name;
    WeaponType type;
    int damage;
    float range;
    float attackSpeed;
    bool isRanged;
};

/* ======================== 链表节点 ======================== */

/**
 * @brief 链表节点（内部使用）
 */
struct WeaponNode {
    WeaponData data;
    WeaponNode* next;
};

/* ======================== 武器链表类 ======================== */

/**
 * @class WeaponList
 * @brief 单链表武器存储与管理
 *
 * 从头构建的C++单链表，头插法，O(n)查找/删除，O(1)插入。
 */
class WeaponList {
public:
    WeaponList();
    ~WeaponList();

    /* 禁止拷贝 */
    WeaponList(const WeaponList&) = delete;
    WeaponList& operator=(const WeaponList&) = delete;

    /* 允许移动 */
    WeaponList(WeaponList&& other) noexcept;
    WeaponList& operator=(WeaponList&& other) noexcept;

    /**
     * @brief 头插法添加武器
     */
    void insert(const WeaponData& data);

    /**
     * @brief 按名称删除武器
     * @return true=成功
     */
    bool remove(const std::string& name);

    /**
     * @brief 按名称查找武器
     */
    std::optional<WeaponData> find(const std::string& name) const;

    /**
     * @brief 按类型查找首个匹配武器
     */
    std::optional<WeaponData> findByType(WeaponType type) const;

    /**
     * @brief 遍历所有武器
     */
    void foreach(std::function<void(const WeaponData&)> callback) const;

    /**
     * @brief 获取所有武器列表
     */
    std::vector<WeaponData> getAll() const;

    /**
     * @brief 获取武器数量
     */
    int count() const;

    /**
     * @brief 初始化默认武器
     */
    void initDefaults();

private:
    WeaponNode* head_;
    int count_;
};

/* ======================== 武器管理器（含装备） ======================== */

/**
 * @class WeaponManager
 * @brief 武器管理器，包含链表+装备逻辑
 */
class WeaponManager {
public:
    WeaponManager();

    /**
     * @brief 装备武器（按名称）
     * @return true=成功
     */
    bool equip(const std::string& name);

    /**
     * @brief 获取当前装备的武器
     */
    const WeaponData* getEquipped() const;

    /**
     * @brief 获取武器链表
     */
    WeaponList& getList();
    const WeaponList& getList() const;

private:
    WeaponList list_;
    WeaponData equipped_;
    bool hasEquipped_ = false;
};
