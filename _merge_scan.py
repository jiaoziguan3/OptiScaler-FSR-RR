import re, os, sys
src_root = r'J:\OptiScaler-ffx-denoise-experimental'
dst_root = r'j:\OptiScaler-master-0.10-3'
manual = os.path.join(src_root, 'FSR-RR_Related_Files.txt')
with open(manual, 'r', encoding='utf-8') as f:
    text = f.read()
paths = []
for line in text.splitlines():
    m = re.match(r'\s*\d+\.\s+(\S+)', line)
    if m:
        p = m.group(1)
        if p.startswith('OptiScaler'):
            paths.append(p)
print(f'Found {len(paths)} paths')
results = []
for p in paths:
    sp = os.path.join(src_root, p)
    dp = os.path.join(dst_root, p)
    se = os.path.exists(sp)
    de = os.path.exists(dp)
    if se and not de:
        tag = 'NEW'
    elif not se and de:
        tag = 'MISSING_IN_SRC'
    elif se and de:
        with open(sp, 'rb') as f1, open(dp, 'rb') as f2:
            tag = 'SAME' if f1.read() == f2.read() else 'DIFF'
    else:
        tag = 'BOTH_MISSING'
    results.append((tag, p))
    print(f'{tag}\t{p}')
# Summary counts
from collections import Counter
c = Counter(t for t, _ in results)
print('\nSummary:', dict(c))
