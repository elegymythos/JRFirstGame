"""v21: 从 v16 best_model 恢复，敌人生成与C++完全一致"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if __name__ == '__main__':
    from multiprocessing import freeze_support
    freeze_support()
    
    os.makedirs("rl/logs/ppo_v21", exist_ok=True)
    
    from rl.train import main
    sys.argv = [
        "train.py", "--algo", "ppo", "--mode", "train",
        "--total-timesteps", "4000000", "--n-envs", "4",
        "--difficulty", "2", "--lr", "5e-4",
        "--log-dir", "rl/logs/ppo_v21",
        "--resume", "rl/logs/ppo_v16/best_model/best_model.zip",
    ]
    main()
