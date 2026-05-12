import os
import sys
import math
import argparse

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pygame
import numpy as np

from rl.game_env import JRFirstGameEnv, ENEMY_TYPES, MOVE_DIRS, NUM_MOVE, NUM_ATTACK_TARGETS

ENEMY_NAMES = {0: "Slime", 1: "SkeletonW", 2: "SkeletonA", 3: "Giant"}
ENEMY_COLORS = {
    0: (0, 200, 0),
    1: (200, 200, 200),
    2: (180, 180, 140),
    3: (139, 90, 43),
}
DROP_COLORS = {
    0: (255, 80, 80),
    1: (80, 255, 80),
    2: (255, 160, 60),
    3: (100, 100, 255),
    4: (200, 100, 255),
    5: (255, 255, 100),
}

SCREEN_W, SCREEN_H = 1280, 720
CELL_SIZE = 800
FPS = 60


def world_to_screen(wx, wy, cam_x, cam_y):
    sx = (wx - cam_x) + SCREEN_W / 2
    sy = (wy - cam_y) + SCREEN_H / 2
    return int(sx), int(sy)


def draw_player(screen, env, cam_x, cam_y):
    p = env.player
    sx, sy = world_to_screen(p.x, p.y, cam_x, cam_y)
    if p.is_rolling:
        color = (100, 200, 255)
    elif p.is_invincible:
        color = (255, 255, 100)
    else:
        color = (50, 120, 255)
    pygame.draw.circle(screen, color, (sx, sy), int(p.radius))
    pygame.draw.circle(screen, (255, 255, 255), (sx, sy), int(p.radius), 2)

    bar_w, bar_h = 40, 5
    hp_ratio = max(0, p.health / p.max_health)
    bx = sx - bar_w // 2
    by = sy - int(p.radius) - 10
    pygame.draw.rect(screen, (80, 0, 0), (bx, by, bar_w, bar_h))
    pygame.draw.rect(screen, (0, 220, 0), (bx, by, int(bar_w * hp_ratio), bar_h))

    if abs(p.vx) > 1 or abs(p.vy) > 1:
        angle = math.atan2(p.vy, p.vx)
        ex = sx + int(math.cos(angle) * p.radius * 1.5)
        ey = sy + int(math.sin(angle) * p.radius * 1.5)
        pygame.draw.line(screen, (255, 255, 0), (sx, sy), (ex, ey), 2)


def draw_enemies(screen, env, cam_x, cam_y):
    for e in env.enemies:
        if not e.alive:
            continue
        sx, sy = world_to_screen(e.x, e.y, cam_x, cam_y)
        color = ENEMY_COLORS.get(e.entity_type, (200, 200, 200))
        pygame.draw.circle(screen, color, (sx, sy), int(e.radius))
        pygame.draw.circle(screen, (0, 0, 0), (sx, sy), int(e.radius), 1)

        bar_w, bar_h = 30, 4
        hp_ratio = max(0, e.health / max(e.max_health, 1))
        bx = sx - bar_w // 2
        by = sy - int(e.radius) - 8
        pygame.draw.rect(screen, (80, 0, 0), (bx, by, bar_w, bar_h))
        pygame.draw.rect(screen, (220, 0, 0), (bx, by, int(bar_w * hp_ratio), bar_h))

        if abs(e.vx) > 1 or abs(e.vy) > 1:
            angle = math.atan2(e.vy, e.vx)
            ex = sx + int(math.cos(angle) * e.radius * 1.3)
            ey = sy + int(math.sin(angle) * e.radius * 1.3)
            pygame.draw.line(screen, (255, 100, 100), (sx, sy), (ex, ey), 2)


def draw_projectiles(screen, env, cam_x, cam_y):
    for p in env.projectiles:
        if not p.alive:
            continue
        sx, sy = world_to_screen(p.x, p.y, cam_x, cam_y)
        color = (255, 255, 100) if p.from_player else (255, 80, 80)
        pygame.draw.circle(screen, color, (sx, sy), int(p.radius))
        angle = math.atan2(p.vy, p.vx)
        tx = sx - int(math.cos(angle) * 8)
        ty = sy - int(math.sin(angle) * 8)
        pygame.draw.line(screen, color, (tx, ty), (sx, sy), 2)


def draw_drops(screen, env, cam_x, cam_y):
    for d in env.drops:
        if not d.alive:
            continue
        sx, sy = world_to_screen(d.x, d.y, cam_x, cam_y)
        color = DROP_COLORS.get(d.drop_type, (255, 255, 255))
        pts = [
            (sx, sy - 8), (sx + 8, sy), (sx, sy + 8), (sx - 8, sy)
        ]
        pygame.draw.polygon(screen, color, pts)
        pygame.draw.polygon(screen, (255, 255, 255), pts, 1)


def draw_grid(screen, cam_x, cam_y):
    left_col = int(math.floor((cam_x - SCREEN_W / 2) / CELL_SIZE))
    right_col = int(math.floor((cam_x + SCREEN_W / 2) / CELL_SIZE))
    top_row = int(math.floor((cam_y - SCREEN_H / 2) / CELL_SIZE))
    bot_row = int(math.floor((cam_y + SCREEN_H / 2) / CELL_SIZE))

    for col in range(left_col - 1, right_col + 2):
        wx = col * CELL_SIZE
        sx, _ = world_to_screen(wx, 0, cam_x, cam_y)
        pygame.draw.line(screen, (40, 40, 40), (sx, 0), (sx, SCREEN_H), 1)

    for row in range(top_row - 1, bot_row + 2):
        wy = row * CELL_SIZE
        _, sy = world_to_screen(0, wy, cam_x, cam_y)
        pygame.draw.line(screen, (40, 40, 40), (0, sy), (SCREEN_W, sy), 1)


