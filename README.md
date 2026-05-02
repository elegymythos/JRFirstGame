# JRFirstGame

基于 C++17 + SFML 3.x + SQLite3 的 RPG 闯关游戏，支持单机闯关与双人联机合作。

## 功能

- 用户系统：注册/登录，加盐哈希密码存储
- 多角色：每用户最多3个角色槽位，支持创建/选择/删除
- 三种职业：战士（力量加成/铁剑）、法师（魔法加成/火焰法器）、刺客（敏捷加成/暗影刺刀）
- 六大属性：力量/敏捷/魔法/智力/生命/幸运，正态分布随机生成
- 等级系统：经验值升级，每级全属性+2
- 无限地图：基于区域网格，越远敌人越强
- 四种敌人：史莱姆、骷髅刀斧手、骷髅弓箭手、巨人，各有不同AI
- 武器系统：C语言单链表 + C++桥接层，三种武器
- 战斗系统：扇形近战/远程投射物，刀光动画，暴击机制
- 翻滚系统：Shift键翻滚，翻滚期间无敌
- 掉落物：击杀敌人概率掉落血瓶和属性提升道具
- 分数加成：每50分增加10%全属性加成，上限200%
- 阶段胜利：分数达到阈值时强化角色，回满血
- 暂停/存档：Esc暂停，P键中途存档
- 双人闯关：UDP联机合作，主机-客户端架构
- 排行榜：按等级和分数排名
- 国际化：中/英双语切换（L键或右上角按钮）

## 技术栈

| 技术 | 用途 |
|------|------|
| C++17 | 核心语言 |
| SFML 3.x | 图形/窗口/网络 |
| SQLite3 | 数据存储（源码编译） |
| CMake 3.15+ | 构建系统 |
| GitHub Actions | 跨平台CI/CD |

## 项目结构

```
src/
├── main.cpp              程序入口与主循环
├── ViewBase.hpp/cpp      视图基类（语言切换按钮）
├── UIWidgets.hpp/cpp     UI控件（Button/InputField/AttributePanel）
├── AuthView.hpp/cpp      登录/注册视图
├── MenuView.hpp/cpp      主菜单/模式选择视图
├── CharacterView.hpp/cpp 角色创建/选择视图
├── GameView.hpp/cpp      单机闯关游戏视图
├── OnlineView.hpp/cpp    联机大厅/联机游戏视图
├── RankingsView.hpp/cpp  排行榜视图
├── GameLogic.hpp/cpp     游戏逻辑（属性/职业/等级/敌人/实体）
├── MapSystem.hpp/cpp     无限地图区域与摄像机系统
├── Network.hpp/cpp       UDP网络通信
├── Database.hpp/cpp      SQLite数据库操作
├── WeaponList.h/c        C语言武器链表（纯C实现）
├── WeaponListBridge.hpp/cpp C++桥接层（RAII封装C链表）
├── I18n.hpp/cpp          国际化系统（中/英翻译表）
├── Utils.hpp/cpp         工具函数（盐值/哈希）
└── embedded_font.hpp     嵌入字体数据（自动生成）
```

## 下载

[Releases 页面](https://github.com/elegymythos/JRFirstGame/releases)

| 平台 | 说明 |
|------|------|
| Windows | 解压后双击 JRFirstGame.exe（保留同目录 .dll 文件） |
| Linux | 解压后运行 ./JRFirstGame |
| macOS | 解压后双击 JRFirstGame.app（若提示损坏：xattr -cr JRFirstGame.app） |

## 从源码构建

```bash
# 安装 SFML 3.x
# Linux:  sudo apt install libsfml-all-dev
# macOS:  brew install sfml
# Windows: 从 https://www.sfml-dev.org/ 下载

cmake -B build
cmake --build build -j4
./build/JRFirstGame
```

## 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| Space / 鼠标左键 | 攻击 |
| Shift | 翻滚（无敌） |
| Tab | 属性面板 |
| Esc | 暂停 |
| P | 存档 |
| L | 切换语言 |

## 字体

使用 [Smiley Sans 得意黑](https://github.com/atelier-anchor/smiley-sans) 字体，SIL Open Font License 1.1 授权。

## 已知限制

- 联机模式仅支持局域网UDP，无!断线重连和延迟补偿
- 密码哈希使用SHA-256算法，不可用于生产环境
- macOS版本未经深度测试
