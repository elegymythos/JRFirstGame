import math
import numpy as np
import gymnasium as gym
from gymnasium import spaces


ENEMY_TYPES = {"Slime": 0, "SkeletonWarrior": 1, "SkeletonArcher": 2, "Giant": 3}
DROP_TYPES = {"HealthPotion": 0, "STRBoost": 1, "AgiBoost": 2, "MagBoost": 3, "VitBoost": 4, "RangeBoost": 5}

MAX_ENEMIES = 20
MAX_PROJECTILES = 10
MAX_DROPS = 10
ARENA_SIZE = 800.0

MOVE_DIRS = [
    (0.0, 0.0),
    (0.0, -1.0),
    (0.0, 1.0),
    (-1.0, 0.0),
    (1.0, 0.0),
    (-1.0, -1.0),
    (1.0, -1.0),
    (-1.0, 1.0),
    (1.0, 1.0),
]

ACTION_MEANING = [
    "stay", "up", "down", "left", "right",
    "up_left", "up_right", "down_left", "down_right",
    "attack_nearest", "roll", "pickup",
]
NUM_ACTIONS = len(ACTION_MEANING)

ATTACK_DIRS = {}

PLAYER_STATE_DIM = 20
ENEMY_STATE_DIM = 10
PROJECTILE_STATE_DIM = 6
DROP_STATE_DIM = 4
DANGER_GRID_DIM = 8

OBS_DIM = (
    PLAYER_STATE_DIM
    + MAX_ENEMIES * ENEMY_STATE_DIM
    + MAX_PROJECTILES * PROJECTILE_STATE_DIM
    + MAX_DROPS * DROP_STATE_DIM
    + DANGER_GRID_DIM
    + 2
)

MELEE_HALF_ARC = math.pi / 2.0

DROP_RATES = {
    "Slime": 0.40,
    "SkeletonWarrior": 0.55,
    "SkeletonArcher": 0.45,
    "Giant": 0.80,
}

DROP_TYPE_TABLE = [
    (0, 0.30),
    (1, 0.15),
    (2, 0.15),
    (3, 0.15),
    (4, 0.13),
    (5, 0.12),
]


def _normalize_angle(dx, dy):
    if abs(dx) < 1e-8 and abs(dy) < 1e-8:
        return 0.0, 0.0
    length = math.sqrt(dx * dx + dy * dy)
    return dx / length, dy / length


def _clamp(v, lo, hi):
    return max(lo, min(hi, v))


def _angle_diff(a, b):
    d = a - b
    while d > math.pi:
        d -= 2 * math.pi
    while d < -math.pi:
        d += 2 * math.pi
    return d


class GameEntity:
    __slots__ = ("x", "y", "vx", "vy", "health", "max_health", "radius",
                 "entity_type", "attack_damage", "attack_range", "attack_cooldown",
                 "attack_cooldown_max", "speed", "is_ranged", "score",
                 "target_x", "target_y", "alive", "etype_name")

    def __init__(self):
        self.x = self.y = 0.0
        self.vx = self.vy = 0.0
        self.health = 100
        self.max_health = 100
        self.radius = 16.0
        self.entity_type = 0
        self.attack_damage = 10
        self.attack_range = 50.0
        self.attack_cooldown = 0.0
        self.attack_cooldown_max = 1.0
        self.speed = 80.0
        self.is_ranged = False
        self.score = 15
        self.target_x = self.target_y = 0.0
        self.alive = True
        self.etype_name = "Slime"


class PlayerState:
    __slots__ = ("x", "y", "vx", "vy", "health", "max_health", "radius", "speed",
                 "attack_cooldown", "attack_cooldown_max", "is_rolling",
                 "roll_cooldown", "roll_cooldown_max", "is_invincible",
                 "total_score", "kill_count", "level", "victory_stage",
                 "crit_chance", "crit_multiplier", "attack_damage", "attack_range",
                 "damage_bonus", "attack_speed_bonus",
                 "roll_timer", "facing_angle", "vitality", "strength",
                 "victory_stage_health_bonus")

    def __init__(self):
        self.x = self.y = 0.0
        self.vx = self.vy = 0.0
        self.vitality = 10
        self.strength = 15
        self.victory_stage_health_bonus = 0
        self.max_health = 120 + self.vitality * 8
        self.health = self.max_health
        self.radius = 18.0
        self.speed = 150.0
        self.attack_cooldown = 0.0
        self.attack_cooldown_max = 0.5
        self.is_rolling = False
        self.roll_timer = 0.0
        self.roll_cooldown = 0.0
        self.roll_cooldown_max = 0.8
        self.is_invincible = False
        self.total_score = 0
        self.kill_count = 0
        self.level = 1
        self.victory_stage = 0
        self.crit_chance = 0.05
        self.crit_multiplier = 1.5
        self.attack_damage = 20
        self.attack_range = 80.0
        self.damage_bonus = 0.0
        self.attack_speed_bonus = 0.0
        self.facing_angle = 0.0


class ProjectileState:
    __slots__ = ("x", "y", "vx", "vy", "damage", "from_player", "alive", "radius",
                 "start_x", "start_y", "max_distance")

    def __init__(self):
        self.x = self.y = 0.0
        self.vx = self.vy = 0.0
        self.damage = 10
        self.from_player = True
        self.alive = True
        self.radius = 4.0
        self.start_x = self.start_y = 0.0
        self.max_distance = float('inf')


class DropState:
    __slots__ = ("x", "y", "drop_type", "alive")

    def __init__(self):
        self.x = self.y = 0.0
        self.drop_type = 0
        self.alive = True


