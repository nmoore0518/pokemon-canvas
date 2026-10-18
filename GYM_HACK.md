# Gym Trainer hack

A FireRed hack built on pokeemerald-expansion (FireRed mode). The player is training to become a
Gym Leader: they pick one type at the start and build their team, and later their own gym, around it.

## Getting the game

Every push to `main` builds the ROM automatically on GitHub:
**Actions** tab → latest **Build ROM** run → download **gym-trainer-hack-rom** under *Artifacts*
(it arrives as a .zip with `pokefirered.gba` inside). A build takes roughly 10-15 minutes.

Debug menu: hold **R** and press **Start** in the overworld.

## Editing it yourself

You can make small changes right on github.com: open a file, click the pencil icon, edit, then
**Commit changes** (commit directly to `main`). The Build ROM run starts on its own.
If a build fails, the run shows a red X; open it to see the error, or just ask Claude.

### Dialogue

Each map's text lives in `data/maps/<MapName>/scripts.inc`, near the bottom, like this:

```
PalletTown_ProfessorOaksLab_Text_ThoseArePokeBalls::
	.string "A shelf packed with POKé BALLS.\n"
	.string "They hold POKéMON of every type!$"
```

- Change only the text inside the quotes. Keep the label line (ending in `::`) as is.
- `\n` = second line of the box, `\l` = scroll to a new line, `\p` = new text box,
  `$` = end of the message (must be on the last line).
- A line holds about 34 characters. Longer lines run off the edge of the box.
- `{PLAYER}` and `{RIVAL}` insert names; `{STR_VAR_1}` etc. are filled in by the game.
- Use GitHub's search (press `/` in the repo) to find a line you saw in the game.
  The game writes most names in caps (POKéMON, PROF. OAK), so search for the exact wording.

### Trainers

- The rival's teams: `src/data/rival_teams_frlg.party` (one entry per stage per type, e.g.
  `RIVAL_TEAM_CERULEAN_WATER`). Change species, levels, held items (`Squirtle @ Oran Berry`),
  or add moves on lines starting with `- `. Without moves, a Pokémon uses its normal level-up moves.
- Every other trainer: `src/data/trainers_frlg.party`, same format.

## Design decisions

- **Modern typings** (Fairy included). 17 selectable types; Flying is excluded because no Gen 1-3
  Pokémon has Flying as its primary type.
- **Primary type** means `gSpeciesInfo[species].types[0]`.
- The player's type is stored in `VAR_PLAYER_TYPE` (0x40F7) as a `TYPE_*` constant; 0 = not chosen yet.
- **Rival** specializes in a type that is super effective against the player's (table in
  `src/gym_hack.c`), stored in `VAR_RIVAL_TYPE` (0x40F8). His starter is the starter for his type.
  The original rival battles are left in the scripts; `GetTrainerStructFromId` swaps in his
  type-specialist team at battle time (`GymHack_GetRivalTrainer`).
  12 types are possible rival types. Steel and Ice borrow one line each from a secondary
  type (Magnemite/Magneton, Lapras), because those types have too few non-legendary lines.
- **Team rules** (`GymHack_CheckTeam`): every non-egg Pokémon in the party must have the player's
  type as its primary type, and no two may be the same species (different stages of one family are
  fine). Ordinary trainers and wild battles have no rules.
  - Gym leaders and rival battles are checked when the battle is about to start
    (`ScrCmd_dotrainerbattle` → `GymHack_GetBlockedBattleScript`). If the team breaks a rule, the
    game explains why and reloads the map in place, so a rival who walked up resets.
  - The Elite Four and Champion are checked once, at the League door in the Indigo Plateau lobby,
    so the player can't get stuck between Elite Four rooms.
- **Evolution toggle**: party menu → EVOLVE: ON / EVOLVE: OFF (shows the current setting; selecting it switches). Stored in a spare bit of the
  Pokémon's data (`MON_DATA_EVOLUTION_LOCKED`, formerly unused), so old saves load fine. Blocks
  level-up, trade and friendship evolution like an Everstone, but evolution stones still work.
- **HM key items**: Hedge Clippers (Cut), Flashlight (Flash), Boogie Board (Surf), Hot Air Balloon
  (Fly), Can of Spinach (Strength), Sledgehammer (Rock Smash), Rocket Boots (Waterfall). Given by the same NPCs right after each HM. They need the same
  badges as the HMs. Walking up to a tree, boulder or water offers the item when no party Pokémon
  knows the move (`checkfieldmove` returns `FIELD_MOVE_USER_ITEM`); they can also be used from the
  bag or registered to SELECT. Icons drawn by `tools/gym_hack/make_hm_item_icons.py`.
- **Wild Pokémon** are planned in `planning/wild-pokemon-planner.xlsx` (one row per encounter slot).
  To change them: edit the yellow cells, then run
  `python3 tools/gym_hack/import_wild_sheet.py <species_info.i> planning/wild-pokemon-planner.xlsx`
  (see `export_wild_sheet.py` for making `species_info.i`), or ask Claude.
