"""
JRFirstGame 音频生成脚本
使用Python + numpy + scipy 程序化生成所有游戏音效和BGM
输出格式: WAV (16-bit PCM, 44100Hz, 立体声/单声道)
"""
import numpy as np
from scipy.io import wavfile
import os, struct, math

SAMPLE_RATE = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'assets', 'sounds')

def ensure_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

def save_wav(filename, samples, stereo=False):
    path = os.path.join(OUTPUT_DIR, filename)
    data = np.clip(samples, -1.0, 1.0)
    pcm = (data * 32767).astype(np.int16)
    if stereo and pcm.ndim == 1:
        pcm = np.column_stack([pcm, pcm])
    wavfile.write(path, SAMPLE_RATE, pcm)
    print(f"  Generated: {filename} ({len(pcm)/SAMPLE_RATE:.2f}s)")

def generate_tone(freq, duration, volume=0.5, wave_type='sine', fade_out=True):
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), endpoint=False)
    if wave_type == 'sine':
        samples = np.sin(2 * np.pi * freq * t)
    elif wave_type == 'square':
        samples = np.sign(np.sin(2 * np.pi * freq * t))
    elif wave_type == 'sawtooth':
        samples = 2 * (freq * t - np.floor(freq * t + 0.5))
    elif wave_type == 'triangle':
        samples = 2 * np.abs(2 * (freq * t - np.floor(freq * t + 0.5))) - 1
    else:
        samples = np.sin(2 * np.pi * freq * t)
    if fade_out:
        fade = np.linspace(1.0, 0.0, len(samples))
        samples *= fade
    return samples * volume

def generate_noise(duration, volume=0.3, fade_out=True):
    n = int(SAMPLE_RATE * duration)
    samples = np.random.uniform(-1, 1, n)
    if fade_out:
        fade = np.linspace(1.0, 0.0, n)
        samples *= fade
    return samples * volume

def envelope_adsr(samples, attack=0.01, decay=0.05, sustain_level=0.7, release=0.1):
    n = len(samples)
    a_n = int(attack * SAMPLE_RATE)
    d_n = int(decay * SAMPLE_RATE)
    r_n = int(release * SAMPLE_RATE)
    s_n = n - a_n - d_n - r_n
    if s_n < 0:
        s_n = 0
        r_n = n - a_n - d_n
    env = np.ones(n)
    if a_n > 0:
        env[:a_n] = np.linspace(0, 1, a_n)
    if d_n > 0:
        env[a_n:a_n+d_n] = np.linspace(1, sustain_level, d_n)
    if s_n > 0:
        env[a_n+d_n:a_n+d_n+s_n] = sustain_level
    if r_n > 0:
        env[a_n+d_n+s_n:] = np.linspace(sustain_level, 0, r_n)
    return samples * env

# ==================== BGM ====================

def generate_bgm_main_menu():
    """主菜单BGM - 宁静、奇幻风格"""
    print("Generating BGM: main_menu_bgm...")
    duration = 32.0
    n = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, n, endpoint=False)
    samples = np.zeros(n)
    
    # 低音pad - C大调和弦进行 C-Am-F-G
    chord_progressions = [
        (261.63, 329.63, 392.00),  # C
        (220.00, 261.63, 329.63),  # Am
        (349.23, 440.00, 523.25),  # F
        (392.00, 493.88, 587.33),  # G
    ]
    chord_dur = duration / 4
    
    for i, (f1, f2, f3) in enumerate(chord_progressions):
        start = int(i * chord_dur * SAMPLE_RATE)
        end = int((i + 1) * chord_dur * SAMPLE_RATE)
        seg_t = t[start:end] - t[start]
        seg = np.zeros(end - start)
        seg += 0.12 * np.sin(2 * np.pi * f1 * seg_t)
        seg += 0.10 * np.sin(2 * np.pi * f2 * seg_t)
        seg += 0.10 * np.sin(2 * np.pi * f3 * seg_t)
        seg += 0.06 * np.sin(2 * np.pi * f1 * 0.5 * seg_t)  # 低八度
        samples[start:end] += seg
    
    # 旋律线 - 简单的八音盒风格
    melody_notes = [523.25, 587.33, 659.25, 523.25, 659.25, 587.33, 523.25, 493.88,
                    523.25, 659.25, 783.99, 659.25, 587.33, 523.25, 493.88, 523.25,
                    659.25, 587.33, 523.25, 392.00, 440.00, 493.88, 523.25, 587.33,
                    523.25, 493.88, 440.00, 392.00, 349.23, 392.00, 440.00, 523.25]
    note_dur = duration / len(melody_notes)
    for i, freq in enumerate(melody_notes):
        start = int(i * note_dur * SAMPLE_RATE)
        end = int((i + 1) * note_dur * SAMPLE_RATE)
        seg_t = t[start:end] - t[start]
        seg = 0.15 * np.sin(2 * np.pi * freq * seg_t)
        fade = np.linspace(1.0, 0.0, end - start)
        seg *= fade
        samples[start:end] += seg
    
    # 整体淡入淡出
    fade_in = np.minimum(t / 2.0, 1.0)
    fade_out = np.minimum((duration - t) / 2.0, 1.0)
    samples *= fade_in * fade_out * 0.6
    
    save_wav("main_menu_bgm.wav", samples)

