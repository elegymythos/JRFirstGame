"""
RL训练管线 WebUI - 基于Gradio的交互式训练控制界面
提供训练、评估、导出ONNX、监控四个功能标签页
启动方式: python rl/webui.py
访问地址: http://localhost:7860
"""

import os
import sys
import time
import json
import threading
import subprocess
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

def _setup_chinese_font():
    candidates = ["SimHei", "Microsoft YaHei", "STHeiti", "WenQuanYi Micro Hei",
                  "Noto Sans CJK SC", "Arial Unicode MS"]
    available = {f.name for f in fm.fontManager.ttflist}
    for name in candidates:
        if name in available:
            plt.rcParams["font.sans-serif"] = [name, "DejaVu Sans"]
            plt.rcParams["axes.unicode_minus"] = False
            return
    plt.rcParams["font.sans-serif"] = ["DejaVu Sans"]
    plt.rcParams["axes.unicode_minus"] = True

_setup_chinese_font()

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import gradio as gr

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_DIR = os.path.join(PROJECT_ROOT, "rl", "logs")
MODELS_DIR = os.path.join(PROJECT_ROOT, "rl", "models")

_training_state = {
    "process": None,
    "running": False,
    "log_dir": None,
    "start_time": None,
}
_training_lock = threading.Lock()


def find_available_log_dir(algo="ppo"):
    os.makedirs(LOG_DIR, exist_ok=True)
    existing = [d for d in os.listdir(LOG_DIR) if d.startswith(f"{algo}_v")]
    max_v = 0
    for d in existing:
        try:
            v = int(d.split("_v")[1])
            max_v = max(max_v, v)
        except (ValueError, IndexError):
            pass
    return os.path.join(LOG_DIR, f"{algo}_v{max_v + 1}")


def get_all_log_dirs():
    os.makedirs(LOG_DIR, exist_ok=True)
    return sorted([d for d in os.listdir(LOG_DIR)
                    if os.path.isdir(os.path.join(LOG_DIR, d))])


def get_available_models():
    models = []
    os.makedirs(LOG_DIR, exist_ok=True)
    for d in sorted(os.listdir(LOG_DIR)):
        dir_path = os.path.join(LOG_DIR, d)
        if not os.path.isdir(dir_path):
            continue
        best_path = os.path.join(dir_path, "best_model", "best_model.zip")
        if os.path.exists(best_path):
            models.append(f"{d}/best_model")
        for algo in ["ppo", "dqn"]:
            final_path = os.path.join(dir_path, f"{algo}_final.zip")
            if os.path.exists(final_path):
                models.append(f"{d}/{algo}_final")
    return models


def model_display_to_path(display_name):
    if not display_name:
        return None
    parts = display_name.split("/")
    if len(parts) < 2:
        return None
    dir_name = parts[0]
    model_name = parts[1]
    if model_name == "best_model":
        return os.path.join(LOG_DIR, dir_name, "best_model", "best_model.zip")
    return os.path.join(LOG_DIR, dir_name, f"{model_name}.zip")


def read_eval_results(log_dir):
    npz_path = os.path.join(log_dir, "eval_results", "evaluations.npz")
    if not os.path.exists(npz_path):
        return None, None, None
    try:
        data = np.load(npz_path)
        timesteps = data.get("timesteps", np.array([]))
        results = data.get("results", np.array([]))
        ep_lengths = data.get("ep_lengths", np.array([]))
        if len(results.shape) == 2:
            mean_rewards = results.mean(axis=1)
        else:
            mean_rewards = results
        return timesteps, mean_rewards, ep_lengths
    except Exception as e:
        print(f"读取评估结果失败: {e}")
        return None, None, None


def plot_reward_curve(log_dir, title="Reward Curve"):
    timesteps, mean_rewards, _ = read_eval_results(log_dir)
    if timesteps is None or len(timesteps) == 0:
        fig, ax = plt.subplots(figsize=(10, 5))
        ax.text(0.5, 0.5, "No evaluation data", ha="center", va="center", fontsize=16)
        ax.set_title(title)
        path = os.path.join(LOG_DIR, "_temp_plot.png")
        fig.savefig(path, dpi=100, bbox_inches="tight")
        plt.close(fig)
        return path

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(timesteps, mean_rewards, "b-", linewidth=1.5, label="Mean Reward")
    best_idx = np.argmax(mean_rewards)
    best_reward = mean_rewards[best_idx]
    best_step = timesteps[best_idx]
    ax.plot(best_step, best_reward, "r*", markersize=15,
            label=f"Best: {best_reward:.1f} @{int(best_step)}steps")
    ax.set_xlabel("Timesteps")
    ax.set_ylabel("Mean Reward")
    ax.set_title(title)
    ax.legend()
    ax.grid(True, alpha=0.3)
    path = os.path.join(LOG_DIR, "_temp_plot.png")
    fig.savefig(path, dpi=100, bbox_inches="tight")
    plt.close(fig)
    return path


