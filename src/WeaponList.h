/**
 * @file WeaponList.h
 * @brief C语言链表武器系统 - 头文件
 *
 */

#ifndef WEAPON_LIST_H
#define WEAPON_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* ======================== 类型定义 ======================== */

/**
 * @brief 武器类型枚举
 */
typedef enum {
    WEAPON_SWORD    = 0,  /* 剑 - 近战/中等伤害/中等范围/中等速度 */
    WEAPON_ARTIFACT = 1,  /* 法器 - 远程/魔法伤害/远范围/慢速度 */
    WEAPON_DAGGER   = 2   /* 刺刀 - 近战/低伤害/短范围/快速度 */
} WeaponType;

/**
 * @brief 武器节点结构体（单链表节点）
 */
typedef struct Weapon {
    char name[32];          /* 武器名称 */
    WeaponType type;        /* 武器类型 */
    int damage;             /* 基础伤害 */
    float range;            /* 攻击范围 */
    float attackSpeed;      /* 每秒攻击次数 */
    int isRanged;           /* 0=近战, 1=远程 */
    struct Weapon* next;    /* 链表下一节点 */
} Weapon;

/**
 * @brief 武器链表结构体
 */
typedef struct {
    Weapon* head;   /* 链表头节点 */
    int count;      /* 节点计数 */
} WeaponList;

/* ======================== 链表操作 ======================== */

/**
 * @brief 创建并初始化武器链表
 * @return 新链表指针，失败返回NULL
 */
WeaponList* weapon_list_create(void);

/**
 * @brief 销毁武器链表，释放所有节点和链表本身
 * @param list 链表指针
 */
void weapon_list_destroy(WeaponList* list);

/**
 * @brief 向链表头部插入武器（深拷贝）
 * @param list 链表指针
 * @param weapon 要插入的武器数据（会被深拷贝）
 * @return 0=成功, -1=失败
 */
int weapon_list_insert(WeaponList* list, const Weapon* weapon);

/**
 * @brief 按名称从链表中删除武器
 * @param list 链表指针
 * @param name 武器名称
 * @return 0=成功, -1=未找到
 */
int weapon_list_remove(WeaponList* list, const char* name);

/**
 * @brief 按名称查找武器
 * @param list 链表指针
 * @param name 武器名称
 * @return 找到的武器节点指针，未找到返回NULL
 */
Weapon* weapon_list_find(WeaponList* list, const char* name);

/**
 * @brief 按类型查找首个匹配的武器
 * @param list 链表指针
 * @param type 武器类型
 * @return 找到的武器节点指针，未找到返回NULL
 */
Weapon* weapon_list_find_by_type(WeaponList* list, WeaponType type);

/**
 * @brief 遍历链表，对每个节点调用回调函数
 * @param list 链表指针
 * @param callback 回调函数，参数为武器节点和用户数据
 * @param userdata 传递给回调的用户数据
 */
void weapon_list_foreach(WeaponList* list, void (*callback)(Weapon*, void*), void* userdata);

/**
 * @brief 获取链表中的武器数量
 * @param list 链表指针
 * @return 武器数量
 */
int weapon_list_count(WeaponList* list);

#ifdef __cplusplus
}
#endif

#endif /* WEAPON_LIST_H */