def generate_bgm_game():
    """游戏内BGM - 战斗感、节奏感"""
    print("Generating BGM: game_bgm...")
    duration = 40.0
    n = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, n, endpoint=False)
    samples = np.zeros(n)
    
    bpm = 120
    beat = 60.0 / bpm
    bar = beat * 4
    
    # 鼓点节奏
    for i in range(int(duration / beat)):
        bt = i * beat
        # kick on 1, 3
        if i % 4 == 0 or i % 4 == 2:
            start = int(bt * SAMPLE_RATE)
            end = min(start + int(0.15 * SAMPLE_RATE), n)
            seg_t = t[start:end] - t[start]
            kick = 0.3 * np.sin(2 * np.pi * 60 * seg_t * np.exp(-8 * seg_t))
            samples[start:end] += kick
        # snare on 2, 4
        if i % 4 == 1 or i % 4 == 3:
            start = int(bt * SAMPLE_RATE)
            end = min(start + int(0.1 * SAMPLE_RATE), n)
            noise = np.random.uniform(-1, 1, end - start) * 0.15
            fade = np.linspace(1, 0, end - start)
            samples[start:end] += noise * fade
    
    # 低音bass线 - 持续的八分音符
    bass_pattern = [130.81, 130.81, 146.83, 146.83, 110.00, 110.00, 130.81, 130.81]
    note_dur = beat * 0.5
    for i in range(int(duration / note_dur)):
        freq = bass_pattern[i % len(bass_pattern)]
        start = int(i * note_dur * SAMPLE_RATE)
        end = min(start + int(note_dur * 0.8 * SAMPLE_RATE), n)
        seg_t = t[start:end] - t[start]
        seg = 0.15 * np.sin(2 * np.pi * freq * seg_t)
        fade = np.linspace(1, 0.3, end - start)
        samples[start:end] += seg * fade
    
    # 和弦pad
    chords = [(261.63, 329.63, 392.00), (220.00, 261.63, 329.63)]
    for i, (f1, f2, f3) in enumerate(chords):
        start = int(i * bar * 2 * SAMPLE_RATE)
        end = min(int((i + 1) * bar * 2 * SAMPLE_RATE), n)
        seg_t = t[start:end] - t[start]
        seg = 0.06 * np.sin(2 * np.pi * f1 * seg_t)
        seg += 0.05 * np.sin(2 * np.pi * f2 * seg_t)
        seg += 0.05 * np.sin(2 * np.pi * f3 * seg_t)
        samples[start:end] += seg
    
    # 旋律
    melody = [523.25, 587.33, 659.25, 587.33, 523.25, 493.88, 523.25, 659.25,
              587.33, 523.25, 440.00, 493.88, 523.25, 587.33, 659.25, 783.99]
    m_note_dur = beat
    for i, freq in enumerate(melody):
        bt = i * m_note_dur
        while bt >= duration:
            bt -= duration
        start = int(bt * SAMPLE_RATE)
        end = min(start + int(m_note_dur * 0.7 * SAMPLE_RATE), n)
        seg_t = t[start:end] - t[start]
        seg = 0.12 * np.sin(2 * np.pi * freq * seg_t)
        fade = np.linspace(1, 0, end - start)
        samples[start:end] += seg * fade
    
    # 循环拼接
    fade_in = np.minimum(t / 1.0, 1.0)
    fade_out = np.minimum((duration - t) / 1.0, 1.0)
    samples *= fade_in * fade_out * 0.7
    
    save_wav("game_bgm.wav", samples)

# ==================== 音效 ====================