def start_training(algo, difficulty, total_timesteps, lr, ent_coef,
                   n_envs, curriculum, resume_best, resume_model):
    global _training_state

    with _training_lock:
        if _training_state["running"]:
            return "训练正在进行中，请先停止当前训练！", get_training_status()

        log_dir = find_available_log_dir(algo)

        cmd = [
            sys.executable, os.path.join(PROJECT_ROOT, "rl", "train.py"),
            "--algo", algo,
            "--mode", "train",
            "--total-timesteps", str(int(total_timesteps)),
            "--lr", str(lr),
            "--n-envs", str(int(n_envs)),
            "--difficulty", str(int(difficulty)),
            "--log-dir", log_dir,
        ]

        if curriculum:
            cmd.append("--curriculum")

        if resume_best:
            if resume_model and resume_model.strip():
                resume_path = model_display_to_path(resume_model)
                if resume_path and os.path.exists(resume_path):
                    cmd.extend(["--resume", resume_path])
            else:
                cmd.extend(["--resume", "auto"])

        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                cwd=PROJECT_ROOT,
            )
            _training_state["process"] = process
            _training_state["running"] = True
            _training_state["log_dir"] = log_dir
            _training_state["start_time"] = time.time()

            def _monitor():
                process.wait()
                with _training_lock:
                    _training_state["running"] = False
                    _training_state["process"] = None

            threading.Thread(target=_monitor, daemon=True).start()

            return (f"训练已启动！\n算法: {algo.upper()}\n日志目录: {log_dir}\n"
                    f"总步数: {int(total_timesteps):,}"), get_training_status()
        except Exception as e:
            return f"启动训练失败: {e}", get_training_status()


def stop_training():
    global _training_state
    with _training_lock:
        if not _training_state["running"] or _training_state["process"] is None:
            return "当前没有正在运行的训练进程。"

        try:
            _training_state["process"].terminate()
            try:
                _training_state["process"].wait(timeout=3)
            except subprocess.TimeoutExpired:
                _training_state["process"].kill()
            _training_state["running"] = False
            _training_state["process"] = None
            return "训练进程已停止。"
        except Exception as e:
            return f"停止训练失败: {e}"


def get_training_status():
    with _training_lock:
        running = _training_state["running"]
        start_time = _training_state["start_time"]
        log_dir = _training_state["log_dir"]

    if not running:
        return "状态: 空闲（无训练任务）"

    elapsed = time.time() - start_time if start_time else 0
    hours, remainder = divmod(int(elapsed), 3600)
    minutes, seconds = divmod(remainder, 60)

    timesteps, mean_rewards, _ = read_eval_results(log_dir)

    status = f"状态: 训练中\n"
    status += f"已运行: {hours}小时{minutes}分{seconds}秒\n"
    status += f"日志目录: {log_dir}\n"

    if timesteps is not None and len(timesteps) > 0:
        current_step = int(timesteps[-1])
        best_reward = float(np.max(mean_rewards))
        latest_reward = float(mean_rewards[-1])
        status += f"当前步数: {current_step:,}\n"
        status += f"最新奖励: {latest_reward:.2f}\n"
        status += f"最佳奖励: {best_reward:.2f}"
    else:
        status += "暂无评估数据"

    return status


def run_evaluation(model_name, difficulty, eval_episodes, max_steps):
    model_path = model_display_to_path(model_name)
    if not model_path or not os.path.exists(model_path):
        return "模型文件不存在！", None

    cmd = [
        sys.executable, os.path.join(PROJECT_ROOT, "rl", "train.py"),
        "--mode", "eval",
        "--model-path", model_path,
        "--difficulty", str(int(difficulty)),
        "--eval-episodes", str(int(eval_episodes)),
        "--max-steps", str(int(max_steps)),
    ]

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=300, cwd=PROJECT_ROOT
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            return f"评估失败:\n{output}", None
    except subprocess.TimeoutExpired:
        return "评估超时（超过5分钟）", None
    except Exception as e:
        return f"评估出错: {e}", None

    log_dir = os.path.dirname(os.path.dirname(model_path))
    results_path = os.path.join(log_dir, "eval_results.json")
    if os.path.exists(results_path):
        with open(results_path, "r") as f:
            data = json.load(f)
        report = f"评估完成！\n"
        report += f"平均奖励: {data['avg_reward']:.2f}\n"
        report += f"平均分数: {data['avg_score']:.2f}\n"
        report += f"评估局数: {len(data['episodes'])}\n\n"
        report += "各局详情:\n"
        for ep in data["episodes"]:
            report += (
                f"  第{ep['episode']}局: 奖励={ep['reward']:.1f} "
                f"分数={ep['score']} 击杀={ep['kills']} "
                f"阶段={ep['victory_stage']} 时长={ep['game_time']:.1f}s\n"
            )
    else:
        report = output[:2000]

    log_dirs = get_all_log_dirs()
    plot_path = None
    if log_dirs:
        latest_log = os.path.join(LOG_DIR, log_dirs[-1])
        plot_path = plot_reward_curve(latest_log, title="Eval Reward Curve")

    return report, plot_path


