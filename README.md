# JRFirstGame - 一个不成熟的 小游戏

![Build Status](https://github.com/elegymythos/JRFirstGame/actions/workflows/build.yml/badge.svg)

## 🎮 关于本项目

这是一个**大一下学期**学生出于好奇和练习目的制作的 RPG 游戏启动器 Demo。项目实现了基础的登录/注册、单机闯关、联机对战和排行榜功能，但各方面都非常粗糙。

**特别声明**：
- 本项目**大量使用了 AI 工具辅助编程**（代码生成、调试、构建脚本撰写等），因此**代码质量、架构设计均不具有参考价值**。
- 仅作为个人学习 Git、CMake、GitHub Actions 跨平台构建的一次实验记录。
- 如果您是同学或开发者，**请勿将此仓库视为标准实践范例**。

## 🙏 字体致谢

本游戏使用的免费商用字体为 **[Smiley Sans 得意黑](https://github.com/atelier-anchor/smiley-sans)**，由 **Atelier Anchor** 发布。 
版权 © 2022–2024 atelierAnchor，保留字体名称 “Smiley” 和 “得意黑”。
该字体采用 SIL Open Font License 1.1 授权，许可证全文见项目根目录下的
[OFL.txt](./OFL.txt) 或访问 https://scripts.sil.org/OFL。
感谢设计师的慷慨分享，让这个简陋的项目在视觉上能稍微友好一点。

## 🚀 直接下载游玩

项目通过 GitHub Actions 自动构建了 Windows / Linux / macOS 的独立版本，无需安装 SFML 或 SQLite，下载解压即可运行。

[**👉 点击前往 Releases 下载最新版本**](https://github.com/elegymythos/JRFirstGame/releases)

| 平台 | 说明 |
|------|------|
| Windows | 解压后双击 `JRFirstGame.exe`（同目录下需保留 `.dll` 文件） |
| Linux | 解压后运行 `./JRFirstGame` |
| macOS | 解压后双击 `JRFirstGame.app`（若提示损坏，请在终端执行 `xattr -cr JRFirstGame.app`） |

## 🛠️ 技术栈

- **语言**：C++17
- **图形/网络**：[SFML 3.0.2](https://www.sfml-dev.org/)
- **数据存储**：[SQLite3](https://www.sqlite.org/)
- **构建系统**：CMake 3.15+
- **自动化**：GitHub Actions（全平台静态打包）

## 📁 项目结构

项目刻意保持了较少的文件数量以便于手动修改：

## ⚠️ 已知问题与免责声明

- **网络联机**：仅实现了最基础的 UDP 双人对战，无断线重连、无延迟补偿，仅限局域网测试。
- **安全性**：密码哈希使用了简单的 djb2 算法，**绝不可用于生产环境**。
- **代码质量**：存在大量硬编码、紧耦合、全局状态以及不规范的 C++ 写法。
- **跨平台**：macOS 版本未经过深度测试，可能存在路径或权限问题。

## 📝 致未来的自己

这是我大一结束时的作业级项目，它见证了我从“只会写单文件 `main.cpp`”到“能用 CMake 和 CI 把程序分发给朋友玩”的过程。虽然绝大部分代码都由 AI 完成，但过程中的折磨与摸索却是真实的。

如果这个仓库不小心被搜索引擎收录，请记住：**它是一个反面教材**。

---

*Made with ❤️ by a confused freshman, with massive help from AI.*