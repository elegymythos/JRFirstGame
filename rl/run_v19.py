"""Start v19 training with GPU"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.makedirs("rl/logs/ppo_v19", exist_ok=True)
os.makedirs("rl/models", exist_ok=True)

from rl.train import main
sys.argv = [
    "train.py", "--algo", "ppo", "--mode", "train",
    "--total-timesteps", "4000000", "--n-envs", "4",
    "--difficulty", "2", "--lr", "5e-4",
    "--log-dir", "rl/logs/ppo_v19",
]
main()
