/**
 * @file WeaponList.c
 * @brief C语言链表武器系统 - 实现
 *
 */

#include "WeaponList.h"
#include <stdlib.h>
#include <string.h>

/* ======================== 链表操作实现 ======================== */

WeaponList* weapon_list_create(void) {
    WeaponList* list = (WeaponList*)malloc(sizeof(WeaponList));
    if (!list) return NULL;
    list->head = NULL;
    list->count = 0;
    return list;
}

void weapon_list_destroy(WeaponList* list) {
    if (!list) return;
    Weapon* current = list->head;
    while (current) {
        Weapon* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

int weapon_list_insert(WeaponList* list, const Weapon* weapon) {
    if (!list || !weapon) return -1;

    Weapon* node = (Weapon*)malloc(sizeof(Weapon));
    if (!node) return -1;

    /* 深拷贝武器数据 */
    strncpy(node->name, weapon->name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->type = weapon->type;
    node->damage = weapon->damage;
    node->range = weapon->range;
    node->attackSpeed = weapon->attackSpeed;
    node->isRanged = weapon->isRanged;

    /* 头插法 */
    node->next = list->head;
    list->head = node;
    list->count++;

    return 0;
}

int weapon_list_remove(WeaponList* list, const char* name) {
    if (!list || !name) return -1;

    Weapon* prev = NULL;
    Weapon* current = list->head;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                list->head = current->next;
            }
            free(current);
            list->count--;
            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1;  /* 未找到 */
}

Weapon* weapon_list_find(WeaponList* list, const char* name) {
    if (!list || !name) return NULL;

    Weapon* current = list->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

Weapon* weapon_list_find_by_type(WeaponList* list, WeaponType type) {
    if (!list) return NULL;

    Weapon* current = list->head;
    while (current) {
        if (current->type == type) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void weapon_list_foreach(WeaponList* list, void (*callback)(Weapon*, void*), void* userdata) {
    if (!list || !callback) return;

    Weapon* current = list->head;
    while (current) {
        callback(current, userdata);
        current = current->next;
    }
}

int weapon_list_count(WeaponList* list) {
    if (!list) return 0;
    return list->count;
}
