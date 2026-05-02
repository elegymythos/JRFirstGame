/**
 * @file I18n.cpp
 * @brief 国际化系统实现
 */

#include "I18n.hpp"

I18n& I18n::instance() {
    static I18n inst;
    return inst;
}

I18n::I18n() {
    initTranslations();
}

void I18n::setLanguage(Language lang) { current_ = lang; }
Language I18n::getLanguage() const { return current_; }

void I18n::toggleLanguage() {
    current_ = (current_ == Language::EN) ? Language::ZH : Language::EN;
}

std::string I18n::t(const std::string& key) const {
    auto it = translations_.find(key);
    if (it == translations_.end()) return key;
    return (current_ == Language::EN) ? it->second.first : it->second.second;
}

std::string I18n::getLanguageName() const {
    return (current_ == Language::EN) ? "EN" : "中文";
}

void I18n::initTranslations() {
    // 通用
    translations_["back"] = {"Back", "返回"};
    translations_["confirm"] = {"Confirm", "确认"};
    translations_["restart"] = {"Restart", "重新开始"};
    translations_["exit"] = {"Exit", "退出"};

    // 主菜单
    translations_["main_title"] = {"JRFirstGame", "JRFirstGame"};
    translations_["login"] = {"Log In", "登录"};
    translations_["signup"] = {"Sign Up", "注册"};

    // 登录
    translations_["login_title"] = {"LOGIN", "登录"};
    translations_["invalid_cred"] = {"Invalid credentials!", "用户名或密码错误！"};
    translations_["username"] = {"Username", "用户名"};
    translations_["password"] = {"Password", "密码"};

    // 注册
    translations_["signup_title"] = {"SIGN UP", "注册"};
    translations_["empty_field"] = {"Username/Password empty!", "用户名/密码不能为空！"};
    translations_["user_exists"] = {"User exists!", "用户名已存在！"};
    translations_["reg_success"] = {"Registered! Please login.", "注册成功！请登录。"};
    translations_["db_error"] = {"Database error!", "数据库错误！"};

    // 模式选择
    translations_["welcome"] = {"Welcome, Hero!\nPreparing your adventure...", "欢迎，英雄！\n准备你的冒险..."};
    translations_["choose_mode"] = {"Choose Your Mode", "选择模式"};
    translations_["create_char"] = {"Create Character", "创建角色"};
    translations_["level_mode"] = {"Level Mode", "闯关模式"};
    translations_["coop_mode"] = {"Coop Mode", "双人闯关"};
    translations_["rankings"] = {"Rankings", "排行榜"};

    // 角色创建
    translations_["char_create_title"] = {"Create Character", "创建角色"};
    translations_["attributes"] = {"Attributes (random):", "属性（随机）："};
    translations_["total"] = {"Total:", "总计："};
    translations_["warrior"] = {"Warrior", "战士"};
    translations_["mage"] = {"Mage", "法师"};
    translations_["assassin"] = {"Assassin", "刺客"};
    translations_["reroll"] = {"Reroll", "重新随机"};
    translations_["create_char_title"] = {"Create Character", "创建角色"};
    translations_["select_char_title"] = {"Select Character", "选择角色"};
    translations_["select_class_first"] = {"Please select a class first!", "请先选择职业！"};
    translations_["slots_full"] = {"All character slots are full!", "角色槽位已满！"};
    translations_["class_label"] = {"Class:", "职业："};
    translations_["bonus"] = {"Bonus:", "加成："};

    // 属性名
    translations_["str"] = {"STR", "力量"};
    translations_["agi"] = {"AGI", "敏捷"};
    translations_["mag"] = {"MAG", "魔法"};
    translations_["int"] = {"INT", "智力"};
    translations_["vit"] = {"VIT", "生命"};
    translations_["luk"] = {"LUK", "幸运"};
    translations_["attr_random"] = {"Attributes (random):", "属性（随机）:"};

    // 属性面板
    translations_["attr_panel_title"] = {"=== Attributes (Tab) ===", "=== 属性面板 (Tab) ==="};
    translations_["level"] = {"Level:", "等级："};
    translations_["exp"] = {"EXP:", "经验："};
    translations_["score"] = {"Score:", "分数："};
    translations_["weapon"] = {"Weapon:", "武器："};
    translations_["none"] = {"None", "无"};

    // 游戏内
    translations_["game_over"] = {"GAME OVER\nPress Restart", "游戏结束\n按重新开始"};
    translations_["back_menu"] = {"Back to Menu", "返回菜单"};
    translations_["hp"] = {"HP:", "血量："};
    translations_["lv"] = {"Lv:", "等级："};
    translations_["controls"] = {"[WASD]Move [Shift]Roll [Space/Click]Attack [Tab]Stats [L]Lang [Esc]Pause", "[WASD]移动 [Shift]翻滚 [空格/点击]攻击 [Tab]属性 [L]语言 [Esc]暂停"};
    translations_["time"] = {"Time:", "时间："};
    translations_["range"] = {"Range:", "范围："};
    translations_["paused"] = {"PAUSED", "游戏暂停"};
    translations_["resume"] = {"Resume", "继续游戏"};
    translations_["stage_victory"] = {"STAGE VICTORY!", "阶段胜利！"};
    translations_["stage_victory_final"] = {"FINAL VICTORY!", "最终胜利！"};
    translations_["continue"] = {"Continue", "继续游戏"};

    // 排行榜
    translations_["rankings_title"] = {"Rankings (Top 10)", "排行榜 (前10)"};
    translations_["no_rankings"] = {"No rankings yet.", "暂无排行数据。"};

    // 联机
    translations_["server_ip"] = {"Server IP:", "服务器IP："};
    translations_["port"] = {"Port:", "端口："};
    translations_["host_coop"] = {"Host Coop", "创建房间"};
    translations_["join_coop"] = {"Join Coop", "加入房间"};
    translations_["fill_port"] = {"Fill Port", "请填写端口"};
    translations_["invalid_port"] = {"Invalid port", "无效端口"};
    translations_["fill_ip_port"] = {"Fill IP and Port", "请填写IP和端口"};
    translations_["invalid_ip"] = {"Invalid IP", "无效IP"};
    translations_["connecting"] = {"Connecting...", "连接中..."};
    translations_["connected"] = {"Connected!", "已连接！"};
    translations_["conn_failed"] = {"Connection failed", "连接失败"};
    translations_["you_lost"] = {"You Lost!", "你输了！"};
    translations_["server_start_failed"] = {"Failed to start server on port ", "无法在端口 "};
    translations_["server_started"] = {"Server started on port ", "服务器已启动，端口 "};
    translations_["connecting_server"] = {"Connecting to server...", "正在连接服务器..."};

    // 存档
    translations_["save_game"] = {"Save Game", "保存游戏"};
    translations_["game_saved"] = {"Game Saved!", "游戏已保存！"};
    translations_["load_char"] = {"Load Character", "选择角色"};
    translations_["char_slot"] = {"Slot", "角色槽"};
    translations_["delete_char"] = {"Delete", "删除"};
    translations_["select_char"] = {"Select", "选择"};
    translations_["empty_slot"] = {"Empty Slot", "空槽位"};
    translations_["new_char"] = {"New Character", "新建角色"};

    // 武器名
    translations_["iron_sword"] = {"Iron Sword", "铁剑"};
    translations_["flame_artifact"] = {"Flame Artifact", "火焰法器"};
    translations_["shadow_dagger"] = {"Shadow Dagger", "暗影刺刀"};

    // 敌人名
    translations_["slime"] = {"Slime", "史莱姆"};
    translations_["sk_warrior"] = {"Sk.Warrior", "骷髅刀斧手"};
    translations_["sk_archer"] = {"Sk.Archer", "骷髅弓箭手"};
    translations_["giant"] = {"Giant", "巨人"};
}
