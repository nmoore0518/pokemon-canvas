#!/usr/bin/env python3
"""Pokémon Canvas: the synthetic abilities as one review page (HTML), from
tools/gym_hack/synthetic_abilities.txt and build/synthetic_abilities.json
(run build_synthetic_abilities.py first).

    python3 tools/gym_hack/synthetic_abilities_page.py ability_descriptions.json out.html
"""
import html
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FORMS = {"ALOLA": "Alolan", "GALAR": "Galarian", "HISUI": "Hisuian", "PALDEA": "Paldean",
         "M": "Male", "F": "Female", "BLOODMOON": "Bloodmoon"}


def form_label(key, base):
    root = re.sub(r"[^A-Z0-9]", "_", base.upper())
    rest = key[8:]
    if rest.startswith(root):
        rest = rest[len(root):].strip("_")
    if not rest:
        return ""
    words = [FORMS.get(w, w.title()) for w in rest.split("_")]
    return " ".join(words)


def main():
    desc = json.loads(Path(sys.argv[1]).read_text())
    res = json.loads((ROOT / "build/synthetic_abilities.json").read_text())
    rows, gen = [], ""
    for line in (ROOT / "tools/gym_hack/synthetic_abilities.txt").read_text().splitlines():
        m = re.match(r"^# -+ Gen (\d)", line)
        if m:
            gen = m.group(1)
        if not line.strip() or line.startswith("#"):
            continue
        sp, ab, why = [p.strip() for p in (line.split(" : ", 2) + [""])[:3]]
        keys = ["SPECIES_" + s for s in sp.split()]
        mons = []
        for k in keys:
            r = res[k]
            fl = form_label(k, r["species"])
            fl = {"Normal": "", "Standard": "", "Land": "Land", "Plant": "", "West": "", "Average": "",
                  "Red": "", "Natural": "", "Icy Snow": "", "Red Striped": "Red-Striped",
                  "Solo": "", "Disguised": "", "Ordinary": "", "Aria": "Aria", "Phony": "", "Counterfeit": "",
                  "Unremarkable": "", "Two Segment": "", "Three": "", "Strawberry Vanilla Cream": "",
                  "Overcast": "", "Shield": "", "Meteor Red": "", "Ice": "" if r["species"] == "Eiscue" else "Ice Rider",
                  "Full Belly": "", "Zero": "", "Curly": "", "Confined": "Confined", "Neutral": "",
                  "Incarnate": "Incarnate", "Teal": "Teal Mask"}.get(fl, fl)
            mons.append(r["species"] + (f" ({fl})" if fl else ""))
        last = res[keys[-1]]
        tag = "flag" if why.startswith("flag") or "(flag" in why else "sidegrade" if "sidegrade" in why else ""
        rows.append({"gen": gen, "mons": mons, "dex": res[keys[0]]["dex"], "types": [t.title() for t in last["types"]],
                     "own": list(dict.fromkeys(last["own"])), "ab": res[keys[0]]["name"], "why": why, "tag": tag})
    abilities = sorted({r["ab"] for r in rows})
    missing = [a for a in abilities if a not in desc]
    if missing:
        raise SystemExit("no description: " + ", ".join(missing))
    page = TEMPLATE.replace("/*DATA*/", json.dumps({"rows": rows, "desc": desc}, ensure_ascii=False))
    Path(sys.argv[2]).write_text(page)
    print(len(rows), "rows,", sum(len(r["mons"]) for r in rows), "Pokémon,", len(abilities), "abilities")