- **No trading needed**: Kadabra, Machoke, Graveler and Haunter evolve at level 36. Items that normally
  need a trade (King's Rock, Metal Coat, Dragon Scale, Up-Grade, Deep Sea Tooth/Scale, Prism Scale)
  are used from the bag like evolution stones (`I_USE_EVO_HELD_ITEMS_FROM_BAG`). Eevee becomes Espeon
  with a Sun Shard and Umbreon with a Moon Shard (FireRed has no clock). All of these, plus every
  evolution stone, are sold at the Celadon Dept. Store 4F. Evolutions into Pokémon after Gen 3 are
  turned off (`P_GEN_x_CROSS_EVOS`). `tools/gym_hack/check_obtainable.py` (run on every GitHub build)
  checks that all 386 can be caught or evolved.
- **Gym type finder**: on the Town Map (the item or a wall map), SELECT shows a Poké Ball on every
  area whose wild tables (grass, water, fishing, Rock Smash) include a Pokémon whose primary type is
  the player's; SELECT again hides them. It never says which Pokémon. Only the map being shown
  (Kanto, or the Sevii group the player is on) gets markers. Worked out live from the wild tables, so
  edits to the planner show up automatically (`GymHack_GetTypeFinderMapSecs`, `src/field_region_map.c`,
  marker art `graphics/gym_hack/type_finder_marker.png`).
- **Pallet Town** is rebuilt and much bigger (60x52 blocks): raised yards for the rival and a
  neighbor joined by a bridge over the north road, the player's gym on a rocky plateau in the middle,
  Oak's lab on a grassy rise, a Pokémon Center and Poké Mart in the east, a lake and waterfall, and
  the player's house on a peninsula below the cliffs. The whole map is drawn by
  `tools/gym_hack/build_pallet_town.py`: edit the LAYOUT section (plain x/y coordinates) and run
  `python3 tools/gym_hack/build_pallet_town.py preview.png`. It also writes the map's warps, signs and
  people (`map.json`) and Oak's walking routes (`movements.inc`), worked out from the layout, so the
  intro and post-League scenes follow the new streets. Don't hand-edit those two files or the
  layout in Porymap; changes would be overwritten the next time the script runs.
  The gym door is locked for now; the gym itself comes in the next step.
  Houses take a roof color (`house(x, y, "blue")`: red, blue, green or purple). The Chansey statue,
  benches and street lamps are pixel art in `tools/gym_hack/pallet_art.py` (`statue()`, `bench()`,
  `lamp()` in the layout); the script adds their tiles and palettes to the Pallet Town tileset.
- **Story**: in the starter scene, Oak explains that the POKéMON LEAGUE is adding a ninth gym in
  Pallet Town (so many trainers reach the League that the Elite Four never get a day off), and he has
  picked the player and the rival as candidates. League rules say a Gym Leader uses one primary type,
  so the whole League challenge is run that way. When both finish the League, they battle once more
  for the gym. After the first rival battle Oak takes them both to the empty gym
  (`PalletTown_Gym_Frlg`, scene in its `scripts.inc`), invites them to decorate it whenever they're
  in town (part of their "audition"), and jokes that he'll run it himself if neither is good enough.
  The gym door is swapped for a locked one (`setmetatile` in Pallet's `OnLoad`) until
  `FLAG_PALLET_GYM_OPEN` is set. Scene state: `VAR_MAP_SCENE_PALLET_TOWN_GYM` (0x40F9).
  The gym inside is drawn by `tools/gym_hack/build_pallet_gym.py` (25x28, Viridian Gym tiles).
- **Gym decorations**: the gym has a PC left of the entrance (`data/scripts/gym_decor.inc`). BUY
  DECORATIONS lists everything unlocked so far with its price, how many you have and a short
  description; bought pieces go to the gym's storage. DECORATE starts edit mode: walk anywhere in
  the gym (you can walk through things while decorating) and the selected piece is shown in front
  of you. A places it, SELECT puts away the one in front, L/R switch pieces, B finishes. Nothing can
  go on the walls, the doormat, the statues or the PC, and if you've boxed yourself in when you
  finish, you're put back at the PC. Invisible walls are shown while decorating.
  - Catalog: `DECORATIONS` in `tools/gym_hack/build_gym_decor.py` (name, price, unlock flag,
    description, and the blocks that draw it). Run `python3 tools/gym_hack/build_pallet_gym.py`
    after changing it. New entries go at the end (saves store decorations by position).
    `python3 tools/gym_hack/build_gym_decor.py catalog.png` draws every piece with its price.
  - Unlocks: an entry's `unlock` is a flag, usually a badge (`FLAG_BADGE02_GET` = Misty's water).
    It's just a flag, so any trainer's defeat flag works too.
  - Pieces are blocks in the gym's own tileset (`data/tilesets/secondary/pallet_gym`, a copy of the
    Viridian Gym tileset plus the decorations): furniture from the indoor tileset drawn over the gym
    floor, Viridian gym pieces (spinners, stop tiles, walls, statues) as they are, and pieces imported
    from other gyms (Cerulean water, the Vermilion trash can) with their tiles and palettes.
    Spinners and water keep their behavior, so they work like the real gyms' versions.
  - Save data: what you own and where it's placed live where Emerald keeps secret bases
    (`gSaveBlock1Ptr->secretBases`, 3,200 bytes that FireRed never uses). `src/gym_decor.c` marks
    it with a magic number and clears it on a new game or an older save. Record mixing never
    touches it.
  - Capacity: up to 256 kinds of decoration (255 of each), and 700 placed pieces (more than the
    room's ~500 free floor squares). The gym tileset has 384 blocks and 384 tiles; the 19 pieces
    so far use 118 blocks. The real limit is palettes: the gym tileset had 3 spare palette slots,
    and the water (1) and trash can (2) use all of them, so pieces with new colors from other
    tilesets need palettes merged or shared (the importer would have to fold similar colors into
    existing palettes). Furniture and anything drawn in the existing gym/indoor colors costs no
    palette.
- **Planned ending twist**: after the player beats the rival and goes to take over the gym, Oak
  reveals he wants it for himself and challenges the player to prove they're good enough.
- **Oak's lab**: the three-ball table is replaced by two shelves of Poké Balls (new art in the lab
  tileset, palette 09). Checking the shelf opens the type menu.

