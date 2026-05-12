import os
import sys
import numpy as np
import torch

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

OBS_DIM = 330
NUM_ACTIONS = 12

model_path = "rl/logs/ppo_v23/best_model/best_model.zip"
output_path = "rl/models/ppo_policy.onnx"

import gymnasium as gym
from gymnasium import spaces

class DummyEnv(gym.Env):
    observation_space = spaces.Box(low=-1.0, high=1.0, shape=(OBS_DIM,), dtype=np.float32)
    action_space = spaces.Discrete(NUM_ACTIONS)
    def reset(self, *, seed=None, options=None):
        return self.observation_space.sample(), {}
    def step(self, action):
        return self.observation_space.sample(), 0.0, False, False, {}

from stable_baselines3 import PPO

env = DummyEnv()
model = PPO.load(model_path, env=env, device="cpu")
print(f"Loaded model from {model_path}")

policy_net = model.policy
features_extractor = policy_net.features_extractor
latent_pi_net = policy_net.mlp_extractor.policy_net
action_net = policy_net.action_net

class PPOActionNet(torch.nn.Module):
    def __init__(self, feat_ext, latent_pi, act_net):
        super().__init__()
        self.feat_ext = feat_ext
        self.latent_pi = latent_pi
        self.act_net = act_net
    def forward(self, x):
        features = self.feat_ext(x)
        latent = self.latent_pi(features)
        action_logits = self.act_net(latent)
        return action_logits

combined_net = PPOActionNet(features_extractor, latent_pi_net, action_net)
combined_net.eval()

dummy_obs = torch.randn(1, OBS_DIM, dtype=torch.float32)

import onnx

with torch.no_grad():
    torch.onnx.export(
        combined_net, dummy_obs, output_path,
        export_params=True, opset_version=17, do_constant_folding=True,
        input_names=["observation"], output_names=["action_logits"],
        dynamic_axes={"observation": {0: "batch_size"}, "action_logits": {0: "batch_size"}},
    )

print(f"ONNX model exported to {output_path}")

onnx_model = onnx.load(output_path)
onnx.checker.check_model(onnx_model)
print("ONNX model validation passed")

import onnxruntime as ort
sess = ort.InferenceSession(output_path, providers=["CPUExecutionProvider"])
test_obs = np.random.randn(1, OBS_DIM).astype(np.float32)
ort_inputs = {sess.get_inputs()[0].name: test_obs}
ort_outs = sess.run(None, ort_inputs)
logits = ort_outs[0][0]
action = np.argmax(logits)
print(f"ONNX inference test: action={action}, logits_shape={logits.shape}")
print(f"Top-5 actions: {np.argsort(logits)[-5:][::-1]}")
print("Export complete!")