def export_onnx_model(model_name, output_name):
    model_path = model_display_to_path(model_name)
    if not model_path or not os.path.exists(model_path):
        return "模型文件不存在！"

    os.makedirs(MODELS_DIR, exist_ok=True)
    output_path = os.path.join(MODELS_DIR,
                                output_name if output_name.endswith(".onnx") else f"{output_name}.onnx")

    cmd = [
        sys.executable, os.path.join(PROJECT_ROOT, "rl", "export_onnx_standalone.py"),
        "--model", model_path,
        "--output", output_path,
    ]

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=120, cwd=PROJECT_ROOT
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            return f"导出失败:\n{output[:1000]}"
        return f"ONNX模型导出成功！\n保存路径: {output_path}\n\n{output[:500]}"
    except subprocess.TimeoutExpired:
        return "导出超时（超过2分钟）"
    except Exception as e:
        return f"导出出错: {e}"


def refresh_monitor():
    all_dirs = get_all_log_dirs()
    if not all_dirs:
        return "暂无训练记录。", None, "", ""

    latest_log = None
    for d in reversed(all_dirs):
        d_path = os.path.join(LOG_DIR, d)
        npz_path = os.path.join(d_path, "eval_results", "evaluations.npz")
        if os.path.exists(npz_path):
            latest_log = d_path
            break

    status = get_training_status()

    summary = "所有训练记录:\n"
    summary += "-" * 60 + "\n"
    for d in all_dirs:
        d_path = os.path.join(LOG_DIR, d)
        timesteps, mean_rewards, _ = read_eval_results(d_path)
        best_model_path = os.path.join(d_path, "best_model", "best_model.zip")
        has_best = "Y" if os.path.exists(best_model_path) else "N"
        if timesteps is not None and len(timesteps) > 0:
            best_reward = np.max(mean_rewards)
            latest_reward = mean_rewards[-1]
            latest_step = int(timesteps[-1])
            summary += (f"  {d}: steps={latest_step:,} "
                        f"latest={latest_reward:.1f} best={best_reward:.1f} "
                        f"saved={has_best}\n")
        else:
            summary += f"  {d}: 无数据 saved={has_best}\n"

    plot_path = None
    best_reward_str = ""
    current_step_str = ""
    if latest_log:
        timesteps, mean_rewards, _ = read_eval_results(latest_log)
        if timesteps is not None and len(timesteps) > 0:
            best_reward_str = f"{np.max(mean_rewards):.2f}"
            current_step_str = f"{int(timesteps[-1]):,}"
            fig, ax = plt.subplots(figsize=(12, 6))
            for d in all_dirs:
                d_path = os.path.join(LOG_DIR, d)
                ts, mr, _ = read_eval_results(d_path)
                if ts is not None and len(ts) > 0:
                    ax.plot(ts, mr, linewidth=1.2, label=d)
            ax.set_xlabel("Timesteps")
            ax.set_ylabel("Mean Reward")
            ax.set_title("All Runs - Reward Curve Comparison")
            ax.legend(fontsize=8)
            ax.grid(True, alpha=0.3)
            plot_path = os.path.join(LOG_DIR, "_temp_monitor.png")
            fig.savefig(plot_path, dpi=100, bbox_inches="tight")
            plt.close(fig)

    return status + "\n\n" + summary, plot_path, best_reward_str, current_step_str