TEMPLATE = r"""<title>Synthetic Abilities</title>
<meta name="claude:viewport-fit-sides" content="cover">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Chivo:wght@400;600;800&family=Chivo+Mono:wght@400;500&display=swap">
<style>
/* Layout: a lab ledger - a sticky filter bar over one long, scannable table; cards on phones */
:root {
  --bg: #f3f4f1; --panel: #ffffff; --ink: #1d2422; --muted: #5e6965; --rule: #d9ddd6;
  --accent: #1f8a5b; --accent-ink: #ffffff; --chip: #e6ebe6; --side: #9a6a12; --side-bg: #fbf1dc;
  --flag: #b23a3a; --flag-bg: #fbe5e3;
  --display: "Chivo", "Segoe UI", Arial, sans-serif; --body: "Chivo", "Segoe UI", Arial, sans-serif;
  --mono: "Chivo Mono", ui-monospace, "SFMono-Regular", Menlo, monospace;
}
@media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) {
  --bg: #121614; --panel: #1a201d; --ink: #e5ebe7; --muted: #9aa69f; --rule: #2c3531;
  --accent: #4cc48a; --accent-ink: #0e1512; --chip: #24302a; --side: #e3b45a; --side-bg: #2e2716;
  --flag: #f08a80; --flag-bg: #3a1f1d; color-scheme: dark } }
:root[data-theme="dark"] {
  --bg: #121614; --panel: #1a201d; --ink: #e5ebe7; --muted: #9aa69f; --rule: #2c3531;
  --accent: #4cc48a; --accent-ink: #0e1512; --chip: #24302a; --side: #e3b45a; --side-bg: #2e2716;
  --flag: #f08a80; --flag-bg: #3a1f1d; color-scheme: dark }
body { background: var(--bg); color: var(--ink); font-family: var(--body); font-size: 15px; line-height: 1.45; }
.wrap { max-width: 1240px; margin: 0 auto; padding-block: 28px 64px;
  padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); }
header h1 { font-family: var(--display); font-weight: 800; font-size: clamp(28px, 4vw, 40px); margin: 0; letter-spacing: -0.01em; text-wrap: balance; }
header p { color: var(--muted); max-width: 70ch; margin: 8px 0 0; }
.eyebrow { font-family: var(--mono); font-size: 12px; letter-spacing: 0.08em; text-transform: uppercase; color: var(--accent); }
.tally { display: flex; flex-wrap: wrap; gap: 8px 22px; margin: 18px 0 6px; font-family: var(--mono); font-size: 13px; color: var(--muted); }
.tally b { color: var(--ink); font-weight: 500; font-variant-numeric: tabular-nums; }
.legend { display: flex; flex-wrap: wrap; gap: 8px 18px; font-size: 13px; color: var(--muted); margin-top: 6px; }
.bar { position: sticky; top: env(safe-area-inset-top, 0px); z-index: 5; background: var(--bg); padding-block: 12px; border-bottom: 1px solid var(--rule);
  display: flex; flex-wrap: wrap; gap: 10px; align-items: center; margin-top: 18px; }
body::before { content: ""; position: fixed; inset: 0 0 auto; height: env(safe-area-inset-top, 0px); z-index: 10; background: var(--bg); }
.bar input, .bar select { font: inherit; font-size: 14px; padding: 8px 10px; border: 1px solid var(--rule); border-radius: 6px; background: var(--panel); color: var(--ink); min-width: 0; }
.bar input { flex: 1 1 220px; }
.bar select { flex: 0 1 220px; }
.seg { display: flex; flex-wrap: wrap; gap: 4px; }
.seg button { font: inherit; font-size: 13px; padding: 6px 10px; border: 1px solid var(--rule); background: var(--panel); color: var(--ink); border-radius: 6px; cursor: pointer; }
.seg button[aria-pressed="true"] { background: var(--accent); color: var(--accent-ink); border-color: var(--accent); }
button:focus-visible, input:focus-visible, select:focus-visible, summary:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.count { font-family: var(--mono); font-size: 13px; color: var(--muted); margin-left: auto; font-variant-numeric: tabular-nums; }
details.index { margin-top: 18px; background: var(--panel); border: 1px solid var(--rule); border-radius: 8px; padding: 10px 14px; }
details.index summary { cursor: pointer; font-weight: 600; }
.abl { columns: 3 280px; column-gap: 28px; margin-top: 10px; }
.abl div { break-inside: avoid; padding: 6px 0; border-bottom: 1px solid var(--rule); font-size: 13.5px; }
.abl button { font: inherit; font-weight: 600; background: none; border: 0; color: var(--accent); padding: 0; cursor: pointer; }
.abl .n { font-family: var(--mono); color: var(--muted); font-size: 12px; }
.tablewrap { overflow-x: auto; margin-top: 14px; }
table { width: 100%; border-collapse: collapse; background: var(--panel); border: 1px solid var(--rule); border-radius: 8px; }
th { text-align: left; font-family: var(--mono); font-weight: 500; font-size: 11.5px; letter-spacing: 0.06em; text-transform: uppercase; color: var(--muted);
  padding: 10px 12px; border-bottom: 1px solid var(--rule); white-space: nowrap; }
td { padding: 10px 12px; border-bottom: 1px solid var(--rule); vertical-align: top; }
tr.gen td { background: var(--chip); font-family: var(--mono); font-size: 12px; letter-spacing: 0.08em; text-transform: uppercase; color: var(--muted); padding: 6px 12px; }
.num { font-family: var(--mono); color: var(--muted); font-size: 12.5px; font-variant-numeric: tabular-nums; white-space: nowrap; }
.mons { font-weight: 600; min-width: 160px; }
.mons span { white-space: nowrap; }
.types { display: flex; flex-wrap: wrap; gap: 4px; }
.type { font-size: 11.5px; padding: 2px 7px; border-radius: 4px; color: #fff; white-space: nowrap; text-shadow: 0 1px 0 rgba(0,0,0,.25); }
.own { color: var(--muted); font-size: 13.5px; min-width: 130px; }
.ab { font-weight: 700; color: var(--accent); white-space: nowrap; }
.what { font-size: 13.5px; min-width: 220px; max-width: 34ch; }
.why { font-size: 13.5px; color: var(--muted); min-width: 220px; }
.pill { display: inline-block; font-family: var(--mono); font-size: 11px; padding: 1px 6px; border-radius: 4px; margin-right: 6px; vertical-align: 1px; }
.pill.side { background: var(--side-bg); color: var(--side); }
.pill.flag { background: var(--flag-bg); color: var(--flag); }
.empty { padding: 24px; color: var(--muted); text-align: center; }
@media (max-width: 760px) {
  table, tbody, tr, td { display: block; width: 100%; }
  thead { display: none; }
  .bar { position: static; }
  table { border: 0; background: none; }
  tr.row { background: var(--panel); border: 1px solid var(--rule); border-radius: 8px; margin-bottom: 10px; padding: 8px 0; }
  tr.gen td { border-radius: 6px; margin: 14px 0 8px; }
  td { border: 0; padding: 4px 14px; }
  .what, .why, .own, .mons { max-width: none; min-width: 0; }
  .own::before { content: "Now: "; }
}
@media (prefers-reduced-motion: no-preference) { .seg button { transition: background .15s, color .15s; } }
</style>

<div class="wrap">
  <header>
    <div class="eyebrow">Pokémon Canvas · Dad's Ability Synthesizer</div>
    <h1>Synthetic Abilities</h1>
    <p>The ability each Pokémon gets from the Ability Synthesizer. Every one is an existing ability, picked to be a little better than what the Pokémon has now without breaking it. Evolution lines share one unless a stage needs its own; forms not listed share their species' pick.</p>
    <div class="tally" id="tally"></div>
    <div class="legend">
      <span><span class="pill side">sidegrade</span>its own ability is already one of its best, so this is an alternative rather than an upgrade</span>
      <span><span class="pill flag">flag</span>its own ability drives a form change (Stance Change, Schooling...), so the Synthesizer won't take it: it keeps its own ability</span>
    </div>
  </header>

  <div class="bar" role="search">
    <input id="q" type="search" placeholder="Search a Pokémon, ability or type" aria-label="Search">
    <select id="ab" aria-label="Ability"><option value="">Every ability</option></select>
    <div class="seg" id="gens" aria-label="Generation"></div>
    <div class="seg" id="tags" aria-label="Show">
      <button data-tag="" aria-pressed="true">All</button>
      <button data-tag="sidegrade" aria-pressed="false">Sidegrades</button>
      <button data-tag="flag" aria-pressed="false">Flags</button>
    </div>
    <span class="count" id="count"></span>
  </div>

  <details class="index" id="index">
    <summary>The abilities used, with how many lines get each</summary>
    <div class="abl" id="abl"></div>
  </details>

  <div class="tablewrap">
    <table>
      <thead><tr><th>#</th><th>Pokémon</th><th>Type</th><th>Abilities now</th><th>Synthetic</th><th>What it does</th><th>Why this one</th></tr></thead>
      <tbody id="rows"></tbody>
    </table>
    <div class="empty" id="empty" hidden>Nothing matches. Clear the search or pick another filter.</div>
  </div>
</div>

<script>
const DATA = /*DATA*/;
const TYPE = {Normal:"#8f8e6c",Fire:"#d9692a",Water:"#4f7fe0",Electric:"#c9a414",Grass:"#5aa83a",Ice:"#4fa9a6",Fighting:"#b02a24",Poison:"#94368f",Ground:"#b8963f",Flying:"#8a73d6",Psychic:"#e04a78",Bug:"#8c9c18",Rock:"#9c8a30",Ghost:"#64498a",Dragon:"#5a2be0",Dark:"#5d4a3c",Steel:"#8a8aa8",Fairy:"#c06d98"};
const esc = s => s.replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const rows = DATA.rows, desc = DATA.desc;
const state = { q: "", ab: "", gen: "", tag: "" };
const abCount = {};
rows.forEach(r => abCount[r.ab] = (abCount[r.ab] || 0) + 1);
const abilities = Object.keys(abCount).sort();
const sel = document.getElementById("ab");
abilities.forEach(a => { const o = document.createElement("option"); o.value = a; o.textContent = `${a} (${abCount[a]})`; sel.appendChild(o); });
document.getElementById("abl").innerHTML = abilities.map(a =>
  `<div><button data-ab="${esc(a)}">${esc(a)}</button> <span class="n">× ${abCount[a]}</span><br>${esc(desc[a])}</div>`).join("");
const gens = [...new Set(rows.map(r => r.gen))];
document.getElementById("gens").innerHTML = `<button data-gen="" aria-pressed="true">All gens</button>` +
  gens.map(g => `<button data-gen="${g}" aria-pressed="false">Gen ${g}</button>`).join("");
const mons = rows.reduce((n, r) => n + r.mons.length, 0);
document.getElementById("tally").innerHTML =
  `<span><b>${mons}</b> Pokémon and forms</span><span><b>${rows.length}</b> lines</span><span><b>${abilities.length}</b> abilities</span>` +
  `<span><b>${rows.filter(r => r.tag === "sidegrade").length}</b> sidegrades</span><span><b>${rows.filter(r => r.tag === "flag").length}</b> flags</span>`;

function match(r) {
  if (state.ab && r.ab !== state.ab) return false;
  if (state.gen && r.gen !== state.gen) return false;
  if (state.tag && r.tag !== state.tag) return false;
  if (state.q) {
    const hay = (r.mons.join(" ") + " " + r.ab + " " + r.types.join(" ") + " " + r.own.join(" ")).toLowerCase();
    if (!state.q.split(/\s+/).every(w => hay.includes(w))) return false;
  }
  return true;
}
function render() {
  let html = "", gen = null, shown = 0;
  rows.forEach(r => {
    if (!match(r)) return;
    if (r.gen !== gen) { gen = r.gen; html += `<tr class="gen"><td colspan="7">Generation ${gen}</td></tr>`; }
    shown++;
    const why = r.why.replace(/\s*\((sidegrade|flag)[^)]*\)/, m => "").replace(/^flag\s*-\s*/, "");
    const note = (r.why.match(/\((sidegrade|flag)[: ]([^)]*)\)/) || [])[2];
    const pill = r.tag ? `<span class="pill ${r.tag === "flag" ? "flag" : "side"}">${r.tag}</span>` : "";
    html += `<tr class="row">
      <td class="num">#${String(r.dex).padStart(4, "0")}</td>
      <td class="mons">${r.mons.map(m => `<span>${esc(m)}</span>`).join(", ")}</td>
      <td><div class="types">${r.types.map(t => `<span class="type" style="background:${TYPE[t] || "#777"}">${t}</span>`).join("")}</div></td>
      <td class="own">${esc(r.own.join(", "))}</td>
      <td class="ab">${esc(r.ab)}</td>
      <td class="what">${esc(desc[r.ab])}</td>
      <td class="why">${pill}${esc(why)}${note ? ` <i>(${esc(note.trim())})</i>` : ""}</td>
    </tr>`;
  });
  document.getElementById("rows").innerHTML = html;
  document.getElementById("empty").hidden = shown > 0;
  document.getElementById("count").textContent = `${shown} of ${rows.length} lines`;
}
function press(groupId, attr, value) {
  document.querySelectorAll(`#${groupId} button`).forEach(b => b.setAttribute("aria-pressed", String(b.dataset[attr] === value)));
}
document.getElementById("q").addEventListener("input", e => { state.q = e.target.value.trim().toLowerCase(); render(); });
sel.addEventListener("change", e => { state.ab = e.target.value; render(); });
document.getElementById("gens").addEventListener("click", e => {
  const b = e.target.closest("button"); if (!b) return; state.gen = b.dataset.gen; press("gens", "gen", state.gen); render(); });
document.getElementById("tags").addEventListener("click", e => {
  const b = e.target.closest("button"); if (!b) return; state.tag = b.dataset.tag; press("tags", "tag", state.tag); render(); });
document.getElementById("abl").addEventListener("click", e => {
  const b = e.target.closest("button"); if (!b) return; state.ab = b.dataset.ab; sel.value = state.ab; render();
  document.querySelector(".tablewrap").scrollIntoView({ behavior: "smooth", block: "start" }); });
render();
</script>
"""

if __name__ == "__main__":
    main()
