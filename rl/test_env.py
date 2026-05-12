import sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from rl.game_env import JRFirstGameEnv, NUM_ACTIONS, OBS_DIM, ACTION_MEANING
import numpy as np
print(f"NUM_ACTIONS={NUM_ACTIONS}, OBS_DIM={OBS_DIM}")
print(f"Actions: {ACTION_MEANING}")
env = JRFirstGameEnv(max_steps=500, seed=42, difficulty_level=2)
obs, info = env.reset()
print(f"obs shape={obs.shape}, enemies={info['num_enemies']}")
for i in range(100):
    a = np.random.randint(0, NUM_ACTIONS)
    obs, r, term, trunc, info = env.step(a)
    if term or trunc:
        break
print(f"Steps={info['step']}, score={info['score']}")
env.close()
print("OK")
