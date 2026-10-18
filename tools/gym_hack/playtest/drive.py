"""Drive gbarun from Python: read the player position, walk Pallet Town routes, detect menus.
Addresses come from pokefirered.map (gSaveBlock1Ptr, gMain, CB2_Overworld) and change when the ROM
changes; update them if the driver stops working."""
import subprocess, sys
sys.path.insert(0,str(__import__('pathlib').Path(__file__).resolve().parents[1]))
import os
# Work dir with gbarun (see build.sh); save states and screenshots go here too
S=os.environ.get('GBARUN_DIR', '/tmp/gbarun')
ROM=os.environ.get('ROM', str(__import__('pathlib').Path(__file__).resolve().parents[3] / 'pokefirered.gba'))
class GBA:
    def __init__(self, state=None):
        self.p=subprocess.Popen([S+'/gbarun',ROM],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
        if state: self.cmd(f'load {state}')
    def cmd(self,c):
        self.p.stdin.write(c+'\n'); self.p.stdin.flush()
    def pos(self):
        self.cmd('pos 0x030052d4 x'); l=self.p.stdout.readline().split()
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
        self.cmd('peek 0x030067d4 4'); l=self.p.stdout.readline().split()[1:]
        return int(''.join(reversed(l)), 16)
    def in_overworld(self):
        return self.cb2() & ~1 == 0x08197860
    def close(self):
        self.p.stdin.close(); self.p.wait()
