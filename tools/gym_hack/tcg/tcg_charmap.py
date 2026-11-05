"""Pokémon Canvas: the Trading Card Game's text encoding, as rgbasm builds pret/poketcg's text: its
charmaps (constants/charmaps.asm) and text macros (macros/text.asm), so the duel's strings are the
game's own bytes (TX_HALFWIDTH, TX_FULLWIDTH*, TX_SYMBOL, TX_RAM*...), for the game's text engine.

    cm = Charmaps(src)          # src: <poketcg>/src
    cm.encode_text_block(lines) # the lines of one text label (text / line / textfw / ... / done)
"""
import re

TX = {"TX_FULLWIDTH0": 0, "TX_FULLWIDTH1": 1, "TX_FULLWIDTH2": 2, "TX_FULLWIDTH3": 3, "TX_FULLWIDTH4": 4,
      "TX_SYMBOL": 5, "TX_HALFWIDTH": 6, "TX_HALF2FULL": 7, "TX_RAM1": 9, "TX_LINE": 0x0a, "TX_RAM2": 0x0b,
      "TX_RAM3": 0x0c, "TX_HIRAGANA": 0x0e, "TX_KATAKANA": 0x0f, "TX_END": 0}


def _num(s):
    s = s.strip()
    if s in TX:
        return TX[s]
    if s.startswith("$"):
        return int(s[1:], 16)
    if s.startswith("%"):
        return int(s[1:], 2)
    return int(s)


def _unescape(s):
    return s.replace("\\n", "\n").replace('\\"', '"').replace("\\\\", "\\").replace("\\{", "{")


class Charmaps:
    def __init__(self, src):
        self.maps = {"main": {}, "katakana": {}, "hiragana": {}, "fullwidth": {}}
        cur = "main"
        stack = []
        const_value = 0
        lines = (src / "constants/charmaps.asm").read_text().splitlines()
        in_macro = False
        for line in lines:
            s = line.split(";")[0].strip()
            if not s:
                continue
            if s.startswith("MACRO"):
                in_macro = True
                continue
            if s.startswith("ENDM"):
                in_macro = False
                continue
            if in_macro:
                continue
            m = re.match(r"NEWCHARMAP (\w+)", s)
            if m:
                cur = m[1]
                continue
            m = re.match(r"SETCHARMAP (\w+)", s)
            if m:
                cur = m[1]
                continue
            if s.startswith("const_def"):
                const_value = 0
                continue
            m = re.match(r'charmap "((?:[^"\\]|\\.)*)",\s*(.*)', s)
            if m:
                self.maps[cur][_unescape(m[1])] = [_num(v) for v in m[2].split(",")]
                continue
            m = re.match(r'fwcharmap (\w+),\s*"((?:[^"\\]|\\.)*)",\s*(\S+)', s)
            if m:
                kind, ch, val = m[1], _unescape(m[2]), _num(m[3])
                if kind in ("TX_KATAKANA", "TX_HIRAGANA"):
                    self.maps[cur][ch] = [val]
                    self.maps["katakana" if kind == "TX_KATAKANA" else "hiragana"][ch] = [val]
                elif kind == "TX_FULLWIDTH0":
                    self.maps[cur][ch] = [val]
                else:
                    self.maps[cur][ch] = [TX[kind], val]
                continue
            m = re.match(r'txsymbol "(\w+)"', s)
            if m:
                key = "<" + m[1] + ">"
                self.maps[cur][key] = [TX["TX_SYMBOL"], const_value]
                self.maps["main"][key] = [TX["TX_SYMBOL"], const_value]
                const_value += 1
                continue

    def chars(self, mapname, string):
        """rgbasm's charmap matching: the longest mapping at each point; unmapped: the UTF-8 bytes"""
        cm = self.maps[mapname]
        longest = max(len(k) for k in cm)
        i, out = 0, []
        while i < len(string):
            for n in range(min(longest, len(string) - i), 0, -1):
                if string[i:i + n] in cm:
                    out.append((string[i:i + n], cm[string[i:i + n]]))
                    i += n
                    break
            else:
                out.append((string[i], list(string[i].encode())))
                i += 1
        return out

    def _strings(self, args):
        return [_unescape(a) for a in re.findall(r'"((?:[^"\\]|\\.)*)"', args)]

    def _fw(self, args, cur_set):
        out = []
        for s in self._strings(args):
            for ch, val in self.chars("fullwidth", s):
                charset = TX["TX_KATAKANA"] if ch in self.maps["katakana"] else \
                    TX["TX_HIRAGANA"] if ch in self.maps["hiragana"] else 0
                if charset and charset != cur_set:
                    cur_set = charset
                    out.append(charset)
                out += val
        return out, cur_set

    def encode_text_block(self, lines):
        """the bytes of one label's text macros, through `done` (TX_END included)"""
        out, cur_set = [], TX["TX_KATAKANA"]
        for line in lines:
            s = line.split(";")[0].strip() if not re.search(r'"[^"]*;[^"]*"', line) else line.strip()
            m = re.match(r"(\w+)\s*(.*)", s)
            if not m:
                continue
            mac, args = m[1], m[2]
            if mac == "text":
                out.append(TX["TX_HALFWIDTH"])
                for st in self._strings(args):
                    out += [b for _, v in self.chars("main", st) for b in v]
            elif mac == "line":
                out.append(TX["TX_LINE"])
                for st in self._strings(args):
                    out += [b for _, v in self.chars("main", st) for b in v]
            elif mac == "db":
                for st in self._strings(args):
                    out += [b for _, v in self.chars("main", st) for b in v]
            elif mac == "textfw":
                cur_set = TX["TX_KATAKANA"]
                b, cur_set = self._fw(args, cur_set)
                out += b
            elif mac == "linefw":
                out.append(TX["TX_LINE"])
                b, cur_set = self._fw(args, cur_set)
                out += b
            elif mac in ("katakana", "hiragana"):
                cur_set = TX["TX_KATAKANA"] if mac == "katakana" else TX["TX_HIRAGANA"]
                out.append(cur_set)
                b, cur_set = self._fw(args, cur_set)
                out += b
            elif mac == "half2full":
                out.append(TX["TX_HALF2FULL"])
            elif mac == "done":
                out.append(TX["TX_END"])
                return out
        return out


def text_blocks(src):
    """{label: [lines]} for every text label in src/text/*.asm"""
    t = {}
    for f in sorted((src / "text").glob("text*.asm")):
        cur, buf = None, []
        for line in f.read_text().splitlines():
            m = re.match(r"^(\w+):", line)
            if m:
                cur, buf = m[1], []
                continue
            if cur is None:
                continue
            buf.append(line)
            if line.strip().startswith("done"):
                t[cur] = buf
                cur = None
    return t


def c_bytes(b):
    """a C string literal of the bytes (the TX_END at the end left to the literal's own 0)"""
    if b and b[-1] == 0:
        b = b[:-1]
    out, prev_hex = "", False
    for x in b:
        c = chr(x)
        if 0x20 <= x < 0x7f and c not in '"\\?':
            if prev_hex and c in "0123456789abcdefABCDEF":
                out += '" "'   # (so the hex escape before it ends)
            out += c
            prev_hex = False
        else:
            out += "\\x%02x" % x
            prev_hex = True
    return '"' + out + '"'
