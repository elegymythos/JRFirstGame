"""v20: 从 v18 best_model 恢复，attack_nearest 自动边打边走"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.makedirs("rl/logs/ppo_v20", exist_ok=True)
os.makedirs("rl/models", exist_ok=True)

from rl.train import main
sys.argv = [
    "train.py", "--algo", "ppo", "--mode", "train",
    "--total-timesteps", "4000000", "--n-envs", "4",
    "--difficulty", "2", "--lr", "5e-4",
    "--log-dir", "rl/logs/ppo_v20",
    "--resume", "rl/logs/ppo_v18/best_model/best_model.zip",
]
main()
