# JRFirstGame

基于 C++17 + SFML 3.x + SQLite3 的塞尔达风格 RPG 闯关游戏，支持单机闯关、双人联机合作与 AI 自动游玩。

## 功能特性

### 核心系统

- 用户系统：注册/登录，加盐哈希密码存储
- 多角色：每用户最多 3 个角色槽位，支持创建/选择/删除
- 五种职业：战士、法师、刺客
- 六大属性：力量/敏捷/魔法/智力/生命/幸运，正态分布随机生成
- 等级系统：经验值升级，每级全属性 +2
- 武器系统：C 语言单链表 + C++ 桥接层，三种武器（铁剑/炎之法器/暗影匕首）

### 游戏玩法

- 无限地图：基于区域网格，越远敌人越强
- 五种敌人：史莱姆、骷髅刀斧手、骷髅弓箭手、巨人、Boss，各有不同 AI 和攻击方式
- 战斗系统：扇形近战/远程投射物，刀光动画，暴击机制
- 翻滚系统：Shift 键翻滚，翻滚期间无敌
- 掉落物：击杀敌人概率掉落血瓶和属性提升道具（6 种）
- 分数加成：每 50 分增加 10% 全属性加成，上限 200%
- 阶段胜利：5 个阶段（100/1000/2000/5000/10000 分），每阶段强化角色并回满血
- 暂停/存档：Esc 暂停，P 键中途存档

### 联机与社交

- 双人闯关：UDP 联机合作，主机-客户端架构
- 排行榜：按等级和分数排名（Top 10）
- 国际化：中/英双语切换（L 键）

### AI 自动游玩

- 强化学习 PPO 策略，330 维观测空间，12 离散动作空间
- ONNX Runtime C++ 推理集成，`--ai` 命令行启动
- 奖励塑形：得分/击杀/闪避/风筝/拾取/寻敌等多维度引导
- WebUI 训练界面（Gradio）

## 技术栈

| 技术 | 用途 |
|------|------|
| C++17 | 核心语言 |
| SFML 3.0.2 | 图形/窗口/网络/音频 |
| SQLite3 | 数据存储（源码编译） |
| CMake 3.15+ | 构建系统 |
| GitHub Actions | 跨平台 CI/CD（Windows/Linux/macOS） |
| Python 3.11+ | RL 训练（Gymnasium + Stable-Baselines3） |
| PyTorch 2.5+ | PPO 策略网络训练（CUDA 加速） |
| ONNX Runtime 1.19+ | C++ 端模型推理 |

## 项目结构

```
src/
├── main.cpp              程序入口与主循环
├── ViewBase.hpp/cpp      视图基类（语言切换按钮）
├── UIWidgets.hpp/cpp     UI 控件（Button/InputField/AttributePanel）
├── AuthView.hpp/cpp      登录/注册视图
├── MenuView.hpp/cpp      主菜单/模式选择视图
├── CharacterView.hpp/cpp 角色创建/选择视图
├── GameView.hpp/cpp      单机闯关游戏视图
├── OnlineView.hpp/cpp    联机大厅/联机游戏视图
├── RankingsView.hpp/cpp  排行榜视图
├── GameLogic.hpp/cpp     游戏逻辑（属性/职业/等级/敌人/实体）
├── MapSystem.hpp/cpp     无限地图区域与摄像机系统
├── Network.hpp/cpp       UDP 网络通信
├── Database.hpp/cpp      SQLite 数据库操作
├── WeaponList.h/c        C 语言武器链表（纯 C 实现）
├── WeaponListBridge.hpp/cpp C++ 桥接层（RAII 封装 C 链表）
├── I18n.hpp/cpp          国际化系统（中/英翻译表）
├── Utils.hpp/cpp         工具函数（盐值/哈希）
├── AudioManager.hpp/cpp  音频管理器（BGM + SFX）
├── AIController.hpp/cpp  AI 观测编码与动作解码（OBS_DIM=330）
├── AIInference.hpp/cpp   ONNX Runtime 推理器
└── embedded_font.hpp     嵌入字体数据（自动生成）

rl/
├── game_env.py           Gymnasium 环境封装（330 维观测）
├── train.py              PPO 训练脚本（GPU + 线性 LR 衰减）
├── play.py               pygame 可视化
├── export_onnx_standalone.py ONNX 独立导出
├── webui.py              Gradio WebUI 训练界面
├── run_v24.py            v24 训练启动脚本
├── run_v25.py            v25 训练启动脚本
└── requirements.txt      Python 依赖

docs/
└── FEATURES.md           功能实现详细清单
```

## AI 自动游玩

### 快速开始

```bash
# 1. 一键启动 WebUI 训练界面（Windows）
start_training.bat

# 2. 或手动训练
rl_env\Scripts\python.exe rl\train.py --algo ppo --mode train ^
    --total-timesteps 4000000 --difficulty 2 --n-envs 4

# 3. 导出 ONNX 模型
rl_env\Scripts\python.exe rl\export_onnx_standalone.py

# 4. 启动 AI 模式
build\JRFirstGame.exe --ai
build\JRFirstGame.exe --ai --ai-model rl\models\ppo_policy.onnx
```

