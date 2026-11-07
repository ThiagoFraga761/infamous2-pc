#!/usr/bin/env python3
"""Put each slow frame of a play log down to one cause, from its [vk hitch] line.

    work/tools/hitch_classify.py work/runs/play-<time>.log [min_ms] [-v]

For every [vk hitch] frame at least min_ms long (default 50): the class whose
share of the gap is largest among the ones below, then a count and the total
time per class, and with -v each frame's line. The thresholds are rough; a
frame can have two causes, and the line itself is the evidence.

  pipeline builds      shader/pipeline builds took most of the gap
  gpu flush            vk_flush waited for the GPU (a ring filled, a readback)
  bq_sync <call>       the walker waited for the recording thread to drain,
                       named by the call that waited longest (sync_top)
  textures             texture creation and uploads (engine and backend)
  main blocked <sc>    the main PPU thread sat in an lv2 call (block_top)
  main preempted       runnable but not running (runq): the host was busy
  page faults          major faults (disk or zram), or many minor ones
  main polling         the main thread spent the gap in its usleep polls:
                       waiting on an SPU job or on the RSX (see spurs, rsx cpu)
  main busy            the main thread ran the whole gap, the walker idle
  walker busy          the RSX walker drained the whole gap (draw work)
  main off-CPU, cause not logged
                       the walker idle and the main thread mostly off the CPU,
                       with no runq, faults or block_top to say why (logs
                       from before these were added)
  other                none of the above
"""
import re
import sys
from collections import Counter, defaultdict

HEAD = re.compile(r'\[vk hitch\] ([0-9.]+) ms frame ending at \d+ ns: (\d+) shader/pipeline builds ([0-9.]+) ms,'
                  r' (\d+) flushes ([0-9.]+) ms')


def section(line, name):
    m = re.search(r'\| ' + name + r'((?: [a-z_0-9]+=[0-9./]+)+)', line)
    return dict(re.findall(r' ([a-z_0-9]+)=([0-9./]+)', m.group(1))) if m else {}


def classify(line):
    m = HEAD.search(line)
    if not m:
        return None
    gap, builds_ms, flush_ms = float(m.group(1)), float(m.group(3)), float(m.group(5))
    buckets = {k: float(v) for k, v in re.findall(r' ([a-z_]+)=([0-9.]+)/\d+', line.split(' | cpu')[0])}
    cpu = {k: float(v) for k, v in section(line, 'cpu').items()}
    runq = {k: float(v) for k, v in section(line, 'runq').items()}
    faults = {k: tuple(int(x) for x in v.split('/')) for k, v in section(line, 'faults').items()}
    top = re.search(r'sync_top=([a-z_]+):([0-9.]+)', line)
    blk = re.search(r'block_top=(sc\d+)@([0-9A-F]+):([0-9.]+)', line)
    tex = sum(buckets.get(k, 0) for k in ('tex_create', 'tex_upload', 'eng_texup', 'eng_texmiss', 'vk_alloc'))
    majflt = sum(f[1] for f in faults.values())
    minflt = sum(f[0] for f in faults.values())
    cand = [
        (builds_ms, 'pipeline builds'),
        (flush_ms, 'gpu flush'),
        (buckets.get('bq_sync', 0), 'bq_sync ' + (top.group(1) if top else '?')),
        (tex, 'textures'),
        (float(blk.group(3)) if blk else 0, 'main blocked ' + (blk.group(1) + '@' + blk.group(2) if blk else '')),
        (runq.get('main', 0) + runq.get('rsx', 0), 'main preempted'),
        # Faults carry no time: they win only where nothing timed explains
        # the gap.
        (0.45 * gap if majflt or minflt > 20000 else 0, 'page faults'),
    ]
    share, cls = max(cand)
    if share >= 0.4 * gap:
        return gap, cls
    if buckets.get('ppu_usleep', 0) + buckets.get('ppu_refpoll', 0) >= 0.5 * gap:
        return gap, 'main polling'
    if cpu.get('main', 0) >= 0.8 * gap and buckets.get('rsx_idle', 0) >= 0.5 * gap:
        return gap, 'main busy'
    if buckets.get('rsx_drain', 0) >= 0.8 * gap:
        return gap, 'walker busy'
    if buckets.get('rsx_idle', 0) >= 0.5 * gap and cpu.get('main', gap) < 0.5 * gap:
        return gap, 'main off-CPU, cause not logged'
    return gap, 'other'


def main():
    args = [a for a in sys.argv[1:] if a != '-v']
    verbose = '-v' in sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    floor = float(args[1]) if len(args) > 1 else 50.0
    n, ms, ex = Counter(), Counter(), defaultdict(list)
    with open(args[0], errors='replace') as f:
        for line in f:
            r = classify(line)
            if not r or r[0] < floor:
                continue
            gap, cls = r
            n[cls] += 1
            ms[cls] += gap
            ex[cls].append(line.strip())
    if not n:
        print(f'no [vk hitch] frames of {floor:g} ms or more')
        return 0
    for cls, k in n.most_common():
        print(f'{k:5d} frames {ms[cls]:9.0f} ms  {cls}')
        if verbose:
            for l in ex[cls][:8]:
                print('      ', l[:600])
    return 0


if __name__ == '__main__':
    sys.exit(main())
