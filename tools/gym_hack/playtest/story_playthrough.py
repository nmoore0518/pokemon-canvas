# Full playthrough: new game -> Oak escort -> lab story -> type pick -> rival battle -> gym scene -> town
import subprocess, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from drive import *
if '--reuse' not in sys.argv:
    subprocess.run(f"{{ cat {HERE}/boot_to_pallet.txt; echo 'save {S}/st_pallet.ss'; }} | {S}/gbarun {ROM} > /dev/null", shell=True)
g=GBA(S+'/st_pallet.ss')
assert g.goto(20,2); g.cmd('hold UP 16')

STEP = {'UP': (0, -1), 'DOWN': (0, 1), 'LEFT': (-1, 0), 'RIGHT': (1, 0)}
def advance_until_moved(direction, start, limit=300, shots=None):
    """Press A to advance text until the player, standing at `start`, can step in `direction`."""
    for i in range(limit):
        before = g.pos()
        g.cmd(f'hold {direction} 16'); g.cmd('wait 8')
        after = g.pos()
        dx, dy = STEP[direction]
        if before[1:] == start and after[1:] == (start[0] + dx, start[1] + dy):
            return i
        if shots and i % shots[1] == 0: g.cmd(f'shot {S}/{shots[0]}{i:03d}.png')
        if not g.in_overworld():   # the nickname screen: accept the default name
            g.cmd('hold START 4'); g.cmd('wait 30'); g.cmd('hold A 4'); g.cmd('wait 90')
            continue
        g.cmd('hold A 4'); g.cmd('wait 40')
    g.cmd(f'shot {S}/fail.png'); g.cmd(f'save {S}/st_fail.ss'); raise SystemExit(f'never moved {direction}')

# Oak's escort, then the lab speech (texts saved as lt*.png)
for i in range(200):
    g.cmd('wait 40'); g.cmd('hold A 4')
    if g.pos()[0] == (38, 3): break
advance_until_moved('DOWN', (6, 4), shots=('lt', 1))
g.cmd('hold RIGHT 32'); g.cmd('wait 10'); g.cmd('hold UP 6'); g.cmd('wait 20')
print('at shelf', g.pos())
# type menu: Fairy, confirm, then decline the nickname
g.cmd('hold A 4'); g.cmd('wait 80'); g.cmd(f'shot {S}/menu.png'); g.cmd('hold A 4')
advance_until_moved('LEFT', (8, 5))
g.cmd('hold DOWN 48'); g.cmd('wait 30')
print('battle trigger', g.pos())
for i in range(600):
    g.cmd('wait 50'); g.cmd('hold A 4')
    if i % 6 == 0: g.cmd(f'shot {S}/fb{i:03d}.png')
    if g.pos()[0] != (38, 3): break
print('warped to gym', g.pos())
g.cmd(f'save {S}/st_fullgym.ss')
for i in range(80):
    g.cmd('wait 50'); g.cmd(f'shot {S}/fg{i:03d}.png'); g.cmd('hold A 4')
g.cmd('wait 200'); print('scene over', g.pos())
g.cmd('hold DOWN 16'); g.cmd('wait 30'); print(g.pos())
for seq in (['hold DOWN 20'], ['hold LEFT 16', 'hold DOWN 20'], ['hold RIGHT 32', 'hold DOWN 20']):
    for c in seq: g.cmd(c)
    g.cmd('wait 200')
    if g.pos()[0][0] == 37: break
print('outside', g.pos()); g.cmd(f'shot {S}/fout.png')
g.cmd('hold UP 32'); g.cmd('wait 200'); print('back in', g.pos()); g.cmd('wait 120'); g.cmd(f'shot {S}/fin.png')
g.close()
