#!/bin/bash
# ==================================================
#   JRFirstGame RL训练管线 - 一键启动脚本 (Linux/Mac)
# ==================================================

# 切换到脚本所在目录
cd "$(dirname "$0")"

echo "=================================================="
echo "  JRFirstGame RL训练管线 - 一键启动脚本"
echo "=================================================="
echo ""

# 检查虚拟环境是否存在
if [ ! -f "rl_env/bin/activate" ]; then
    echo "[错误] 未找到虚拟环境 rl_env，请先创建虚拟环境:"
    echo "  python3 -m venv rl_env"
    echo "  source rl_env/bin/activate"
    echo "  pip install -r rl/requirements.txt"
    echo ""
    exit 1
fi

# 激活虚拟环境
echo "[信息] 正在激活虚拟环境..."
source rl_env/bin/activate

# 检查gradio是否安装
if ! python -c "import gradio" 2>/dev/null; then
    echo "[警告] 未安装gradio，正在安装..."
    pip install gradio matplotlib
    if [ $? -ne 0 ]; then
        echo "[错误] gradio安装失败，请手动执行: pip install gradio matplotlib"
        exit 1
    fi
fi

# 启动WebUI
echo ""
echo "[信息] 正在启动WebUI..."
echo "[信息] 浏览器将自动打开 http://localhost:7860"
echo "[信息] 按 Ctrl+C 可停止服务"
echo ""

# 延迟3秒后打开浏览器
(sleep 3 && (
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open http://localhost:7860
    elif command -v open >/dev/null 2>&1; then
        open http://localhost:7860
    else
        echo "[提示] 无法自动打开浏览器，请手动访问 http://localhost:7860"
    fi
)) &

# 运行WebUI
python rl/webui.py

# 如果WebUI退出
echo ""
echo "[信息] WebUI已停止"