def gen_sfx_player_attack():
    """玩家攻击 - 剑挥砍声"""
    print("Generating SFX: player_attack...")
    dur = 0.25
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 噪声模拟挥剑气流
    noise = np.random.uniform(-1, 1, len(t)) * 0.4
    env = np.exp(-t * 15)
    noise *= env
    # 加一点金属感高频
    metal = 0.2 * np.sin(2 * np.pi * 1200 * t) * np.exp(-t * 20)
    metal += 0.15 * np.sin(2 * np.pi * 2400 * t) * np.exp(-t * 25)
    samples = (noise + metal) * 0.6
    save_wav("player_attack.wav", samples)

def gen_sfx_player_crit():
    """暴击音效 - 更尖锐、更响"""
    print("Generating SFX: player_crit...")
    dur = 0.35
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 强烈的金属撞击
    hit = 0.4 * np.sin(2 * np.pi * 800 * t) * np.exp(-t * 12)
    hit += 0.3 * np.sin(2 * np.pi * 1600 * t) * np.exp(-t * 15)
    hit += 0.2 * np.sin(2 * np.pi * 3200 * t) * np.exp(-t * 20)
    # 噪声冲击
    noise = np.random.uniform(-1, 1, len(t)) * 0.3 * np.exp(-t * 10)
    samples = envelope_adsr(hit + noise, attack=0.005, decay=0.05, sustain_level=0.5, release=0.15)
    save_wav("player_crit.wav", samples)

def gen_sfx_player_hurt():
    """玩家受伤"""
    print("Generating SFX: player_hurt...")
    dur = 0.3
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 低频冲击 + 短噪声
    impact = 0.4 * np.sin(2 * np.pi * 150 * t) * np.exp(-t * 10)
    noise = np.random.uniform(-1, 1, len(t)) * 0.2 * np.exp(-t * 15)
    samples = (impact + noise) * 0.7
    save_wav("player_hurt.wav", samples)

def gen_sfx_player_roll():
    """翻滚闪避"""
    print("Generating SFX: player_roll...")
    dur = 0.3
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 风声whoosh
    freq_sweep = np.linspace(400, 100, len(t))
    phase = np.cumsum(freq_sweep) / SAMPLE_RATE
    whoosh = 0.3 * np.sin(2 * np.pi * phase) * np.sin(np.pi * t / dur)
    save_wav("player_roll.wav", whoosh)

def gen_sfx_player_death():
    """玩家死亡"""
    print("Generating SFX: player_death...")
    dur = 1.0
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 下行音调
    freq = 400 * np.exp(-t * 2)
    phase = np.cumsum(freq) / SAMPLE_RATE
    tone = 0.3 * np.sin(2 * np.pi * phase)
    fade = np.linspace(1, 0, len(t))
    samples = tone * fade
    save_wav("player_death.wav", samples)

def gen_sfx_enemy_hurt():
    """敌人受伤"""
    print("Generating SFX: enemy_hurt...")
    dur = 0.15
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    impact = 0.25 * np.sin(2 * np.pi * 300 * t) * np.exp(-t * 20)
    noise = np.random.uniform(-1, 1, len(t)) * 0.15 * np.exp(-t * 25)
    save_wav("enemy_hurt.wav", impact + noise)

def gen_sfx_enemy_death():
    """敌人死亡"""
    print("Generating SFX: enemy_death...")
    dur = 0.4
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 爆裂声
    noise = np.random.uniform(-1, 1, len(t)) * 0.3 * np.exp(-t * 8)
    tone = 0.2 * np.sin(2 * np.pi * 200 * t) * np.exp(-t * 6)
    samples = (noise + tone) * 0.8
    save_wav("enemy_death.wav", samples)

def gen_sfx_enemy_shoot():
    """敌人发射投射物"""
    print("Generating SFX: enemy_shoot...")
    dur = 0.2
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    freq_sweep = np.linspace(600, 300, len(t))
    phase = np.cumsum(freq_sweep) / SAMPLE_RATE
    tone = 0.2 * np.sin(2 * np.pi * phase) * np.exp(-t * 10)
    save_wav("enemy_shoot.wav", tone)

def gen_sfx_explosion():
    """法师火球爆炸"""
    print("Generating SFX: explosion...")
    dur = 0.5
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 低频轰鸣 + 噪声
    boom = 0.4 * np.sin(2 * np.pi * 80 * t) * np.exp(-t * 5)
    noise = np.random.uniform(-1, 1, len(t)) * 0.3 * np.exp(-t * 8)
    crackle = np.random.uniform(-1, 1, len(t)) * 0.1 * np.sin(2 * np.pi * 1000 * t) * np.exp(-t * 10)
    samples = (boom + noise + crackle) * 0.7
    save_wav("explosion.wav", samples)

