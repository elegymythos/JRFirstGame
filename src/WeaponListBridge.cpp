/**
 * @file WeaponListBridge.cpp
 * @brief C++桥接层实现
 */

#include "WeaponListBridge.hpp"
#include <cstring>

/* ======================== 构造/析构 ======================== */

WeaponListBridge::WeaponListBridge()
    : list_(weapon_list_create())
    , equipped_()
    , hasEquipped_(false)
{
}

WeaponListBridge::~WeaponListBridge() {
    if (list_) {
        weapon_list_destroy(list_);
        list_ = nullptr;
    }
}

WeaponListBridge::WeaponListBridge(WeaponListBridge&& other) noexcept
    : list_(other.list_)
    , equipped_(std::move(other.equipped_))
    , hasEquipped_(other.hasEquipped_)
{
    other.list_ = nullptr;
    other.hasEquipped_ = false;
}

WeaponListBridge& WeaponListBridge::operator=(WeaponListBridge&& other) noexcept {
    if (this != &other) {
        if (list_) weapon_list_destroy(list_);
        list_ = other.list_;
        equipped_ = std::move(other.equipped_);
        hasEquipped_ = other.hasEquipped_;
        other.list_ = nullptr;
        other.hasEquipped_ = false;
    }
    return *this;
}

/* ======================== 武器操作 ======================== */

bool WeaponListBridge::addWeapon(const WeaponData& data) {
    if (!list_) return false;

    Weapon w;
    std::strncpy(w.name, data.name.c_str(), sizeof(w.name) - 1);
    w.name[sizeof(w.name) - 1] = '\0';
    w.type = data.type;
    w.damage = data.damage;
    w.range = data.range;
    w.attackSpeed = data.attackSpeed;
    w.isRanged = data.isRanged ? 1 : 0;
    w.next = nullptr;

    return weapon_list_insert(list_, &w) == 0;
}

bool WeaponListBridge::removeWeapon(const std::string& name) {
    if (!list_) return false;

    /* 如果移除的是当前装备，取消装备 */
    if (hasEquipped_ && equipped_.name == name) {
        hasEquipped_ = false;
    }

    return weapon_list_remove(list_, name.c_str()) == 0;
}

std::optional<WeaponData> WeaponListBridge::findWeapon(const std::string& name) const {
    if (!list_) return std::nullopt;

    Weapon* w = weapon_list_find(list_, name.c_str());
    if (w) return cToCpp(w);
    return std::nullopt;
}

std::optional<WeaponData> WeaponListBridge::findWeaponByType(WeaponType type) const {
    if (!list_) return std::nullopt;

    Weapon* w = weapon_list_find_by_type(list_, type);
    if (w) return cToCpp(w);
    return std::nullopt;
}

bool WeaponListBridge::equipWeapon(const std::string& name) {
    auto found = findWeapon(name);
    if (found) {
        equipped_ = *found;
        hasEquipped_ = true;
        return true;
    }
    return false;
}

const WeaponData* WeaponListBridge::getEquipped() const {
    if (hasEquipped_) return &equipped_;
    return nullptr;
}

std::vector<WeaponData> WeaponListBridge::getAllWeapons() const {
    std::vector<WeaponData> result;
    if (!list_) return result;

    struct CallbackData {
        std::vector<WeaponData>* vec;
    };

    CallbackData cbd{&result};

    weapon_list_foreach(list_, [](Weapon* w, void* userdata) {
        CallbackData* data = static_cast<CallbackData*>(userdata);
        data->vec->push_back(WeaponListBridge::cToCpp(w));
    }, &cbd);

    return result;
}

int WeaponListBridge::getCount() const {
    if (!list_) return 0;
    return weapon_list_count(list_);
}

/* ======================== 默认武器初始化 ======================== */

void WeaponListBridge::initDefaultWeapons() {
    /* 铁剑 - 近战/中等伤害/中等范围/中等速度 */
    addWeapon(WeaponData{"Iron Sword", WEAPON_SWORD, 15, 80.0f, 1.5f, false});

    /* 火焰法器 - 远程/魔法伤害/远范围/慢速度 */
    addWeapon(WeaponData{"Flame Artifact", WEAPON_ARTIFACT, 20, 300.0f, 0.8f, true});

    /* 暗影刺刀 - 近战/低伤害/短范围/快速度 */
    addWeapon(WeaponData{"Shadow Dagger", WEAPON_DAGGER, 8, 50.0f, 3.0f, false});
}

/* ======================== 辅助函数 ======================== */

WeaponData WeaponListBridge::cToCpp(const Weapon* weapon) {
    return WeaponData{
        std::string(weapon->name),
        weapon->type,
        weapon->damage,
        weapon->range,
        weapon->attackSpeed,
        weapon->isRanged != 0
    };
}
