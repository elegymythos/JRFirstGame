"""v22: 增强弹幕预判闪避 + 攻击范围惩罚"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if __name__ == '__main__':
    from multiprocessing import freeze_support
    freeze_support()
    
    os.makedirs("rl/logs/ppo_v22", exist_ok=True)
    
    from rl.train import main
    sys.argv = [
        "train.py", "--algo", "ppo", "--mode", "train",
        "--total-timesteps", "4000000", "--n-envs", "4",
        "--difficulty", "2", "--lr", "5e-4",
        "--log-dir", "rl/logs/ppo_v22",
        "--resume", "rl/logs/ppo_v21/best_model/best_model.zip",
    ]
    main()