def gen_sfx_pickup():
    """拾取掉落物"""
    print("Generating SFX: pickup...")
    dur = 0.3
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 上行叮咚声
    tone1 = 0.3 * np.sin(2 * np.pi * 523.25 * t) * np.exp(-t * 8)
    tone2 = 0.25 * np.sin(2 * np.pi * 659.25 * t)
    tone2_shifted = np.zeros_like(t)
    shift = int(0.08 * SAMPLE_RATE)
    tone2_shifted[shift:] = tone2[:len(t)-shift] * np.exp(-(t[shift:] - 0.08) * 8)
    samples = (tone1 + tone2_shifted) * 0.7
    save_wav("pickup.wav", samples)

def gen_sfx_level_up():
    """升级/阶段胜利"""
    print("Generating SFX: level_up...")
    dur = 0.8
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    # 上行琶音 C-E-G-C
    notes = [523.25, 659.25, 783.99, 1046.50]
    samples = np.zeros_like(t)
    for i, freq in enumerate(notes):
        start = int(i * 0.12 * SAMPLE_RATE)
        end = min(start + int(0.4 * SAMPLE_RATE), len(t))
        seg_t = t[start:end] - t[start]
        seg = 0.25 * np.sin(2 * np.pi * freq * seg_t) * np.exp(-seg_t * 4)
        samples[start:end] += seg
    save_wav("level_up.wav", samples)

def gen_sfx_button_click():
    """UI按钮点击"""
    print("Generating SFX: button_click...")
    dur = 0.08
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    samples = 0.3 * np.sin(2 * np.pi * 800 * t) * np.exp(-t * 30)
    save_wav("button_click.wav", samples)

def gen_sfx_connect():
    """联机连接成功"""
    print("Generating SFX: connect...")
    dur = 0.5
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    notes = [440.00, 554.37, 659.25]
    samples = np.zeros_like(t)
    for i, freq in enumerate(notes):
        start = int(i * 0.1 * SAMPLE_RATE)
        end = min(start + int(0.3 * SAMPLE_RATE), len(t))
        seg_t = t[start:end] - t[start]
        seg = 0.2 * np.sin(2 * np.pi * freq * seg_t) * np.exp(-seg_t * 5)
        samples[start:end] += seg
    save_wav("connect.wav", samples)

def gen_sfx_disconnect():
    """断开连接"""
    print("Generating SFX: disconnect...")
    dur = 0.4
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    freq_sweep = np.linspace(600, 200, len(t))
    phase = np.cumsum(freq_sweep) / SAMPLE_RATE
    tone = 0.25 * np.sin(2 * np.pi * phase) * np.exp(-t * 5)
    save_wav("disconnect.wav", tone)

def gen_sfx_victory():
    """游戏胜利"""
    print("Generating SFX: victory...")
    dur = 2.0
    t = np.linspace(0, dur, int(SAMPLE_RATE * dur), endpoint=False)
    notes = [523.25, 659.25, 783.99, 1046.50, 783.99, 1046.50, 1318.51, 1046.50]
    samples = np.zeros_like(t)
    note_dur = dur / len(notes)
    for i, freq in enumerate(notes):
        start = int(i * note_dur * SAMPLE_RATE)
        end = min(start + int(note_dur * SAMPLE_RATE), len(t))
        seg_t = t[start:end] - t[start]
        seg = 0.2 * np.sin(2 * np.pi * freq * seg_t)
        fade = np.exp(-seg_t * 2)
        samples[start:end] += seg * fade
    fade_out = np.minimum((dur - t) / 0.5, 1.0)
    samples *= fade_out
    save_wav("victory.wav", samples)

# ==================== 主函数 ====================

def main():
    print("=" * 50)
    print("JRFirstGame Audio Generator")
    print("=" * 50)
    ensure_dir()
    
    print("\n--- Generating BGM ---")
    generate_bgm_main_menu()
    generate_bgm_game()
    
    print("\n--- Generating SFX ---")
    gen_sfx_player_attack()
    gen_sfx_player_crit()
    gen_sfx_player_hurt()
    gen_sfx_player_roll()
    gen_sfx_player_death()
    gen_sfx_enemy_hurt()
    gen_sfx_enemy_death()
    gen_sfx_enemy_shoot()
    gen_sfx_explosion()
    gen_sfx_pickup()
    gen_sfx_level_up()
    gen_sfx_button_click()
    gen_sfx_connect()
    gen_sfx_disconnect()
    gen_sfx_victory()
    
    print("\n" + "=" * 50)
    print(f"All audio files generated in: {os.path.abspath(OUTPUT_DIR)}")
    print("=" * 50)

if __name__ == "__main__":
    main()