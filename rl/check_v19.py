import re, os
from collections import Counter

base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
with open(os.path.join(base, 'rl', 'logs', 'ppo_v19_train.log')) as f:
    log = f.read()

scores = [int(m.group(1)) for m in re.finditer(r'Score:\s*(\d+)', log)]
stages = [m.group(1) for m in re.finditer(r'Stage:(\d+)', log)]

if scores:
    print(f'Scores: N={len(scores)}, Mean={sum(scores)/len(scores):.0f}, Max={max(scores)}')
    print(f'>=1k: {sum(1 for s in scores if s>=1000)} ({100*sum(1 for s in scores if s>=1000)/len(scores):.1f}%)')
    print(f'>=2k: {sum(1 for s in scores if s>=2000)} ({100*sum(1 for s in scores if s>=2000)/len(scores):.1f}%)')
    print('Stages:', dict(Counter(stages)))