## Code map

| File | What |
|---|---|
| `src/gym_hack.c`, `include/gym_hack.h` | Hack logic: starter and rival tables, specials, rival team swap |
| `src/data/rival_teams_frlg.party` | The rival's teams |
| `test/gym_hack.c` | Automated tests (`make check BUILD=firered TESTS="GymHack"`) |
| `data/maps/PalletTown_ProfessorOaksLab_Frlg/` | Oak's lab: type menu, shelf, rival's pick |
| `data/scripts/gym_hack.inc` | Team-rule messages, HM key item scripts and text |
| `src/field_region_map.c` | Town Map screen: type finder markers |
| `tools/gym_hack/build_pallet_town.py` | Draws Pallet Town and writes its events and Oak's routes |
| `tools/gym_hack/pallet_art.py` | Chansey statue, bench and lamp art |
| `tools/gym_hack/build_pallet_gym.py`, `data/maps/PalletTown_Gym_Frlg/` | The gym inside and Oak's gym scene |
| `tools/gym_hack/build_gym_decor.py` | Decoration catalog and the gym tileset that draws it |
| `src/gym_decor.c`, `include/gym_decor.h`, `data/scripts/gym_decor.inc` | Gym PC, decoration shop, edit mode, save data |
| `tools/gym_hack/playtest/` | Headless emulator harness used to playtest scenes |
| `data/maps/PalletTown_House1_Frlg/` etc. | New Pallet interiors: two neighbors, Pokémon Center, Mart |
| `src/item_use.c` (bottom) | Using the HM key items from the bag |
| `data/maps/IndigoPlateau_PokemonCenter_1F_Frlg/` | League door team check |
| `tools/gym_hack/` | Helper scripts: map preview renderer, rival team generator, shelf art |
| `.github/workflows/build-rom.yml` | Automatic ROM builds |

## Building locally

```
make firered -j$(nproc)                                   # -> pokefirered.gba
make check BUILD=firered TESTS="GymHack" -j$(nproc)       # run this hack's automated tests
python3 tools/gym_hack/render_map.py <MapFolder> out.png  # preview a map
```

## Roadmap

1. [x] Source build, debug menu
2. Type identity
   - [x] 17-starter type selection in Oak's lab (Poké Ball shelf)
   - [x] Rival specializes in a counter type
   - [x] Type + no-duplicates check for gym, rival and Elite Four battles
3. [x] Evolution toggle per Pokémon (no held item); HMs as key items
4. [x] All 386 Pokémon obtainable; gym type finder on the Town Map
5. Gym builder and challengers
   - [x] Pallet Town redesign (gym plateau, Pokémon Center, Mart, player's house by the water)
   - [x] Gym interior (empty) and Oak's ninth-gym story at the starter scene
   - [x] Gym decorating system: gym PC shop, unlocks by badge, edit mode, save data; 19 test pieces
         (furniture, walls, statue, water, trash can, fern, invisible walls, spinner and stop tiles)
   - [ ] More decorations: pool edges, teleport pads (need warp scripting), working trash-can
         switch puzzle, Cinnabar/Saffron/Celadon pieces, house decorating
   - [ ] Final rival battle for the gym after the League, then Oak's challenge (twist)
   - [ ] Challengers and the ninth badge after becoming Leader