### RL 训练架构

| 组件 | 文件 | 说明 |
|------|------|------|
| Gymnasium 环境 | `rl/game_env.py` | 330 维观测空间，12 离散动作空间 |
| PPO 训练 | `rl/train.py` | PPO 算法，[256,256] 网络，线性 LR 衰减，CUDA |
| ONNX 导出 | `rl/export_onnx_standalone.py` | 策略网络导出为 ONNX 格式 |
| WebUI | `rl/webui.py` | Gradio 训练/评估/导出/监控界面 |
| C++ 推理 | `src/AIInference.hpp/cpp` | ONNX Runtime C++ 推理 |
| C++ 控制 | `src/AIController.hpp/cpp` | 观测编码 + 动作解码 |

### 观测空间（330 维）

| 模块 | 维度 | 内容 |
|------|------|------|
| PLAYER | 20 | 位置/速度/血量/攻击范围/冷却/是否翻滚/最近敌距离/附近敌人数/... |
| ENEMY×20 | 200 | 类型/相对位置/血量/距离/是否远程/速度/攻击冷却/是否在攻击范围内 |
| PROJECTILE×10 | 60 | 相对位置/方向/是否友方/到达时间估计 |
| DROP×10 | 40 | 相对位置/类型 |
| DANGER_GRID | 8 | 8 方向弹幕危险度 |
| GLOBAL | 2 | 游戏时间/难度缩放 |

### 动作空间（12 个离散动作）

| 动作 ID | 含义 | 动作 ID | 含义 |
|--------|------|--------|------|
| 0 | 停留 | 5 | 左上移动 |
| 1 | 上移 | 6 | 右上移动 |
| 2 | 下移 | 7 | 左下移动 |
| 3 | 左移 | 8 | 右下移动 |
| 4 | 右移 | 9 | 攻击最近敌人（自动边打边走） |
| - | - | 10 | 翻滚 |
| - | - | 11 | 拾取最近掉落物 |

### 依赖安装

```bash
# 创建虚拟环境
python -m venv rl_env
rl_env\Scripts\activate

pip install -r rl/requirements.txt
# GPU 版本 PyTorch:
pip install torch --index-url https://download.pytorch.org/whl/cu124
```

## 下载

[Releases 页面](https://github.com/elegymythos/JRFirstGame/releases)

| 平台 | 说明 |
|------|------|
| Windows | 解压后双击 JRFirstGame.exe（保留同目录 .dll 文件） |
| Linux | 解压后运行 `./JRFirstGame` |
| macOS | 解压后双击 JRFirstGame.app（若提示损坏：`xattr -cr JRFirstGame.app`） |

## 从源码构建

```bash
# 安装 SFML 3.x
# Linux:  sudo apt install libsfml-all-dev
# macOS:  brew install sfml
# Windows: 从 https://www.sfml-dev.org/ 下载，或编译 SFML-mingw/

# 配置（AI 默认启用，需要 onnxruntime/ 目录）
cmake -B build -DCMAKE_PREFIX_PATH=<SFML路径>

# 禁用 AI（不需要 ONNX Runtime）
cmake -B build -DENABLE_AI=OFF

# 编译运行
cmake --build build -j4
./build/JRFirstGame
```

### 构建选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `EMBED_RESOURCES` | ON | 将字体嵌入可执行文件 |
| `ENABLE_AI` | ON | 启用 AI 模式（需要 ONNX Runtime） |

## 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| Space / 鼠标左键 | 攻击 |
| Shift | 翻滚（无敌） |
| E | 交互/拾取 |
| Tab | 属性面板 |
| Esc | 暂停 |
| P | 存档 |
| L | 切换语言 |

## 训练版本历史

| 版本 | 策略 | Best Score | 备注 |
|------|------|-----------|------|
| v16 | diff=2+ 自适应 | 4050 | 最稳定基线（OBS=278），C++ 实测不行 |
| v22 | 预判闪避+范围惩罚 | 2657 | 不会躲弹幕/误判范围/打完不动 |
| v23 | OBS 扩展到 330 维 | 2692 | 塑形奖励过多，score 下降 |
| v24 | 降低塑形 50%+score 5x | **3164** | 当前最佳，Mean 1568 |
| v25 | ent_coef 0.05+batch 1024 | 2535 | 过早收敛，效果退步 |

## 字体

使用 [Smiley Sans 得意黑](https://github.com/atelier-anchor/smiley-sans) 字体，SIL Open Font License 1.1 授权。

## 已知限制

- 联机模式仅支持局域网 UDP，无断线重连和延迟补偿
- 密码哈希使用 SHA-256 算法，不可用于生产环境
- macOS 版本未经深度测试
- AI 模型训练存在 sim-to-real gap，Python env 与 C++ 仍有细微差异
