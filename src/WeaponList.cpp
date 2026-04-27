/**
 * @file WeaponList.cpp
 * @brief 武器链表系统实现
 */

#include "WeaponList.hpp"

/* ======================== WeaponList 实现 ======================== */

WeaponList::WeaponList() : head_(nullptr), count_(0) {}

WeaponList::~WeaponList() {
    WeaponNode* current = head_;
    while (current) {
        WeaponNode* next = current->next;
        delete current;
        current = next;
    }
}

WeaponList::WeaponList(WeaponList&& other) noexcept
    : head_(other.head_), count_(other.count_) {
    other.head_ = nullptr;
    other.count_ = 0;
}

WeaponList& WeaponList::operator=(WeaponList&& other) noexcept {
    if (this != &other) {
        WeaponNode* current = head_;
        while (current) {
            WeaponNode* next = current->next;
            delete current;
            current = next;
        }
        head_ = other.head_;
        count_ = other.count_;
        other.head_ = nullptr;
        other.count_ = 0;
    }
    return *this;
}

void WeaponList::insert(const WeaponData& data) {
    WeaponNode* node = new WeaponNode{data, head_};
    head_ = node;
    count_++;
}

bool WeaponList::remove(const std::string& name) {
    WeaponNode* prev = nullptr;
    WeaponNode* current = head_;

    while (current) {
        if (current->data.name == name) {
            if (prev) prev->next = current->next;
            else head_ = current->next;
            delete current;
            count_--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

std::optional<WeaponData> WeaponList::find(const std::string& name) const {
    WeaponNode* current = head_;
    while (current) {
        if (current->data.name == name) return current->data;
        current = current->next;
    }
    return std::nullopt;
}

std::optional<WeaponData> WeaponList::findByType(WeaponType type) const {
    WeaponNode* current = head_;
    while (current) {
        if (current->data.type == type) return current->data;
        current = current->next;
    }
    return std::nullopt;
}

void WeaponList::foreach(std::function<void(const WeaponData&)> callback) const {
    WeaponNode* current = head_;
    while (current) {
        callback(current->data);
        current = current->next;
    }
}

std::vector<WeaponData> WeaponList::getAll() const {
    std::vector<WeaponData> result;
    result.reserve(count_);
    WeaponNode* current = head_;
    while (current) {
        result.push_back(current->data);
        current = current->next;
    }
    return result;
}

int WeaponList::count() const { return count_; }

void WeaponList::initDefaults() {
    insert(WeaponData{"Iron Sword",      WeaponType::Sword,    15, 80.0f,  1.5f, false});
    insert(WeaponData{"Flame Artifact",  WeaponType::Artifact, 20, 300.0f, 0.8f, true});
    insert(WeaponData{"Shadow Dagger",   WeaponType::Dagger,   8,  50.0f,  3.0f, false});
}

/* ======================== WeaponManager 实现 ======================== */

WeaponManager::WeaponManager() {
    list_.initDefaults();
}

bool WeaponManager::equip(const std::string& name) {
    auto found = list_.find(name);
    if (found) {
        equipped_ = *found;
        hasEquipped_ = true;
        return true;
    }
    return false;
}

const WeaponData* WeaponManager::getEquipped() const {
    return hasEquipped_ ? &equipped_ : nullptr;
}

WeaponList& WeaponManager::getList() { return list_; }
const WeaponList& WeaponManager::getList() const { return list_; }
