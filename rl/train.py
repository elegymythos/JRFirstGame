import os
import sys
import argparse
import json
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from rl.game_env import JRFirstGameEnv, NUM_ACTIONS, OBS_DIM

from stable_baselines3 import DQN, PPO
from stable_baselines3.common.callbacks import BaseCallback, EvalCallback
from stable_baselines3.common.vec_env import DummyVecEnv, SubprocVecEnv
from stable_baselines3.common.utils import set_random_seed


class ProgressLogger(BaseCallback):
    def __init__(self, log_freq=1000, verbose=0):
        super().__init__(verbose)
        self.log_freq = log_freq

    def _on_step(self):
        if self.num_timesteps % self.log_freq == 0:
            info = self.locals.get("infos", [{}])
            if info and info[0]:
                i = info[0]
                print(
                    f"[Step {self.num_timesteps:>8d}] "
                    f"HP:{i.get('health', '?'):>4} "
                    f"Score:{i.get('score', '?'):>5} "
                    f"Kills:{i.get('kill_count', '?'):>3} "
                    f"Stage:{i.get('victory_stage', '?')} "
                    f"Diff:{i.get('effective_difficulty', '?')} "
                    f"Time:{i.get('game_time', 0):>6.1f}s "
                    f"Enemies:{i.get('num_enemies', '?')}"
                )
        return True


class CurriculumCallback(BaseCallback):
    def __init__(self, total_timesteps, eval_env=None, levels=None, verbose=0):
        super().__init__(verbose)
        self.total_timesteps = total_timesteps
        self.eval_env = eval_env
        self.levels = levels or [0, 1, 2, 3]
        self._current_level = -1

    def _on_step(self):
        progress = self.num_timesteps / self.total_timesteps
        if progress < 0.25:
            target_level = self.levels[0]
        elif progress < 0.5:
            target_level = self.levels[1]
        elif progress < 0.75:
            target_level = self.levels[2]
        else:
            target_level = self.levels[3]

        if target_level != self._current_level:
            self._current_level = target_level
            try:
                self.training_env.env_method('set_difficulty_level', target_level)
                if self.eval_env is not None:
                    self.eval_env.set_difficulty_level(target_level)
            except Exception:
                pass
        return True


def make_env(rank, seed=0, max_steps=36000, difficulty_level=0):
    def _init():
        env = JRFirstGameEnv(max_steps=max_steps, seed=seed + rank,
                             difficulty_level=difficulty_level)
        return env
    set_random_seed(seed + rank)
    return _init


def train_dqn(args):
    n_envs = args.n_envs
    vec_env_cls = SubprocVecEnv if n_envs > 1 else DummyVecEnv
    train_envs = vec_env_cls([make_env(i, args.seed, args.max_steps, args.difficulty) for i in range(n_envs)])

    model = DQN(
        "MlpPolicy",
        train_envs,
        learning_rate=args.lr,
        buffer_size=args.buffer_size,
        learning_starts=args.learning_starts,
        batch_size=args.batch_size,
        gamma=args.gamma,
        target_update_interval=args.target_update,
        exploration_fraction=args.exploration_fraction,
        exploration_initial_eps=1.0,
        exploration_final_eps=0.05,
        verbose=1,
        seed=args.seed,
    )

    eval_env = JRFirstGameEnv(max_steps=args.max_steps, seed=args.seed + 100,
                              difficulty_level=args.difficulty)
    eval_callback = EvalCallback(
        eval_env,
        best_model_save_path=os.path.join(args.log_dir, "best_model"),
        log_path=os.path.join(args.log_dir, "eval_results"),
        eval_freq=max(args.learning_starts, 5000),
        n_eval_episodes=5,
        deterministic=True,
    )

    logger_callback = ProgressLogger(log_freq=1000)

    print(f"=== DQN Training ===")
    print(f"Observation dim: {OBS_DIM}")
    print(f"Action dim: {NUM_ACTIONS}")
    print(f"Total timesteps: {args.total_timesteps}")
    print(f"N environments: {n_envs}")
    print(f"Log dir: {args.log_dir}")

    model.learn(
        total_timesteps=args.total_timesteps,
        callback=[eval_callback, logger_callback],
        log_interval=100,
    )

    model_path = os.path.join(args.log_dir, "dqn_final")
    model.save(model_path)
    print(f"Model saved to {model_path}")

    train_envs.close()
    eval_env.close()