def draw_hud(screen, env, font, fps_val, is_ai):
    p = env.player
    lines = [
        f"HP: {max(0, p.health)}/{p.max_health}",
        f"Score: {p.total_score}  Kills: {p.kill_count}",
        f"Stage: {env.current_victory_stage}/5  Level: {p.level}",
        f"Time: {env.game_time:.1f}s  Enemies: {len([e for e in env.enemies if e.alive])}",
        f"Roll CD: {max(0, p.roll_cooldown):.2f}s  Atk CD: {max(0, p.attack_cooldown):.2f}s",
        f"FPS: {fps_val:.0f}  {'[AI]' if is_ai else '[MANUAL]'}",
    ]
    y = 10
    for line in lines:
        surf = font.render(line, True, (220, 220, 220))
        screen.blit(surf, (10, y))
        y += 22

    victory_thresholds = env.victory_thresholds
    next_stage = env.current_victory_stage
    if next_stage < len(victory_thresholds):
        progress = p.total_score / victory_thresholds[next_stage]
        bar_w, bar_h = 200, 12
        bx, by = SCREEN_W - bar_w - 10, 10
        pygame.draw.rect(screen, (40, 40, 40), (bx, by, bar_w, bar_h))
        pygame.draw.rect(screen, (0, 180, 255), (bx, by, int(bar_w * min(progress, 1.0)), bar_h))
        pygame.draw.rect(screen, (100, 100, 100), (bx, by, bar_w, bar_h), 1)
        label = font.render(f"Stage {next_stage + 1}: {p.total_score}/{victory_thresholds[next_stage]}", True, (200, 200, 200))
        screen.blit(label, (bx, by + bar_h + 2))


def get_manual_action(env):
    keys = pygame.key.get_pressed()
    dx, dy = 0.0, 0.0
    if keys[pygame.K_w] or keys[pygame.K_UP]:
        dy -= 1.0
    if keys[pygame.K_s] or keys[pygame.K_DOWN]:
        dy += 1.0
    if keys[pygame.K_a] or keys[pygame.K_LEFT]:
        dx -= 1.0
    if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
        dx += 1.0

    move_idx = 0
    for i, (mx, my) in enumerate(MOVE_DIRS):
        if abs(dx - mx) < 0.01 and abs(dy - my) < 0.01:
            move_idx = i
            break

    attack = 1 if keys[pygame.K_SPACE] or keys[pygame.K_j] else 0
    roll = 1 if keys[pygame.K_LSHIFT] or keys[pygame.K_k] else 0

    return move_idx + NUM_MOVE * (attack + NUM_ATTACK_TARGETS * roll)


def main():
    parser = argparse.ArgumentParser(description="Watch AI play JRFirstGame")
    parser.add_argument("--model", type=str, default=None, help="Path to model (None=manual play)")
    parser.add_argument("--algo", type=str, default="dqn", choices=["dqn", "ppo"])
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--max-steps", type=int, default=36000)
    parser.add_argument("--speed", type=int, default=1, help="Game speed multiplier")
    args = parser.parse_args()

    pygame.init()
    screen = pygame.display.set_mode((SCREEN_W, SCREEN_H))
    pygame.display.set_caption("JRFirstGame - AI Player" if args.model else "JRFirstGame - Manual Play")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("consolas", 18)

    env = JRFirstGameEnv(max_steps=args.max_steps, seed=args.seed)

    model = None
    if args.model:
        from stable_baselines3 import DQN, PPO
        if args.algo == "dqn":
            model = DQN.load(args.model, env=env)
        else:
            model = PPO.load(args.model, env=env)
        print(f"Loaded {args.algo} model from {args.model}")

    obs, info = env.reset()
    is_ai = model is not None

    running = True
    paused = False
    total_reward = 0.0

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_p:
                    paused = not paused
                elif event.key == pygame.K_r:
                    obs, info = env.reset()
                    total_reward = 0.0
                    print("=== Reset ===")

        if paused:
            pause_surf = font.render("PAUSED (P to resume)", True, (255, 255, 0))
            screen.blit(pause_surf, (SCREEN_W // 2 - 100, SCREEN_H // 2))
            pygame.display.flip()
            clock.tick(FPS)
            continue

        for _ in range(args.speed):
            if model is not None:
                action, _ = model.predict(obs, deterministic=True)
            else:
                action = get_manual_action(env)

            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward

            if terminated or truncated:
                print(f"Game Over! Score:{info['score']} Kills:{info['kill_count']} "
                      f"Stage:{info['victory_stage']} Time:{info['game_time']:.1f}s "
                      f"Reward:{total_reward:.1f}")
                obs, info = env.reset()
                total_reward = 0.0

        cam_x = env.player.x
        cam_y = env.player.y

        screen.fill((15, 15, 25))
        draw_grid(screen, cam_x, cam_y)
        draw_drops(screen, env, cam_x, cam_y)
        draw_enemies(screen, env, cam_x, cam_y)
        draw_projectiles(screen, env, cam_x, cam_y)
        draw_player(screen, env, cam_x, cam_y)
        draw_hud(screen, env, font, clock.get_fps(), is_ai)

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()
    env.close()


if __name__ == "__main__":
    main()
