"""v25: 从 v24 resume，降低 ent_coef 0.1→0.05，增大 batch_size 512→1024"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if __name__ == '__main__':
    from multiprocessing import freeze_support
    freeze_support()

    os.makedirs("rl/logs/ppo_v25", exist_ok=True)

    from rl.train import main
    sys.argv = [
        "train.py", "--algo", "ppo", "--mode", "train",
        "--total-timesteps", "4000000", "--n-envs", "4",
        "--difficulty", "2", "--lr", "5e-4",
        "--batch-size", "1024",
        "--ent-coef", "0.05",
        "--log-dir", "rl/logs/ppo_v25",
        "--resume", "rl/logs/ppo_v24/best_model/best_model.zip",
    ]
    main()