class JRFirstGameEnv(gym.Env):
    metadata = {"render_modes": ["human", "ansi"]}

    def __init__(self, render_mode=None, max_steps=36000, seed=None,
                 difficulty_level=0):
        super().__init__()
        self.render_mode = render_mode
        self.max_steps = max_steps
        self.difficulty_level = difficulty_level

        self.observation_space = spaces.Box(
            low=-1.0, high=1.0, shape=(OBS_DIM,), dtype=np.float32
        )
        self.action_space = spaces.Discrete(NUM_ACTIONS)

        self.rng = np.random.default_rng(seed)
        self._reset_state()

    def _score_mult(self):
        return 1.0 + min(self.player.total_score / 50.0 * 0.1, 1.0)

    def _effective_speed(self):
        return 150.0 * self._score_mult()

    def _effective_roll_speed(self):
        return 400.0 * self._score_mult()

    def _reset_state(self):
        self.player = PlayerState()
        self.enemies: list[GameEntity] = []
        self.projectiles: list[ProjectileState] = []
        self.drops: list[DropState] = []
        self.game_time = 0.0
        self.step_count = 0
        self.explored_regions: set[tuple[int, int]] = set()
        self.victory_thresholds = [100, 1000, 2000, 5000, 10000]
        self.current_victory_stage = 0
        self.dt = 1.0 / 60.0
        self._prev_health = self.player.max_health
        self._prev_score = 0
        self._prev_kill_count = 0
        self._prev_nearest_dist = float('inf')
        self._prev_in_range = False
        self._steps_since_kill = 0
        self._prev_enemy_total_hp = 0
        self._prev_strength = self.player.strength
        self._prev_damage_bonus = self.player.damage_bonus
        self._prev_vitality = self.player.vitality
        self._prev_attack_range = self.player.attack_range
        self._prev_aspd_bonus = self.player.attack_speed_bonus
        self._effective_difficulty = self.difficulty_level
        self._recent_hit_steps = []
        self._spawn_initial_enemies()

    def _get_enemy_config_for_difficulty(self, dist):
        """与 C++ MapSystem::assignEnemyConfig 完全一致"""
        if dist == 0:
            return [("Slime", 1)]
        elif dist <= 2:
            return [("Slime", 1), ("SkeletonWarrior", 1)]
        elif dist <= 4:
            return [("Slime", 1), ("SkeletonWarrior", 1), ("SkeletonArcher", 1)]
        elif dist <= 6:
            return [("SkeletonWarrior", 2), ("SkeletonArcher", 1)]
        elif dist <= 8:
            return [("SkeletonArcher", 2), ("Giant", 1)]
        else:
            n_giant = 1 + dist // 8
            n_warrior = 1 + dist // 6
            n_archer = 1 + dist // 8
            return [("Giant", n_giant), ("SkeletonWarrior", n_warrior), ("SkeletonArcher", n_archer)]

    def _spawn_initial_enemies(self):
        self._spawn_enemies_for_region(0, 0)

    def _spawn_enemies_for_region(self, col, row):
        key = (col, row)
        if key in self.explored_regions:
            return
        self.explored_regions.add(key)

        dist = abs(col) + abs(row)
        cx = col * ARENA_SIZE + ARENA_SIZE / 2
        cy = row * ARENA_SIZE + ARENA_SIZE / 2

        configs = self._get_enemy_config_for_difficulty(dist)
        scale = self._difficulty_scale()
        for etype, count in configs:
            for _ in range(count):
                e = self._create_enemy(etype, cx, cy, scale)
                self.enemies.append(e)

    def _difficulty_scale(self):
        diff_scale = 1.0 + len(self.explored_regions) * 0.05
        time_scale = 1.0 + self.game_time / 600.0
        score_scale = 1.0 + self.player.total_score / 1000.0
        return diff_scale * time_scale * score_scale

    def _create_enemy(self, etype, cx, cy, scale):
        e = GameEntity()
        e.entity_type = ENEMY_TYPES[etype]
        e.etype_name = etype
        e.x = cx + (self.rng.random() - 0.5) * ARENA_SIZE * 0.8
        e.y = cy + (self.rng.random() - 0.5) * ARENA_SIZE * 0.8
        e.target_x = self.player.x
        e.target_y = self.player.y

        if etype == "Slime":
            e.health = e.max_health = int(40 * scale)
            e.attack_damage = int(8 * scale)
            e.speed = 50 * math.sqrt(scale)
            e.attack_range = 30
            e.attack_cooldown_max = 0.8
            e.score = int(15 * scale)
            e.radius = 12
            e.is_ranged = False
        elif etype == "SkeletonWarrior":
            e.health = e.max_health = int(80 * scale)
            e.attack_damage = int(15 * scale)
            e.speed = 80 * math.sqrt(scale)
            e.attack_range = 50
            e.attack_cooldown_max = 1.0
            e.score = int(30 * scale)
            e.radius = 16
            e.is_ranged = False
        elif etype == "SkeletonArcher":
            e.health = e.max_health = int(50 * scale)
            e.attack_damage = int(12 * scale)
            e.speed = 70 * math.sqrt(scale)
            e.attack_range = 250
            e.attack_cooldown_max = 1.5
            e.score = int(25 * scale)
            e.radius = 14
            e.is_ranged = True
        elif etype == "Giant":
            e.health = e.max_health = int(200 * scale)
            e.attack_damage = int(30 * scale)
            e.speed = 25 * math.sqrt(scale)
            e.attack_range = 60
            e.attack_cooldown_max = 2.5
            e.score = int(60 * scale)
            e.radius = 28
            e.is_ranged = False

        return e

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)
        if seed is not None:
            self.rng = np.random.default_rng(seed)
        self._reset_state()
        return self._get_obs(), self._get_info()

    def step(self, action):
        action_name = ACTION_MEANING[action]

        if action_name in ("stay", "up", "down", "left", "right",
                           "up_left", "up_right", "down_left", "down_right"):
            idx = ACTION_MEANING.index(action_name)
            dx, dy = MOVE_DIRS[idx]
            self._apply_movement(dx, dy)
            if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                self.player.facing_angle = math.atan2(dy, dx)
        elif action_name == "attack_nearest":
            nearest = None
            min_dist = float('inf')
            for e in self.enemies:
                if e.alive:
                    d = math.sqrt((e.x - self.player.x)**2 + (e.y - self.player.y)**2)
                    if d < min_dist:
                        min_dist = d
                        nearest = e
            if nearest is not None:
                dx = nearest.x - self.player.x
                dy = nearest.y - self.player.y
                length = math.sqrt(dx*dx + dy*dy)
                if length > 1e-6:
                    self.player.facing_angle = math.atan2(dy, dx)
                    move_dx = (dx / length) * 0.4
                    move_dy = (dy / length) * 0.4
                    self._apply_movement(move_dx, move_dy)
            if self._can_attack():
                self._attack_dist = min_dist if nearest else float('inf')
                self._do_attack("nearest")
        elif action_name == "roll":
            if self._can_roll():
                fa = self.player.facing_angle
                dx = math.cos(fa)
                dy = math.sin(fa)
                self._do_roll(dx, dy)
            else:
                self._apply_movement(0.0, 0.0)
        elif action_name == "pickup":
            self._apply_movement(0.0, 0.0)
            self._move_toward_nearest_drop()

        self._update_roll(self.dt)
        self._update_player_position(self.dt)
        self._update_enemies(self.dt)
        self._update_enemy_attacks(self.dt)
        self._update_projectiles(self.dt)
        self._update_drops(self.dt)
        self._check_pickups()
        self._check_enemy_deaths()
        self._check_region_spawns()
        self._check_victory_stage()
        self._update_adaptive_difficulty()

        self.game_time += self.dt
        self.step_count += 1

        reward = self._compute_reward(action)
        reward = max(-50.0, min(reward, 50.0))
        terminated = self.player.health <= 0
        truncated = self.step_count >= self.max_steps

        self._prev_health = self.player.health
        self._prev_score = self.player.total_score
        self._prev_kill_count = self.player.kill_count

        return self._get_obs(), reward, terminated, truncated, self._get_info()

    def _move_toward_nearest_drop(self):
        nearest = None
        min_dist = float('inf')
        health_ratio = max(0, self.player.health) / self.player.max_health
        for d in self.drops:
            if not d.alive:
                continue
            dd = math.sqrt((d.x - self.player.x) ** 2 + (d.y - self.player.y) ** 2)
            weight = dd
            if d.drop_type == 0 and health_ratio < 0.6:
                weight *= 0.5
            if weight < min_dist:
                min_dist = weight
                nearest = d
        if nearest is not None and min_dist < 400:
            dx = nearest.x - self.player.x
            dy = nearest.y - self.player.y
            length = math.sqrt(dx * dx + dy * dy)
            if length > 1e-6:
                spd = self._effective_speed()
                self.player.vx = (dx / length) * spd
                self.player.vy = (dy / length) * spd
                self.player.facing_angle = math.atan2(dy, dx)

    def _can_attack(self):
        return self.player.attack_cooldown <= 0 and not self.player.is_rolling

    def _can_roll(self):
        return self.player.roll_cooldown <= 0 and not self.player.is_rolling

    def _apply_movement(self, dx, dy):
        if not self.player.is_rolling:
            spd = self._effective_speed()
            self.player.vx = dx * spd
            self.player.vy = dy * spd

    def _do_roll(self, dx, dy):
        length = math.sqrt(dx * dx + dy * dy)
        if length < 1e-6:
            dx, dy = 1.0, 0.0
            length = 1.0
        self.player.is_rolling = True
        self.player.is_invincible = True
        self.player.roll_timer = 0.3
        self.player.roll_cooldown = self.player.roll_cooldown_max
        roll_speed = self._effective_roll_speed()
        self.player.vx = (dx / length) * roll_speed
        self.player.vy = (dy / length) * roll_speed
        self.player.facing_angle = math.atan2(dy / length, dx / length)

    def _update_roll(self, dt):
        if self.player.is_rolling:
            self.player.roll_timer -= dt
            if self.player.roll_timer <= 0:
                self.player.is_rolling = False
                self.player.is_invincible = False

    def _update_player_position(self, dt):
        self.player.x += self.player.vx * dt
        self.player.y += self.player.vy * dt
        if self.player.attack_cooldown > 0:
            self.player.attack_cooldown -= dt
        if self.player.roll_cooldown > 0:
            self.player.roll_cooldown -= dt

    def _do_attack(self, target_strategy="nearest"):
        target = self._select_attack_target(target_strategy)
        if target is None:
            target = self._find_nearest_enemy()
        if target is None:
            return

        dx = target.x - self.player.x
        dy = target.y - self.player.y
        if abs(dx) > 1e-6 or abs(dy) > 1e-6:
            self.player.facing_angle = math.atan2(dy, dx)

        self.player.attack_cooldown = max(
            0.05,
            self.player.attack_cooldown_max / (1.0 + self.player.attack_speed_bonus)
        )

        score_mult = self._score_mult()
        base_dmg = self.player.attack_damage
        attr_bonus = self.player.strength * 0.8
        bonus = int(base_dmg * self.player.damage_bonus)
        total_dmg = int((base_dmg + attr_bonus + bonus) * score_mult)

        if self.rng.random() < self.player.crit_chance:
            total_dmg = int(total_dmg * self.player.crit_multiplier)

        if self.player.attack_range <= 80:
            self._melee_attack(target, total_dmg)
        else:
            self._ranged_attack(target, total_dmg)

    def _select_attack_target(self, strategy):
        alive_enemies = [e for e in self.enemies if e.alive]
        if not alive_enemies:
            return None

        if strategy == "nearest":
            return min(alive_enemies, key=lambda e: (e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
        elif strategy == "weakest":
            return min(alive_enemies, key=lambda e: e.health)
        elif strategy == "ranged_priority":
            ranged = [e for e in alive_enemies if e.is_ranged]
            if ranged:
                return min(ranged, key=lambda e: (e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
            return min(alive_enemies, key=lambda e: (e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
        return None

    def _melee_attack(self, nearest, dmg):
        facing = self.player.facing_angle
        for e in self.enemies:
            if not e.alive:
                continue
            dist = math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
            if dist <= self.player.attack_range:
                angle_to_enemy = math.atan2(e.y - self.player.y, e.x - self.player.x)
                if abs(_angle_diff(angle_to_enemy, facing)) <= MELEE_HALF_ARC:
                    e.health -= dmg
                    if e.health <= 0:
                        e.alive = False

    def _ranged_attack(self, nearest, dmg):
        dx = nearest.x - self.player.x
        dy = nearest.y - self.player.y
        length = math.sqrt(dx * dx + dy * dy)
        if length < 1e-6:
            return
        proj = ProjectileState()
        proj.x = self.player.x
        proj.y = self.player.y
        proj.start_x = self.player.x
        proj.start_y = self.player.y
        proj_speed = 400.0
        proj.vx = (dx / length) * proj_speed
        proj.vy = (dy / length) * proj_speed
        proj.damage = dmg
        proj.from_player = True
        proj.max_distance = float('inf')
        self.projectiles.append(proj)

    def _find_nearest_enemy(self):
        nearest = None
        min_dist = float("inf")
        for e in self.enemies:
            if not e.alive:
                continue
            dist = math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
            if dist < min_dist:
                min_dist = dist
                nearest = e
        return nearest

    def _update_enemies(self, dt):
        for e in self.enemies:
            if not e.alive:
                continue
            e.target_x = self.player.x
            e.target_y = self.player.y
            dx = e.target_x - e.x
            dy = e.target_y - e.y
            dist = math.sqrt(dx * dx + dy * dy)

            if dist < 1e-6:
                e.vx = e.vy = 0.0
                continue

            ndx, ndy = dx / dist, dy / dist

            if e.is_ranged:
                if dist < 150:
                    e.vx = -ndx * e.speed
                    e.vy = -ndy * e.speed
                elif dist > 250:
                    e.vx = ndx * e.speed
                    e.vy = ndy * e.speed
                else:
                    e.vx = e.vy = 0.0
            else:
                e.vx = ndx * e.speed
                e.vy = ndy * e.speed

            e.x += e.vx * dt
            e.y += e.vy * dt
            if e.attack_cooldown > 0:
                e.attack_cooldown -= dt

    def _update_enemy_attacks(self, dt):
        for e in self.enemies:
            if not e.alive or e.attack_cooldown > 0:
                continue
            dist = math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
            if e.is_ranged:
                if dist > e.attack_range + self.player.radius:
                    continue
            else:
                if dist > e.radius + self.player.radius:
                    continue

            if e.is_ranged:
                proj = ProjectileState()
                proj.x = e.x
                proj.y = e.y
                proj.start_x = e.x
                proj.start_y = e.y
                dx = self.player.x - e.x
                dy = self.player.y - e.y
                length = math.sqrt(dx * dx + dy * dy)
                if length < 1e-6:
                    continue
                proj_speed = 250.0
                proj.vx = (dx / length) * proj_speed
                proj.vy = (dy / length) * proj_speed
                proj.damage = e.attack_damage
                proj.from_player = False
                proj.max_distance = e.attack_range
                self.projectiles.append(proj)
            else:
                if not self.player.is_invincible:
                    self.player.health = max(0, self.player.health - e.attack_damage)
                    kbx = self.player.x - e.x
                    kby = self.player.y - e.y
                    kbl = math.sqrt(kbx * kbx + kby * kby)
                    if kbl > 1e-6:
                        self.player.x += (kbx / kbl) * 20.0
                        self.player.y += (kby / kbl) * 20.0

            e.attack_cooldown = e.attack_cooldown_max

    def _update_projectiles(self, dt):
        for p in self.projectiles:
            if not p.alive:
                continue
            p.x += p.vx * dt
            p.y += p.vy * dt

            if not p.from_player and p.max_distance < float('inf'):
                traveled = math.sqrt((p.x - p.start_x) ** 2 + (p.y - p.start_y) ** 2)
                if traveled >= p.max_distance:
                    p.alive = False
                    continue

            if p.from_player:
                for e in self.enemies:
                    if not e.alive:
                        continue
                    dist = math.sqrt((p.x - e.x) ** 2 + (p.y - e.y) ** 2)
                    if dist <= p.radius + e.radius:
                        e.health -= p.damage
                        p.alive = False
                        if e.health <= 0:
                            e.alive = False
                        break
            else:
                if not self.player.is_invincible:
                    dist = math.sqrt((p.x - self.player.x) ** 2 + (p.y - self.player.y) ** 2)
                    if dist <= p.radius + self.player.radius:
                        self.player.health = max(0, self.player.health - p.damage)
                        p.alive = False

        self.projectiles = [p for p in self.projectiles if p.alive]
        if len(self.projectiles) > 100:
            self.projectiles = self.projectiles[-100:]

    def _update_drops(self, dt):
        pass

    def _check_pickups(self):
        for d in self.drops:
            if not d.alive:
                continue
            dist = math.sqrt((d.x - self.player.x) ** 2 + (d.y - self.player.y) ** 2)
            if dist <= 30 + self.player.radius:
                d.alive = False
                if d.drop_type == DROP_TYPES["HealthPotion"]:
                    heal = self.player.max_health / 3.0
                    self.player.health = min(self.player.health + heal, self.player.max_health)
                elif d.drop_type == DROP_TYPES["STRBoost"]:
                    self.player.strength += 2
                elif d.drop_type == DROP_TYPES["AgiBoost"]:
                    self.player.attack_speed_bonus += 0.05
                elif d.drop_type == DROP_TYPES["MagBoost"]:
                    self.player.damage_bonus += 0.15
                elif d.drop_type == DROP_TYPES["VitBoost"]:
                    self.player.vitality += 2
                    self.player.max_health = 120 + self.player.vitality * 8 + self.player.victory_stage_health_bonus
                elif d.drop_type == DROP_TYPES["RangeBoost"]:
                    self.player.attack_range += 8

    def _select_drop_type(self):
        r = self.rng.random()
        cumulative = 0.0
        for dtype, prob in DROP_TYPE_TABLE:
            cumulative += prob
            if r < cumulative:
                return dtype
        return DROP_TYPE_TABLE[-1][0]

    def _check_enemy_deaths(self):
        for e in self.enemies:
            if not e.alive and e.health <= 0:
                if e.score > 0:
                    self.player.total_score += e.score
                    self.player.kill_count += 1
                    e.score = 0
                    drop_rate = DROP_RATES.get(e.etype_name, 0.15)
                    if self.rng.random() < drop_rate:
                        drop = DropState()
                        drop.x = e.x
                        drop.y = e.y
                        drop.drop_type = self._select_drop_type()
                        self.drops.append(drop)

        self.enemies = [e for e in self.enemies if e.alive]

    def _check_region_spawns(self):
        col = int(math.floor(self.player.x / ARENA_SIZE))
        row = int(math.floor(self.player.y / ARENA_SIZE))
        for dc in range(-1, 2):
            for dr in range(-1, 2):
                self._spawn_enemies_for_region(col + dc, row + dr)

    def _check_victory_stage(self):
        if self.current_victory_stage < len(self.victory_thresholds):
            threshold = self.victory_thresholds[self.current_victory_stage]
            if self.player.total_score >= threshold:
                self.current_victory_stage += 1
                self.player.victory_stage = self.current_victory_stage
                self.player.victory_stage_health_bonus += 30
                self.player.max_health = 120 + self.player.vitality * 8 + self.player.victory_stage_health_bonus
                self.player.health = self.player.max_health
                self.player.damage_bonus += 0.3
                self.player.attack_speed_bonus += 0.2
                self.player.crit_chance += 0.12
                self.player.crit_multiplier += 0.2
                self.player.attack_range += 10

    def _update_adaptive_difficulty(self):
        """根据 score 和 game_time 自适应提升难度，模拟实际游戏体验"""
        score = self.player.total_score
        base = self.difficulty_level
        if score >= 2000:
            target = max(base, 3)
        elif score >= 1000:
            target = max(base, 2)
        elif score >= 300:
            target = max(base, 1)
        else:
            target = base
        self._effective_difficulty = min(target, 3)

    def _compute_reward(self, action):
        reward = 0.0
        action_name = ACTION_MEANING[action]

        score_delta = self.player.total_score - self._prev_score
        reward += score_delta * 5.0

        health_delta = self._prev_health - self.player.health
        if health_delta > 0:
            reward -= health_delta * 0.3

        health_ratio = max(0, self.player.health) / self.player.max_health
        if health_ratio < 0.2:
            reward -= 0.5 * (1.0 - health_ratio)

        if self.player.health <= 0:
            reward -= 150.0

        kill_delta = self.player.kill_count - self._prev_kill_count
        if kill_delta > 0:
            reward += 30.0 * kill_delta
            self._steps_since_kill = 0
        else:
            self._steps_since_kill += 1

        enemy_total_hp = sum(e.health for e in self.enemies if e.alive and e.health > 0)
        enemy_hp_delta = self._prev_enemy_total_hp - enemy_total_hp
        if enemy_hp_delta > 0:
            reward += enemy_hp_delta * 0.3
        self._prev_enemy_total_hp = enemy_total_hp

        if score_delta > 0:
            reward += 20.0 + self.current_victory_stage * 20.0

        prev_stage = 0
        for i, threshold in enumerate(self.victory_thresholds):
            if self._prev_score >= threshold:
                prev_stage = i + 1
        if self.current_victory_stage > prev_stage:
            reward += 300.0

        if self.current_victory_stage > 0:
            reward += self.current_victory_stage * 2.0

        nearest_dist = float('inf')
        nearest_enemy = None
        for e in self.enemies:
            if e.alive:
                d = math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
                if d < nearest_dist:
                    nearest_dist = d
                    nearest_enemy = e

        alive_enemy_count = sum(1 for e in self.enemies if e.alive)

        if nearest_enemy is not None:
            atk_range = self.player.attack_range + nearest_enemy.radius
            can_attack = self.player.attack_cooldown <= 0 and not self.player.is_rolling

            if nearest_dist <= atk_range:
                reward += 0.15
                if can_attack and action_name == "attack_nearest":
                    reward += 1.0
            elif nearest_dist <= atk_range * 2.0:
                approach_delta = self._prev_nearest_dist - nearest_dist
                if approach_delta > 0:
                    reward += 0.15 * min(approach_delta / 10.0, 1.0)
                if can_attack and action_name == "attack_nearest":
                    reward += 0.75
            elif nearest_dist <= 300:
                approach_delta = self._prev_nearest_dist - nearest_dist
                if approach_delta > 0:
                    reward += 0.075 * min(approach_delta / 10.0, 1.0)
            elif nearest_dist <= 600:
                approach_delta = self._prev_nearest_dist - nearest_dist
                if approach_delta > 0:
                    reward += 0.025 * min(approach_delta / 10.0, 1.0)

            if action_name == "stay" and alive_enemy_count > 0 and can_attack:
                reward -= 0.5
        else:
            if action_name != "stay":
                reward += 0.2

        self._prev_nearest_dist = nearest_dist if nearest_enemy else float('inf')

        if not alive_enemy_count:
            if action_name != "stay":
                reward += 0.5
            else:
                reward -= 0.3

        if alive_enemy_count > 0 and self._steps_since_kill > 120:
            reward -= 0.4 * min((self._steps_since_kill - 120) / 120.0, 2.0)

        if alive_enemy_count == 0 and self._steps_since_kill > 60:
            reward -= 0.25 * min((self._steps_since_kill - 60) / 60.0, 2.0)

        if alive_enemy_count > 0 and action_name == "stay":
            can_attack = self.player.attack_cooldown <= 0 and not self.player.is_rolling
            if can_attack and nearest_enemy is not None and nearest_dist <= self.player.attack_range + nearest_enemy.radius:
                pass
            else:
                reward -= 0.15

        if alive_enemy_count > 0 and nearest_enemy is not None and nearest_dist > 300:
            if action_name != "stay" and action_name != "roll":
                approach_delta = self._prev_nearest_dist - nearest_dist
                if approach_delta > 0:
                    reward += 0.1 * min(approach_delta / 10.0, 1.0)

        # === 拾取掉落物奖励 ===
        prev_str = getattr(self, '_prev_strength', self.player.strength)
        prev_dmg_bonus = getattr(self, '_prev_damage_bonus', self.player.damage_bonus)
        prev_vit = getattr(self, '_prev_vitality', self.player.vitality)
        prev_range = getattr(self, '_prev_attack_range', self.player.attack_range)
        prev_aspd = getattr(self, '_prev_aspd_bonus', self.player.attack_speed_bonus)

        str_gained = self.player.strength - prev_str
        dmg_bonus_gained = self.player.damage_bonus - prev_dmg_bonus
        vit_gained = self.player.vitality - prev_vit
        range_gained = self.player.attack_range - prev_range
        aspd_gained = self.player.attack_speed_bonus - prev_aspd

        if str_gained > 0:
            reward += 2.5 * str_gained
        if dmg_bonus_gained > 0:
            reward += 2.5
        if vit_gained > 0:
            reward += 1.5 * vit_gained
        if range_gained > 0:
            reward += 1.5
        if aspd_gained > 0:
            reward += 1.5

        self._prev_strength = self.player.strength
        self._prev_damage_bonus = self.player.damage_bonus
        self._prev_vitality = self.player.vitality
        self._prev_attack_range = self.player.attack_range
        self._prev_aspd_bonus = self.player.attack_speed_bonus

        # 鼓励主动拾取：低血量时接近血瓶，任何时候接近属性掉落物
        nearest_blood_dist = float('inf')
        nearest_buff_dist = float('inf')
        for d in self.drops:
            if not d.alive:
                continue
            dd = math.sqrt((d.x - self.player.x) ** 2 + (d.y - self.player.y) ** 2)
            if d.drop_type == 0:
                if dd < nearest_blood_dist:
                    nearest_blood_dist = dd
            else:
                if dd < nearest_buff_dist:
                    nearest_buff_dist = dd

        if nearest_blood_dist < 200 and health_ratio < 0.5:
            reward += 0.3
            if nearest_blood_dist < 50:
                reward += 0.5

        if nearest_buff_dist < 150 and alive_enemy_count == 0:
            reward += 0.5
            if action_name == "pickup":
                reward += 1.0

        if action_name == "pickup" and nearest_buff_dist < 300:
            reward += 0.3

        # === 风筝打法奖励 ===
        if enemy_hp_delta > 0 and nearest_enemy is not None:
            retreat_delta = nearest_dist - self._prev_nearest_dist
            if retreat_delta > 5.0 and nearest_dist < atk_range * 2.5:
                reward += 0.4 * min(retreat_delta / 20.0, 1.0)

        nearby_melee = sum(1 for e in self.enemies
                          if e.alive and not e.is_ranged
                          and math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2) < 150)
        if nearby_melee >= 2:
            if nearest_enemy is not None:
                retreat_delta = nearest_dist - self._prev_nearest_dist
                if retreat_delta > 3.0:
                    reward += 0.25 * min(retreat_delta / 15.0, 1.0)

        if nearest_enemy is not None and nearest_enemy.is_ranged:
            if nearest_dist < 80:
                retreat_delta = nearest_dist - self._prev_nearest_dist
                if retreat_delta > 3.0:
                    reward += 0.2

        # === 弹幕预判闪避奖励 ===
        incoming_danger = 0.0
        dodge_dir_x, dodge_dir_y = 0.0, 0.0
        for p in self.projectiles:
            if p.alive and not p.from_player:
                pdx = p.x - self.player.x
                pdy = p.y - self.player.y
                pdist = math.sqrt(pdx * pdx + pdy * pdy)
                if pdist < 200:
                    danger = 1.0 - pdist / 200.0
                    incoming_danger += danger
                    proj_dx_norm = p.vx / max(math.sqrt(p.vx**2 + p.vy**2), 1e-6)
                    proj_dy_norm = p.vy / max(math.sqrt(p.vx**2 + p.vy**2), 1e-6)
                    if pdist < 150:
                        perp_x = -proj_dy_norm
                        perp_y = proj_dx_norm
                        dot = pdx * proj_dx_norm + pdy * proj_dy_norm
                        if dot > 0:
                            dodge_dir_x += perp_x * danger
                            dodge_dir_y += perp_y * danger

        # 预判 Archer 攻击：敌人攻击冷却快结束，提前移动
        archer_about_to_attack = 0
        for e in self.enemies:
            if e.alive and e.is_ranged:
                d = math.sqrt((e.x - self.player.x)**2 + (e.y - self.player.y)**2)
                if d < e.attack_range:
                    if e.attack_cooldown < 0.5:
                        archer_about_to_attack += 1

        if archer_about_to_attack > 0:
            if action_name == "roll":
                reward += 1.5 * archer_about_to_attack
            elif abs(self.player.vx) > 10 or abs(self.player.vy) > 10:
                reward += 0.75 * archer_about_to_attack
            if action_name == "stay":
                reward -= 0.5 * archer_about_to_attack
            if action_name == "attack_nearest":
                reward -= 0.15 * archer_about_to_attack

        if incoming_danger > 0:
            if self.player.is_rolling:
                reward += 2.5 * min(incoming_danger, 3.0)
            elif dodge_dir_x != 0 or dodge_dir_y != 0:
                move_dx = self.player.vx / max(abs(self.player.vx) + abs(self.player.vy), 1e-6)
                move_dy = self.player.vy / max(abs(self.player.vx) + abs(self.player.vy), 1e-6)
                dodge_len = math.sqrt(dodge_dir_x**2 + dodge_dir_y**2)
                if dodge_len > 1e-6:
                    cos_angle = (move_dx * dodge_dir_x + move_dy * dodge_dir_y) / dodge_len
                    if cos_angle > 0.3:
                        reward += 1.5 * min(incoming_danger, 2.0)

        if incoming_danger > 0.5 and action_name == "stay":
            reward -= 0.75

        if incoming_danger > 1.0 and action_name == "attack_nearest":
            reward -= 0.25

        # === 风筝打法奖励（攻击-撤退-攻击循环）===
        if nearest_enemy is not None:
            atk_range = self.player.attack_range + nearest_enemy.radius

            if enemy_hp_delta > 0 and nearest_dist > atk_range * 0.8:
                retreat_delta = nearest_dist - self._prev_nearest_dist
                if retreat_delta > 3.0 and nearest_dist < atk_range * 3.0:
                    reward += 0.5 * min(retreat_delta / 15.0, 1.0)

            if enemy_hp_delta > 0 and nearest_dist <= atk_range * 1.2:
                self._recent_hit_steps.append(self.step_count)
                if len(self._recent_hit_steps) > 20:
                    self._recent_hit_steps = self._recent_hit_steps[-20:]

            if len(self._recent_hit_steps) >= 2:
                last_hit = self._recent_hit_steps[-1]
                prev_hit = self._recent_hit_steps[-2]
                gap = last_hit - prev_hit
                if 30 <= gap <= 200:
                    retreat_in_gap = False
                    if self._prev_nearest_dist > nearest_dist + 10:
                        pass
                    reward += 0.15

            nearby_melee = sum(1 for e in self.enemies
                              if e.alive and not e.is_ranged
                              and math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2) < 120)
            if nearby_melee >= 2:
                retreat_delta = nearest_dist - self._prev_nearest_dist
                if retreat_delta > 3.0:
                    reward += 0.4 * min(retreat_delta / 15.0, 1.0)
                if action_name == "stay":
                    reward -= 0.25

            if nearest_enemy.is_ranged:
                ideal_dist = atk_range * 0.9
                dist_error = abs(nearest_dist - ideal_dist) / ideal_dist
                if dist_error < 0.3:
                    reward += 0.15
                if nearest_dist < 60:
                    retreat_delta = nearest_dist - self._prev_nearest_dist
                    if retreat_delta > 3.0:
                        reward += 0.3

        alive_ranged = sum(1 for e in self.enemies if e.alive and e.is_ranged)
        if alive_ranged > 0 and action_name == "roll" and incoming_danger > 0.3:
            reward += 0.5

        # 攻击范围误判惩罚
        if action_name == "attack_nearest" and hasattr(self, '_attack_dist'):
            atk_range = self.player.attack_range
            if self._attack_dist > atk_range * 1.2:
                reward -= 0.5 * (self._attack_dist / atk_range - 1.0)

        if self.player.health > 0 and health_ratio > 0.3:
            reward += 0.15

        if self.current_victory_stage >= 2:
            reward += 3.0

        return reward

    def _get_obs(self):
        obs = np.zeros(OBS_DIM, dtype=np.float32)
        idx = 0

        obs[idx] = _clamp(self.player.x / 10000.0, -1, 1); idx += 1
        obs[idx] = _clamp(self.player.y / 10000.0, -1, 1); idx += 1
        obs[idx] = self.player.health / self.player.max_health; idx += 1
        obs[idx] = _clamp(self.player.vx / 400.0, -1, 1); idx += 1
        obs[idx] = _clamp(self.player.vy / 400.0, -1, 1); idx += 1
        obs[idx] = float(self.player.is_rolling); idx += 1
        obs[idx] = 1.0 - _clamp(self.player.attack_cooldown / max(self.player.attack_cooldown_max, 1e-6), 0, 1); idx += 1
        obs[idx] = 1.0 - _clamp(self.player.roll_cooldown / max(self.player.roll_cooldown_max, 1e-6), 0, 1); idx += 1
        obs[idx] = _clamp(self.player.total_score / 10000.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.level / 50.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.current_victory_stage / 5.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.kill_count / 100.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.attack_range / 300.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.attack_damage / 100.0, 0, 1); idx += 1
        obs[idx] = float(self.player.is_invincible); idx += 1
        obs[idx] = _clamp(self.player.damage_bonus / 3.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.strength / 50.0, 0, 1); idx += 1
        obs[idx] = _clamp(self.player.crit_chance / 1.0, 0, 1); idx += 1

        nearest_enemy = None
        nearest_dist = float('inf')
        for e in self.enemies:
            if e.alive:
                d = (e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2
                if d < nearest_dist:
                    nearest_dist = d
                    nearest_enemy = e

        if nearest_enemy is not None:
            ndist = math.sqrt(nearest_dist)
            in_atk_range = 1.0 if ndist <= self.player.attack_range else 0.0
        else:
            in_atk_range = 0.0

        nearby_count = sum(1 for e in self.enemies
                          if e.alive and math.sqrt((e.x - self.player.x)**2 + (e.y - self.player.y)**2) < 300)

        obs[idx] = in_atk_range; idx += 1
        obs[idx] = _clamp(float(nearby_count) / 10.0, 0, 1); idx += 1

        if nearest_enemy is not None:
            ndx, ndy = _normalize_angle(nearest_enemy.x - self.player.x, nearest_enemy.y - self.player.y)
        else:
            ndx, ndy = 0.0, 0.0

        sorted_enemies = sorted(
            [e for e in self.enemies if e.alive],
            key=lambda e: (e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2
        )[:MAX_ENEMIES]

        for i in range(MAX_ENEMIES):
            if i < len(sorted_enemies):
                e = sorted_enemies[i]
                obs[idx] = e.entity_type / 3.0; idx += 1
                obs[idx] = _clamp((e.x - self.player.x) / 800.0, -1, 1); idx += 1
                obs[idx] = _clamp((e.y - self.player.y) / 800.0, -1, 1); idx += 1
                obs[idx] = e.health / max(e.max_health, 1); idx += 1
                dist = math.sqrt((e.x - self.player.x) ** 2 + (e.y - self.player.y) ** 2)
                obs[idx] = _clamp(dist / 800.0, 0, 1); idx += 1
                obs[idx] = float(e.is_ranged); idx += 1
                evx, evy = _normalize_angle(e.vx, e.vy)
                obs[idx] = evx; idx += 1
                obs[idx] = evy; idx += 1
                obs[idx] = _clamp(e.attack_cooldown / max(e.attack_cooldown_max, 1e-6), 0, 1); idx += 1
                e_in_atk = 1.0 if dist <= self.player.attack_range + e.radius else 0.0
                obs[idx] = e_in_atk; idx += 1
            else:
                idx += ENEMY_STATE_DIM

        sorted_projs = sorted(
            [p for p in self.projectiles if p.alive and not p.from_player],
            key=lambda p: (p.x - self.player.x) ** 2 + (p.y - self.player.y) ** 2
        )[:MAX_PROJECTILES]

        for i in range(MAX_PROJECTILES):
            if i < len(sorted_projs):
                p = sorted_projs[i]
                obs[idx] = _clamp((p.x - self.player.x) / 800.0, -1, 1); idx += 1
                obs[idx] = _clamp((p.y - self.player.y) / 800.0, -1, 1); idx += 1
                ndx2, ndy2 = _normalize_angle(p.vx, p.vy)
                obs[idx] = ndx2; idx += 1
                obs[idx] = ndy2; idx += 1
                obs[idx] = float(p.from_player); idx += 1
                proj_speed = math.sqrt(p.vx ** 2 + p.vy ** 2)
                if proj_speed > 1e-6 and not p.from_player:
                    pdx = self.player.x - p.x
                    pdy = self.player.y - p.y
                    dot = pdx * p.vx + pdy * p.vy
                    if dot > 0:
                        ttr = math.sqrt(pdx * pdx + pdy * pdy) / proj_speed
                        obs[idx] = _clamp(ttr / 60.0, 0, 1)
                    else:
                        obs[idx] = 1.0
                else:
                    obs[idx] = 1.0
                idx += 1
            else:
                idx += PROJECTILE_STATE_DIM

        sorted_drops = sorted(
            [d for d in self.drops if d.alive],
            key=lambda d: (d.x - self.player.x) ** 2 + (d.y - self.player.y) ** 2
        )[:MAX_DROPS]

        for i in range(MAX_DROPS):
            if i < len(sorted_drops):
                d = sorted_drops[i]
                obs[idx] = d.drop_type / 5.0; idx += 1
                obs[idx] = _clamp((d.x - self.player.x) / 800.0, -1, 1); idx += 1
                obs[idx] = _clamp((d.y - self.player.y) / 800.0, -1, 1); idx += 1
                dist = math.sqrt((d.x - self.player.x) ** 2 + (d.y - self.player.y) ** 2)
                obs[idx] = _clamp(dist / 800.0, 0, 1); idx += 1
            else:
                idx += DROP_STATE_DIM

        danger_dirs = [0.0] * DANGER_GRID_DIM
        for e in self.enemies:
            if not e.alive:
                continue
            edx = e.x - self.player.x
            edy = e.y - self.player.y
            edist = math.sqrt(edx * edx + edy * edy)
            if edist > 400 or edist < 1e-6:
                continue
            angle = math.atan2(edy, edx)
            sector = int((angle + math.pi) / (2 * math.pi / DANGER_GRID_DIM)) % DANGER_GRID_DIM
            weight = (1.0 - edist / 400.0) * (e.attack_damage / 30.0)
            danger_dirs[sector] += weight
        for i in range(DANGER_GRID_DIM):
            obs[idx] = _clamp(danger_dirs[i] / 5.0, 0, 1); idx += 1

        obs[idx] = _clamp(self.game_time / 600.0, 0, 1); idx += 1
        obs[idx] = _clamp(self._difficulty_scale() / 10.0, 0, 1); idx += 1

        return obs

    def _get_info(self):
        return {
            "health": self.player.health,
            "score": self.player.total_score,
            "kill_count": self.player.kill_count,
            "victory_stage": self.current_victory_stage,
            "game_time": self.game_time,
            "num_enemies": len([e for e in self.enemies if e.alive]),
            "step": self.step_count,
            "effective_difficulty": self._effective_difficulty,
        }

    def render(self):
        if self.render_mode == "ansi":
            info = self._get_info()
            print(
                f"HP:{info['health']} Score:{info['score']} "
                f"Kills:{info['kill_count']} Stage:{info['victory_stage']} "
                f"Time:{info['game_time']:.1f}s Enemies:{info['num_enemies']}"
            )

    def set_difficulty_level(self, level):
        self.difficulty_level = level

    def close(self):
        pass


gym.register(
    id="JRFirstGame-v0",
    entry_point="rl.game_env:JRFirstGameEnv",
)