def build_ui():
    with gr.Blocks(title="JRFirstGame RL") as app:
        gr.Markdown("# JRFirstGame RL训练管线控制台")

        with gr.Tab("训练"):
            with gr.Row():
                with gr.Column(scale=1):
                    algo = gr.Dropdown(
                        choices=["ppo"], value="ppo", label="算法",
                        info="目前仅支持PPO"
                    )
                    difficulty = gr.Slider(
                        minimum=0, maximum=3, value=2, step=1,
                        label="基础难度等级", info="自适应难度会随score提升: 0→1→2→3"
                    )
                    total_timesteps = gr.Number(
                        value=2000000, label="总训练步数",
                        info="建议: 100万~400万"
                    )
                    lr = gr.Number(
                        value=5e-4, label="学习率",
                        info="PPO默认: 5e-4"
                    )
                    ent_coef = gr.Number(
                        value=0.1, label="熵系数 (ent_coef)",
                        info="控制探索程度"
                    )
                    n_envs = gr.Slider(
                        minimum=1, maximum=8, value=4, step=1,
                        label="并行环境数",
                    )
                    curriculum = gr.Checkbox(
                        value=False, label="启用课程学习",
                    )
                    with gr.Row():
                        resume_best = gr.Checkbox(
                            value=False, label="从已有模型恢复",
                        )
                        resume_model = gr.Dropdown(
                            choices=get_available_models(), value=None,
                            label="选择恢复模型",
                        )

                    with gr.Row():
                        start_btn = gr.Button("开始训练", variant="primary")
                        stop_btn = gr.Button("停止训练", variant="stop")

                with gr.Column(scale=1):
                    train_output = gr.Textbox(
                        label="训练输出", lines=8, interactive=False
                    )
                    train_status = gr.Textbox(
                        label="训练状态", lines=8, interactive=False
                    )

            start_btn.click(
                fn=start_training,
                inputs=[algo, difficulty, total_timesteps, lr, ent_coef,
                        n_envs, curriculum, resume_best, resume_model],
                outputs=[train_output, train_status],
            )
            stop_btn.click(fn=stop_training, outputs=[train_output])

        with gr.Tab("评估"):
            with gr.Row():
                with gr.Column(scale=1):
                    eval_model = gr.Dropdown(
                        choices=get_available_models(), value=None,
                        label="选择模型",
                    )
                    eval_difficulty = gr.Slider(
                        minimum=0, maximum=3, value=1, step=1,
                        label="评估难度"
                    )
                    eval_episodes = gr.Slider(
                        minimum=1, maximum=50, value=10, step=1,
                        label="评估局数"
                    )
                    eval_max_steps = gr.Number(
                        value=36000, label="每局最大步数"
                    )
                    eval_btn = gr.Button("开始评估", variant="primary")

                with gr.Column(scale=1):
                    eval_output = gr.Textbox(
                        label="评估结果", lines=15, interactive=False
                    )
                    eval_plot = gr.Image(label="奖励曲线")

            eval_btn.click(
                fn=run_evaluation,
                inputs=[eval_model, eval_difficulty, eval_episodes, eval_max_steps],
                outputs=[eval_output, eval_plot],
            )

        with gr.Tab("导出ONNX"):
            with gr.Row():
                with gr.Column(scale=1):
                    export_model = gr.Dropdown(
                        choices=get_available_models(), value=None,
                        label="选择模型",
                    )
                    export_name = gr.Textbox(
                        value="ppo_policy.onnx", label="导出文件名",
                        info="保存到 rl/models/ 目录下"
                    )
                    export_btn = gr.Button("导出ONNX", variant="primary")

                with gr.Column(scale=1):
                    export_output = gr.Textbox(
                        label="导出输出", lines=10, interactive=False
                    )

            export_btn.click(
                fn=export_onnx_model,
                inputs=[export_model, export_name],
                outputs=[export_output],
            )

        with gr.Tab("监控"):
            with gr.Row():
                monitor_refresh_btn = gr.Button("刷新数据", variant="primary")
            with gr.Row():
                monitor_status = gr.Textbox(
                    label="训练状态", lines=12, interactive=False
                )
            with gr.Row():
                with gr.Column(scale=1):
                    monitor_best_reward = gr.Textbox(
                        label="最佳奖励", interactive=False
                    )
                with gr.Column(scale=1):
                    monitor_current_step = gr.Textbox(
                        label="当前步数", interactive=False
                    )
            with gr.Row():
                monitor_plot = gr.Image(label="训练奖励曲线")

            monitor_refresh_btn.click(
                fn=refresh_monitor,
                outputs=[monitor_status, monitor_plot,
                         monitor_best_reward, monitor_current_step],
            )

    return app


if __name__ == "__main__":
    print("=" * 50)
    print("  JRFirstGame RL训练管线 WebUI")
    print("  访问地址: http://localhost:7860")
    print("=" * 50)
    os.makedirs(LOG_DIR, exist_ok=True)
    os.makedirs(MODELS_DIR, exist_ok=True)
    app = build_ui()
    app.launch(
        server_name="0.0.0.0",
        server_port=7860,
        share=False,
        max_threads=4,
        theme=gr.themes.Soft(),
    )
