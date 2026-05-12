"""v23: 扩展观测空间(P20+E10+Pj6=330) + 强化弹幕闪避/风筝/寻敌/范围感知"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if __name__ == '__main__':
    from multiprocessing import freeze_support
    freeze_support()

    os.makedirs("rl/logs/ppo_v23", exist_ok=True)

    from rl.train import main
    sys.argv = [
        "train.py", "--algo", "ppo", "--mode", "train",
        "--total-timesteps", "4000000", "--n-envs", "4",
        "--difficulty", "2", "--lr", "5e-4",
        "--log-dir", "rl/logs/ppo_v23",
    ]
    main()
