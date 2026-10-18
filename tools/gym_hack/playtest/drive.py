"""Drive gbarun from Python: read the player position, walk Pallet Town routes, detect menus.
Addresses come from pokefirered.map (gSaveBlock1Ptr, gMain, CB2_Overworld) and change when the ROM
changes; update them if the driver stops working."""
import subprocess, sys
sys.path.insert(0,str(__import__('pathlib').Path(__file__).resolve().parents[1]))
import os
# Work dir with gbarun (see build.sh); save states and screenshots go here too
S=os.environ.get('GBARUN_DIR', '/tmp/gbarun')
ROM=os.environ.get('ROM', str(__import__('pathlib').Path(__file__).resolve().parents[3] / 'pokefirered.gba'))
def symbol(name, _cache={}):
    """Address of a symbol, read from pokefirered.map next to the ROM."""
    if not _cache:
        import re
        mp = __import__('pathlib').Path(ROM).with_suffix('.map')
        for m in re.finditer(r"^\s+0x([0-9a-f]+)\s+(\w+)\s*$", mp.read_text(), re.M):
            _cache[m[2]] = int(m[1], 16)
    return _cache[name]

class GBA:
    def __init__(self, state=None):
        self.p=subprocess.Popen([S+'/gbarun',ROM],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
        if state: self.cmd(f'load {state}')
    def cmd(self,c):
        self.p.stdin.write(c+'\n'); self.p.stdin.flush()
    def pos(self):
        self.cmd(f"pos {symbol('gSaveBlock1Ptr'):#x} x"); l=self.p.stdout.readline().split()
        return (int(l[3].split('.')[0]),int(l[3].split('.')[1])), int(l[5]), int(l[7])
    def goto(self, gx, gy, z=3, tries=8):
        import build_pallet_town as b
        for _ in range(tries):
            m,x,y=self.pos()
            if (x,y)==(gx,gy): return True
            cells=b.route((x,y),(gx,gy),b.elev[y][x] if b.elev[y][x] not in (0,15) else z)
            for a,c in zip(cells,cells[1:]):
                d={(0,-1):'UP',(0,1):'DOWN',(-1,0):'LEFT',(1,0):'RIGHT'}[(c[0]-a[0],c[1]-a[1])]
                self.cmd(f'hold {d} 16')
                m,x,y=self.pos()
                if (x,y)!=c:
                    self.cmd('wait 30'); break
        return self.pos()[1:]==(gx,gy)
    def cb2(self):
        self.cmd(f"peek {symbol('gMain') + 4:#x} 4"); l=self.p.stdout.readline().split()[1:]
        return int(''.join(reversed(l)), 16)
    def in_overworld(self):
        return self.cb2() & ~1 == symbol('CB2_Overworld')
    def peek32(self, addr):
        self.cmd(f'peek32 {addr:#x}'); return int(self.p.stdout.readline())
    def sb1(self): return self.peek32(symbol('gSaveBlock1Ptr'))
    def sb2(self): return self.peek32(symbol('gSaveBlock2Ptr'))
    # Offsets in the FireRed save (see tools/gym_hack/playtest/README): money, encryption key, secret bases
    MONEY, KEY, SECRET_BASES = 1168, 172, 6800
    def set_money(self, amount):
        key = self.peek32(self.sb2() + self.KEY)
        self.cmd(f'poke32 {self.sb1() + self.MONEY:#x} {amount ^ key}')
    def set_decor_owned(self, decor_id, count):
        self.cmd(f'poke8 {self.sb1() + self.SECRET_BASES + decor_id:#x} {count}')
    def close(self):
        self.p.stdin.close(); self.p.wait()