def train_ppo(args):
    n_envs = args.n_envs
    vec_env_cls = SubprocVecEnv if n_envs > 1 else DummyVecEnv
    train_envs = vec_env_cls([make_env(i, args.seed, args.max_steps, args.difficulty) for i in range(n_envs)])

    policy_kwargs = dict(
        net_arch=dict(pi=[256, 256], vf=[256, 256]),
    )

    def linear_schedule(progress_remaining):
        return 1e-4 + progress_remaining * (args.lr - 1e-4)

    model = PPO(
        "MlpPolicy",
        train_envs,
        learning_rate=linear_schedule,
        n_steps=8192,
        batch_size=512,
        n_epochs=10,
        gamma=args.gamma,
        gae_lambda=0.95,
        clip_range=0.2,
        ent_coef=0.1,
        vf_coef=0.5,
        max_grad_norm=0.5,
        verbose=1,
        seed=args.seed,
        tensorboard_log=None,
        policy_kwargs=policy_kwargs,
        device="cuda",
    )

    if args.resume:
        resume_path = args.resume
        if not os.path.exists(resume_path):
            best_path = os.path.join(args.log_dir, "best_model", "best_model.zip")
            if os.path.exists(best_path):
                resume_path = best_path
        if os.path.exists(resume_path):
            print(f"Resuming from {resume_path}")
            model = PPO.load(resume_path, env=train_envs, device="cuda")

    eval_env = JRFirstGameEnv(max_steps=args.max_steps, seed=args.seed + 100,
                              difficulty_level=args.difficulty)
    eval_callback = EvalCallback(
        eval_env,
        best_model_save_path=os.path.join(args.log_dir, "best_model"),
        log_path=os.path.join(args.log_dir, "eval_results"),
        eval_freq=20000,
        n_eval_episodes=10,
        deterministic=True,
    )

    logger_callback = ProgressLogger(log_freq=1000)

    callbacks = [eval_callback, logger_callback]

    if args.curriculum:
        curriculum_cb = CurriculumCallback(
            args.total_timesteps, eval_env=eval_env, levels=[0, 1, 2, 3]
        )
        callbacks.append(curriculum_cb)

    print(f"=== PPO Training ===")
    print(f"Observation dim: {OBS_DIM}")
    print(f"Action dim: {NUM_ACTIONS}")
    print(f"Total timesteps: {args.total_timesteps}")
    print(f"N environments: {n_envs}")
    print(f"Difficulty: {args.difficulty}")
    print(f"Curriculum: {args.curriculum}")
    print(f"Log dir: {args.log_dir}")

    model.learn(
        total_timesteps=args.total_timesteps,
        callback=callbacks,
        log_interval=100,
    )

    model_path = os.path.join(args.log_dir, "ppo_final")
    model.save(model_path)
    print(f"Model saved to {model_path}")

    train_envs.close()
    eval_env.close()


def evaluate(args):
    model_path = args.model_path
    if not model_path:
        best_path = os.path.join(args.log_dir, "best_model", "best_model.zip")
        if os.path.exists(best_path):
            model_path = best_path
        else:
            print("No model path specified and no best_model found")
            return

    env = JRFirstGameEnv(max_steps=args.max_steps, seed=args.seed,
                         difficulty_level=args.difficulty)

    if "dqn" in model_path.lower():
        model = DQN.load(model_path, env=env)
    else:
        model = PPO.load(model_path, env=env)

    n_episodes = args.eval_episodes
    results = []

    for ep in range(n_episodes):
        obs, _ = env.reset()
        total_reward = 0.0
        done = False
        while not done:
            action, _ = model.predict(obs, deterministic=True)
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            done = terminated or truncated

        results.append({
            "episode": ep,
            "reward": total_reward,
            "score": info["score"],
            "kills": info["kill_count"],
            "victory_stage": info["victory_stage"],
            "game_time": info["game_time"],
        })
        print(
            f"Ep {ep:>3d}: Reward={total_reward:>8.1f} "
            f"Score={info['score']:>5} Kills={info['kill_count']:>3} "
            f"Stage={info['victory_stage']} Time={info['game_time']:.1f}s"
        )

    avg_reward = np.mean([r["reward"] for r in results])
    avg_score = np.mean([r["score"] for r in results])
    print(f"\nAvg Reward: {avg_reward:.1f} | Avg Score: {avg_score:.1f}")

    results_path = os.path.join(args.log_dir, "eval_results.json")
    with open(results_path, "w") as f:
        json.dump({"avg_reward": avg_reward, "avg_score": avg_score, "episodes": results}, f, indent=2)

    env.close()


def export_policy(args):
    model_path = args.model_path
    if not model_path:
        best_path = os.path.join(args.log_dir, "best_model", "best_model.zip")
        if os.path.exists(best_path):
            model_path = best_path
        else:
            print("No model path specified")
            return

    env = JRFirstGameEnv(max_steps=100, seed=0)

    if "dqn" in model_path.lower():
        model = DQN.load(model_path, env=env)
    else:
        model = PPO.load(model_path, env=env)

    export_dir = os.path.join(args.log_dir, "exported")
    os.makedirs(export_dir, exist_ok=True)

    policy_path = os.path.join(export_dir, "policy_net.pt")
    try:
        import torch
        torch.save(model.policy.state_dict(), policy_path)
        print(f"Policy exported to {policy_path}")
    except Exception as e:
        print(f"Export failed: {e}")

    env.close()


def main():
    parser = argparse.ArgumentParser(description="JRFirstGame RL Training")
    parser.add_argument("--algo", type=str, default="dqn", choices=["dqn", "ppo"])
    parser.add_argument("--mode", type=str, default="train", choices=["train", "eval", "export"])
    parser.add_argument("--total-timesteps", type=int, default=500_000)
    parser.add_argument("--n-envs", type=int, default=4)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--max-steps", type=int, default=36000)
    parser.add_argument("--log-dir", type=str, default="rl/logs")
    parser.add_argument("--lr", type=float, default=5e-4)
    parser.add_argument("--gamma", type=float, default=0.99)
    parser.add_argument("--buffer-size", type=int, default=100_000)
    parser.add_argument("--batch-size", type=int, default=256)
    parser.add_argument("--learning-starts", type=int, default=10_000)
    parser.add_argument("--target-update", type=int, default=1000)
    parser.add_argument("--exploration-fraction", type=float, default=0.3)
    parser.add_argument("--model-path", type=str, default=None)
    parser.add_argument("--eval-episodes", type=int, default=10)
    parser.add_argument("--resume", type=str, default=None, help="Resume from model path")
    parser.add_argument("--difficulty", type=int, default=2, help="Difficulty level 0-3")
    parser.add_argument("--curriculum", action="store_true", help="Enable curriculum learning")
    args = parser.parse_args()

    os.makedirs(args.log_dir, exist_ok=True)

    if args.mode == "train":
        if args.algo == "dqn":
            train_dqn(args)
        else:
            train_ppo(args)
    elif args.mode == "eval":
        evaluate(args)
    elif args.mode == "export":
        export_policy(args)


if __name__ == "__main__":
    main()
