import os
import sys
import argparse
import numpy as np
import torch

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from rl.game_env import JRFirstGameEnv, NUM_ACTIONS, OBS_DIM

import onnx
import onnxruntime as ort


def export_onnx(model_path, output_path):
    from stable_baselines3 import PPO, DQN

    env = JRFirstGameEnv(max_steps=100, seed=0)

    if "dqn" in model_path.lower():
        model = DQN.load(model_path, env=env)
    else:
        model = PPO.load(model_path, env=env)

    print(f"Loaded model from {model_path}")

    if "dqn" in model_path.lower():
        policy_net = model.q_net
    else:
        policy_net = model.policy

    device = next(policy_net.parameters()).device
    dummy_obs = torch.randn(1, OBS_DIM, dtype=torch.float32, device=device)

    if "dqn" in model_path.lower():
        torch_out = policy_net(dummy_obs)
        onnx_output_name = "q_values"
    else:
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

        with torch.no_grad():
            torch_out = combined_net(dummy_obs)

        policy_net = combined_net
        onnx_output_name = "action_logits"

    torch.onnx.export(
        policy_net,
        dummy_obs,
        output_path,
        export_params=True,
        opset_version=17,
        do_constant_folding=True,
        input_names=["observation"],
        output_names=[onnx_output_name],
        dynamic_axes={
            "observation": {0: "batch_size"},
            onnx_output_name: {0: "batch_size"},
        },
    )

    print(f"ONNX model exported to {output_path}")

    onnx_model = onnx.load(output_path)
    onnx.checker.check_model(onnx_model)
    print("ONNX model validation passed")

    sess = ort.InferenceSession(output_path, providers=["CPUExecutionProvider"])
    obs, _ = env.reset()
    ort_inputs = {sess.get_inputs()[0].name: obs.reshape(1, -1).astype(np.float32)}
    ort_outs = sess.run(None, ort_inputs)
    logits = ort_outs[0][0]

    if "q_values" in onnx_output_name:
        action = np.argmax(logits)
    else:
        action = np.argmax(logits)

    print(f"ONNX inference test: action={action}, logits_shape={logits.shape}")
    print(f"Top-5 actions: {np.argsort(logits)[-5:][::-1]}")

    env.close()


def main():
    parser = argparse.ArgumentParser(description="Export model to ONNX")
    parser.add_argument("--model", type=str, default=None, help="Model path")
    parser.add_argument("--algo", type=str, default="ppo", choices=["dqn", "ppo"])
    parser.add_argument("--output", type=str, default=None, help="Output ONNX path")
    args = parser.parse_args()

    if args.model is None:
        best_path = f"rl/logs/ppo_v5/best_model/best_model.zip"
        if not os.path.exists(best_path):
            best_path = f"rl/logs/ppo_v2/best_model/best_model.zip"
        args.model = best_path

    if args.output is None:
        args.output = "rl/models/ppo_policy.onnx"

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    export_onnx(args.model, args.output)


if __name__ == "__main__":
    main()
