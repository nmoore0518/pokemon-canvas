# Pokémon Canvas (the Gym Trainer hack)

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

- **Modern typings** (Fairy included), with one change: the birds are Flying first. Pidgey,
  Spearow, Wingull, Doduo, Natu, Taillow and Hoothoot lines, Aerodactyl and Farfetch'd (and Mega
  Pidgeot/Aerodactyl) swap their two types, so Flying is a team type like any other (types are
  in `src/data/pokemon/species_info/gen_*_families.h`). 18 selectable types, listed easiest first (see below); Flying's starter is
  Spearow and its rival is Electric. The rival's Psychic and Rock teams use Chimecho and
  Anorith/Armaldo in place of Natu/Xatu and Aerodactyl.
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
  badges as the HMs, except Fly, which needs no badge at all (move or balloon). Oak gives the
  Hot Air Balloon in the gym scene, right when he says the player can decorate (or in his lab, on
  an older save); the Route 16 girl still gives HM02, and the balloon only if the player has none. Walking up to a tree, boulder or water offers the item when no party Pokémon
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
  the player's; SELECT again hides them (the hint sits top left, clear of Cinnabar). It never says which Pokémon. Only the map being shown
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
- **Type menu order = difficulty.** Oak's list runs from the type with the strongest possible
  team to the weakest: the highest total base stats of six different species with that primary
  type (Gen 1-3, legendaries included). Oak says so before the menu, and picking one of the last
  four (`GYM_HACK_HARD_STARTERS_FROM`) gets a "bold choice" warning. A test keeps the order right.

  | # | Type | Best six (BST total) |
  |---|---|---|
  | 1 | Psychic | 3685 (Mewtwo, Lugia, Mew, Celebi, Deoxys, Espeon) |
  | 2 | Dragon | 3570 (Rayquaza, Dragonite, Salamence, Latias, Latios, Altaria) |
  | 3 | Fire | 3463 (Ho-Oh, Moltres, Entei, Arcanine, Charizard, Typhlosion) |
  | 4 | Water | 3405 (Kyogre, Suicune, Gyarados, Kingdra, Milotic, Lapras) |
  | 5 | Steel | 3285 (Metagross, Jirachi, Registeel, Aggron, Steelix, Skarmory) |
  | 6 | Normal | 3255 (Slaking, Snorlax, Blissey, Porygon2, Ursaring, Kangaskhan) |
  | 7 | Electric | 3175 (Zapdos, Raikou, Jolteon, Ampharos, Electrode, Electabuzz) |
  | 8 | Rock | 3160 (Tyranitar, Regirock, Golem, Omastar, Kabutops, Cradily) |
  | 9 | Ground | 3125 (Groudon, Flygon, Donphan, Claydol, Rhydon, Sandslash) |
  | 10 | Grass | 3090 (Exeggutor, Sceptile, Venusaur, Meganium, Vileplume, Victreebel) |
  | 11 | Ice | 3075 (Articuno, Regice, Walrein, Glalie, Jynx, Piloswine) |
  | 12 | Poison | 3002 (Crobat, Nidoqueen, Nidoking, Muk, Weezing, Swalot) |
  | 13 | Bug | 2970 (Shuckle, Scyther, Pinsir, Scizor, Heracross, Forretress) |
  | 14 | Flying | 2841 (Aerodactyl, Pidgeot, Dodrio, Xatu, Swellow, Noctowl) |
  | 15 | Fighting | 2799 (Machamp, Hariyama, Primeape, Hitmonlee, Hitmonchan, Hitmontop) |
  | 16 | Dark | 2745 (Umbreon, Houndoom, Absol, Sneasel, Mightyena, Murkrow) |
  | 17 | Ghost | 2560 (Gengar, Banette, Dusclops, Misdreavus, Haunter, Gastly) |
  | 18 | Fairy | 2206 (Clefable, Granbull, Togetic, Clefairy, Snubbull, Togepi) |
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
  finish, you're put back at the PC. Invisible walls are shown while decorating. The help box
  sits at the bottom of the screen.
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
  - Shop and storage are split into categories: GENERAL, TRAINERS and one per type (only the
    categories with something in them are listed).
  - **Type decorations** (`TYPE_DECORATIONS` in `build_gym_decor.py`, art drawn in code in
    `tools/gym_hack/decor_art.py`): 5 per type, 90 in all, from 1x1 to 3x3, ¥800-¥12,000
    (e.g. Electric: TESLA COIL, GENERATOR, GIANT BATTERY, NEON SIGN, PLASMA BALL; Fire:
    FIREPLACE, BRAZIER, LAVA LAMP, CAMPFIRE, VOLCANO MODEL). Flat ones (rugs, mats, ponds) can be
    walked on; the top row of tall ones can be walked behind. A type's decorations go on sale after
    a key battle (`sTypeUnlocks` in `src/gym_decor.c`), and every type unlocks when the gym becomes
    the player's (`FLAG_PALLET_GYM_AWARDED`):
    Normal: rival in Cerulean · Fighting: Fighting Dojo master · Psychic: Sabrina · Poison: Koga ·
    Dragon: Lance (Elite Four) · Ghost: rival in Pokémon Tower · Dark: Picnicker Dana (the last
    trainer you must beat in Rock Tunnel, found by path analysis of both floors) · Ground: Super
    Nerd Miguel (Mt. Moon fossils) · Fire: Blaine · Fairy: rival on the S.S. Anne · Water: Misty ·
    Flying: rival in Silph Co. · Rock: Brock · Electric: Lt. Surge · Bug: Bug Catcher Sammy (the
    only trainer you must beat in Viridian Forest) · Grass: Erika · Ice: Giovanni (Viridian Gym) ·
    Steel: Giovanni (Rocket Hideout). Trainer battles count through their trainer flags, so older
    saves past those battles already have them.
  - How the art fits: the gym tileset has no room for ~800 more tiles, so code-drawn pieces live
    in video memory the gym never uses (indoor-tileset tiles no gym block uses, plus spare gym
    tiles). The builder finds those spots and the game copies the art there whenever the gym's
    tileset loads (`GymDecor_PatchTilesetGfx`, hooked in `src/fieldmap.c` next to the badge). All
    of it is always loaded, so any mix can be placed. Their blocks use the gym tileset's spare
    blocks. Colors: three shared 15-color palettes (gym slots 8-10, `decor_art.PALETTES`); each 8x8
    tile uses one, and the Cerulean water and Vermilion trash can were redrawn in them.
  - Capacity: up to 256 kinds of decoration (255 of each in storage, 114 kinds so far), and 650
    placed pieces (more than the room's ~500 free squares).
    Space left for more code-drawn art: 12 video memory tiles and 41 gym blocks (the builder
    prints this). Adding much more art would need loading only the placed pieces.
- **Practice trainers** (TRAINERS in the gym PC: ROOKIE ¥5,000, SKILLED ¥10,000, EXPERT ¥20,000).
  Buy as many as you like (storage holds 255 of each, like every decoration); up to 11 can be
  placed at once, any mix of ranks. That's the GBA's limit: it shows at most 16 people at once and
  the player, aide, challenger, Oak and the rival need 5. Placed like decorations; a person stands
  on the spot (11 map objects `LOCALID_PALLET_GYM_TRAINER_0..10`, handed out in placement order and
  dressed for their rank through `VAR_OBJ_GFX_ID_1..B` by `GymDecor_SetupTrainers` on entering the
  gym and after decorating; while decorating they step aside and an icon shows their spot). Each
  trainer has their own name.
  They don't affect challengers (who just walk around them). Talk to one for a practice battle
  (`data/scripts/gym_practice.inc`): a fresh random team each time, of the player's type with no
  duplicates (the League rules), 3-5 Pokémon, levels averaging 5 below the player's (at least 2),
  no legendaries. ROOKIE: mostly first forms, some second forms, simple AI, low IVs. SKILLED:
  mostly second forms, some first forms, standard AI, medium IVs, some held items. EXPERT: fully
  evolved only (earlier forms only if the type doesn't have enough, e.g. Fairy), smart AI, max IVs,
  held items. Each trainer keeps the same name. No prize money (`gGymPracticeBattle` skips it in
  `data/battle_scripts_1.s`), losing doesn't white out, and your team is healed after.
  `GymChallenge_GeneratePractice` in `src/gym_challenge.c`.
- **Stat training items** (`src/stat_items.c`, `data/scripts/stat_items.inc`), all used up when
  used, all chosen from the bag and then a Pokémon:
  - Silver Bottle Cap (5,000): pick one stat; its IV becomes 31.
  - Gold Bottle Cap (25,000): every IV becomes 31.
  - Ability Capsule (20,000): pick any of the species' Abilities, the hidden one included.
  - Cilantro (15,000): pick a nature from all 25. It sets the stat nature (like a mint), and the
    summary screen shows the new nature.
  - Multivitamin (30,000): an editing screen for all six EVs. Left/Right move 4 at a time, L/R
    set none or the most allowed, and it never lets a stat pass 252 or the total pass 510. It
    shows what each stat would be. Cancelling, or changing nothing, doesn't use it up.
  - Sold only at the Pallet Town Mart (`GymHack_PalletMart`): Silver Bottle Cap after Erika's
    badge, Ability Capsule after Sabrina's, Cilantro after Blaine's, Gold Bottle Cap and
    Multivitamin after the Elite Four (`FLAG_SYS_GAME_CLEAR`).
  - One-time gifts, each with the leader's own lines: a Silver Bottle Cap from every gym leader
    (after the TM), plus Cilantro from Erika and an Ability Capsule from Koga; a Gold Bottle Cap
    from the rival at Pokémon Tower; a Multivitamin from Giovanni at Silph Co. Flags
    `FLAG_GOT_SILVER_CAP_FROM_*` etc. in `include/constants/flags.h`. If the bag is full the
    gift goes to the PC.
- **Gym challenges** (`src/gym_challenge.c`, `data/scripts/gym_challenge.inc`): once the player
  runs the gym, Oak's aide (standing right of the entrance statues) sends in challengers one at a
  time, Battle Frontier style, until one beats the player.
  - Opens once the player has beaten Oak for the gym (`GymChallenge_IsOpen`).
  - The player's team must follow the one-type rule. The party is healed between battles, and a
    loss doesn't white out (`B_FLAG_NO_WHITEOUT` = `FLAG_GYM_CHALLENGE_BATTLE`).
  - Where: the BATTLE STAGE, a free decoration that's always in the PC's storage (one of it).
    The player stands on its top mark and challengers on the bottom one. Starting from the aide,
    the player walks there, and each challenger walks from the door to it and back out, along
    paths found at run time (`GymChallenge_FindPath`: a search over the room that avoids
    decorations, the aide, spinners and water). With no stage placed, or none reachable, they
    battle just below the aide.
  - Challengers (one trainer slot, `TRAINER_GYM_CHALLENGER`, built at run time and returned by
    `GetTrainerStructFromId`). Every team's average level is the player's party average.
    - Random trainers: random class and name, 4-6 different fully evolved Pokémon (base stats
      420+) from the first 386, legendaries included; some hold items. Smart AI.
    - Famous (every 7th challenger in a streak, and 1 in 6 otherwise): the eight gym leaders,
      the Elite Four, the rival (his type team, with his name), Giovanni and Oak (his own team).
      Their real teams are levelled to match and evolve if the new level calls for it; leaders
      with fewer than 4 Pokémon get extras of their type.
    - Townsfolk (1 in 12): Mom, Nurse Joy, Bill, Daisy and Mr. Fuji, with teams made for them.
      They use the nearest existing battle pictures (Breeder, Aroma Lady, PokéManiac, Beauty,
      Gentleman) since FireRed has none of their own.
  - Each win pays 2,000 + 200 per win in the streak (doubled for famous and townsfolk; capped
    at 40) and an item: Rare Candy, Nugget, vitamins, Full Restore, PP Up, Silver Bottle Cap...
    Famous challengers, every 10th win and 1 win in 16 give something rare (Gold Bottle Cap,
    Ability Capsule, Cilantro, Multivitamin or a Master Ball).
  - Saved (in the secret base space after the decorations): current streak (kept if you stop),
    best streak, wins and losses, and the last 20 challengers who beat the player, shown on the
    gym statue. The Pallet Town sign names the player as leader.
- **The player's badge** (`src/gym_badge.c`, art from `tools/gym_hack/badge_art.py` into
  `graphics/gym_badges/`): one per type, 32x32. Normal Vanilla, Fighting Knockout, Flying Rocket,
  Poison Venom, Ground Shovel, Rock Obsidian, Bug Infestation, Ghost Death, Steel Wrench, Fire Lava,
  Water Monsoon, Grass Weed, Electric Amp, Psychic ESPN, Ice Sub-Zero, Dragon Egg, Dark Evil, Fairy
  Elder. A challenger who beats the player is presented with it (its picture shows on screen), and
  the statue lists them as having earned it.
  - BADGE PEDESTAL decoration: shows the badge on a stone plinth. Sold once
    `FLAG_PALLET_GYM_AWARDED` is set (when the player beats Oak for the gym).
  - The gym tileset holds one badge (16 tiles from `GYM_BADGE_TILE_START`, palette slot 11, kept
    free for it); whenever the gym's tileset loads, `src/fieldmap.c` calls into gym_badge.c to copy
    the player's own badge and colors over it. To change the art, edit badge_art.py and run it,
    then `build_pallet_gym.py`.
- **The finale** (`data/scripts/gym_finale.inc`, `VAR_GYM_FINALE`):
  - In the Champion's room (first time only), Oak walks in before the rival battle, heals the
    player's team ("I want a fair fight!"), and announces that the winner becomes Champion and
    Leader of the Pallet Town Gym. After the win he congratulates the player on both.
  - After the Hall of Fame and credits, the next time the player steps out of their house, Oak
    meets them at the door and takes them to the gym. There he has a crisis: it's his town, next
    to his lab, and he won't hand it to a kid ("Remember when I said I'd run it myself? I wasn't
    joking!"). He challenges the player for it.
  - The player can put it off ("come challenge me in the GYM when you're ready"); Oak then waits
    in the gym. The one-type and no-duplicate rules apply (he refuses an illegal team).
  - The battle: Oak's team (Tauros, Exeggutor, Arcanine, Gyarados, Snorlax, Dragonite) with full
    IVs, EVs, held items, competitive moves, two Full Restores and the smartest AI; five levels
    above the player's average, never below 65. Champion battle music.
    `GymChallenge_GenerateOakFinale` in `src/gym_challenge.c`.
  - Losing sends the player home (no money lost), and the scene plays again when they step out.
  - Winning: Oak apologizes and hands over the gym (`FLAG_PALLET_GYM_AWARDED`, which opens the
    gym challenges and the badge pedestal), the League makes the player's badge official, and Oak
    introduces his aide and explains the challenge system before leaving.
  - The National Pokédex scene outside the house waits until the gym is the player's.
- **The candidacy** (`src/gym_candidate.c`, `data/scripts/gym_candidate.inc`): in Oak's first gym
  scene he explains the League's rule: a new Gym Leader needs the approval of all eight Gym
  Leaders and the Elite Four. The League's front door (Indigo Plateau) turns the player back
  until all eight Gym Leaders approve. The Pallet gym's aide shows the count.
  - **Guest Leader battles**: after the badge (and again whenever you talk to them until they
    approve), each leader asks you to hold their gym. A challenger walks in (fade, then they're
    beside you), and you battle in the leader's place: one challenger for Brock, Misty, Surge and
    Erika, two for Koga, Sabrina, Blaine and Giovanni. Challengers are any type, no legendaries,
    with party sizes 2-5 and levels 12-47 by gym (higher if the player is ahead), more evolved
    later in the story (`gGymCandidateLeaders`). Each challenger pays the stand-in a Gym Leader's
    prize, doubled (8 x their last Pokémon's level x 25: about ¥2,400 at Brock, ¥9,400 each at
    Giovanni, ~¥85,000 over the story; `GYM_GUEST_PRIZE_RATE`), to fund decorations. Your team
    must follow the one-type rule; losing just means trying again (no money lost, team healed).
  - **Stand-in shifts** (optional, repeatable): once a leader approves, talking to them again gets
    an offer in their own voice (Brock's stew, Misty's swim, Surge's R&R, Erika's nap, Koga
    vanishing, Sabrina foreseeing it, Blaine's hot springs, Giovanni's "business"). One
    challenger (even at Koga and later), levels 2 below to 3 above the player's average and more
    evolved as the player grows, paying a normal Gym Leader's prize (100 x their last Pokémon's
    level) plus EXP. Losing costs nothing. Once the Pallet gym is the player's, leaders turn the
    offer down in character ("you've got your own GYM to run now"); the gym challenges take over. Approval flags
    `FLAG_GYM_APPROVAL_*` (0x0BA-0x0C1). Each gym map has a spare person
    (`LOCALID_<GYM>_GYM_GUEST`, hidden by `FLAG_HIDE_PALLET_GYM_CHALLENGER`).
  - The Elite Four each give their approval after losing.
  - **Mentors**: the leader or Elite Four member of the player's type. Oak writes to them about
    the player: Oak's Parcel now carries their reply (the Viridian Mart clerk mentions a letter;
    Oak reads it out: they're coming to Pallet Town today), and right after the Pokédex scene
    they walk in through the lab door (a path found on the map, `GymCandidate_PlanVisitorWalk`),
    Oak greets them, and they introduce themselves in character: who they are, their story and
    advice about running a gym, a yes/no question with an answer for each, a housewarming gift
    (the cheapest decoration of the player's type, into the gym PC), and a farewell before they
    walk back out. The gift visits at Cerulean and Cinnabar also walk in from the gym's door. They meet the player again
    later as someone they know (after their badge, or after the Elite Four intro) and come
    around at approval. They bring two held items for the player's type (below): the defensive
    one after Mt. Moon (they drop by the Cerulean Gym after the badge) and the offensive one at
    the Cinnabar Gym after the badge. If the mentor runs that gym (Misty for Water, Blaine for
    Fire) they hand it over themselves. Talking to Misty or Blaine again gives a missed one.
    `VAR_MENTOR_STAGE` (0x40FC) holds `MENTOR_MET`, `MENTOR_GAVE_GUARD`, `MENTOR_GAVE_EDGE`.
    Friendly:
    Brock, Erika, Blaine, Lorelei, Bruno, Sabrina (cool but accepting). Prickly: Misty, Surge,
    Koga, Giovanni, Agatha, Lance. Types with no Kanto leader go to the closest fit:
    Normal→Brock, Steel→Surge, Bug→Koga, Fairy→Sabrina, Dark→Giovanni, Flying→Lance.
  - **The rival** is the other candidate: his Route 22, Cerulean, S.S. Anne, Pokémon Tower and
    Silph Co. lines are about the race for approvals and the Pallet gym.
  - **Oak's doubts** grow quietly: lines in his lab as approvals come in (picked the gym's floor
    tiles himself; "I was quite the TRAINER in my day"; checking the gym's lights at night), Daisy
    noticing him visiting the gym at night in his old cap (4+ approvals), and the rival
    mentioning that Gramps keeps asking about the gym.
  - **Trainers who join**: one trainer per type on the road asks to join your gym after you beat
    them (and when you talk to them again, until they do). They go to the gym PC's TRAINERS
    storage and are placed like the bought trainers; in practice battles they use a team of their
    own favorite type. Andrea (Normal, Route 8), Hitoshi (Fighting, Dojo), Perry (Flying, Route
    13), Jared (Poison, Route 13), Drew (Ground, Route 9), Marcos (Rock, Mt. Moon), Rick (Bug,
    Viridian Forest), Angelica (Ghost, Pokémon Tower), Bernie (Steel, Route 11), Thomas (Fire,
    S.S. Anne), Ned (Water, Route 12), Alicia (Grass, Route 9), Luca (Electric, Route 12), Nelson
    (Psychic, Victory Road), Nora (Ice, Route 20), Shirley (Dragon, Route 20), Lewis (Dark,
    Pokémon Mansion), Julia (Fairy, Route 8). Kanto has no real Ice, Dark or Steel specialists,
    so those three are themed (Nora swims near Seafoam with Shellder and Cloyster, Lewis is a
    burglar going straight, Bernie's Magnemite).
- **The Move Extractor** (`src/move_extractor.c`, `data/scripts/move_extractor.inc`): a battle
  item (ITEMS pocket, `ITEM_MOVE_EXTRACTOR`). Used from the bag after a foe has used a move, it
  asks "Extract <move> from <foe>?", then the user learns that move (the usual "forget a move?"
  screen if it knows four). The learned move takes the learner's primary type (a new STAB move);
  nothing else about it changes. The foe can't use that move again in the battle (its PP drops to
  0). It's used up only when the move is learned; declining keeps it (the turn is still spent,
  like any item). Can't be used before a foe has moved, on a move the Pokémon already knows, or
  while transformed.
  - Storage: 4 bits per Pokémon (`MON_DATA_STOLEN_MOVES`, spare bits in substruct 0), one per move
    slot. They follow the move when moves are reordered (summary screen, Move Deleter) or shift
    (level-up forgetting the first move), and clear when the slot gets a different move.
    `GetDynamicMoveType` returns the primary type for those moves; the battle move menu and the
    summary screen show it.
  - Only three in the game, from the Team Rocket grunt on Nugget Bridge (`VAR_MOVE_EXTRACTOR_STAGE`):
    1. Say YES when he asks you to join (or later, if you said no and beat him, when you talk to him
       again). Surprised, he admits a grunt can't let anyone in, and gives you a Move Extractor to
       prove your worth by stealing moves.
    2. After you beat Giovanni at Silph Co., he finds you in Saffron City, panicking that he
       recommended the kid who beat the boss, but the boss said "That child has talent."
    3. After Giovanni gives his Gym Leader approval (so the Viridian gym's Guest Leader battles come
       first), he finds you in Viridian City: the boss has disbanded Team Rocket, so there's no one
       to recommend you to. He gives you the last one, says to make your gym the toughest in Kanto,
       and thinks he might go straight too.
    He appears next to the player on entering the city (`FLAG_HIDE_MOVE_EXTRACTOR_GRUNT`,
    `LOCALID_SAFFRON/VIRIDIAN_EXTRACTOR_GRUNT`, disguised as on the bridge).
  - Giovanni now says his farewell and leaves once he's approved (as in FireRed); he doesn't offer
    stand-in shifts.
- **Type items** (`HOLD_EFFECT_TYPE_GUARD` / `HOLD_EFFECT_TYPE_EDGE` in `src/battle_util.c`): two
  one-of-a-kind held items per type, from the mentor. Neither is ever used up.
  - Defensive: no move is super effective on the holder; those hits do normal damage (like a
    resist berry, but for every type and forever).
  - Offensive: the holder's damaging moves hit for at least normal damage; resisted or
    type-immune hits become normal ones. Super effective hits don't change. Immunity from
    abilities or items (Levitate, Wonder Guard, Air Balloon, Magnet Rise) still blocks.
  - They're ordinary held items (GIVE from the bag or the party menu), but they can't be stolen,
    knocked off, tricked away or flung, can't be sold, and the bag hides TOSS for them (and for
    important items like the Move Extractor). Icons are drawn in code
    (`tools/gym_hack/type_item_art.py`, `graphics/items/icons/gym/`).

    | Type | Defensive | Offensive |
    |---|---|---|
    | Normal | Fluffy Sweater | Third Eye |
    | Fighting | Mouth Guard | Steroids |
    | Flying | Aviator Goggles | Jet Engine |
    | Poison | Titanium Coil | Corrosive Venom |
    | Ground | Granite Sarcophagus | Diamond Drill Bit |
    | Rock | Diamond Plating | Precision Chisel |
    | Bug | Cacoon | Extra Stinger |
    | Ghost | Light Switch | Ouija Board |
    | Steel | Stainless Alloy | Knife Sharpener |
    | Fire | Heat Shield | Oxidizer |
    | Water | Cistern | Nozzle |
    | Grass | Chicken Wire | Miracle Gro |
    | Electric | Capacitor | Transformer |
    | Psychic | Energy Shield | Psionic Blade |
    | Ice | Cooler | Rock Salt |
    | Dragon | Fire Shroud | Bionic Jaw |
    | Dark | Black Veil | Manifesto |
    | Fairy | Twinkling Lights | Magic Wand |
- **The Palm Pilot** (`data/scripts/palm_pilot.inc`, `ItemUseOutOfBattle_PalmPilot` in
  `src/item_use.c`): Bill's portable PC, a key item. He gives it right after the S.S. Ticket ("I
  made a PC that fits in your pocket!"), or when you talk to him again if you got the ticket
  before this version. Used from the bag or SELECT, it opens the usual PC menu (Bill's/Someone's
  PC, your PC for items, Hall of Fame) anywhere on the field, except inside the Pokémon League
  (Lorelei's room through the Hall of Fame: "can't get a signal"), so the Elite Four run still
  means no box swaps between battles. It doesn't heal.
- **Quick heal pads** (`MB_HEAL_PAD`, `tools/gym_hack/build_heal_pad.py`): a glowing pad in the
  bottom-left corner of every Pokémon Center (Indigo Plateau and One Island included). Stepping
  on it fully heals the party with the healing jingle, no nurse needed. A scientist next to the
  pad in the Viridian City Pokémon Center explains the new Silph tech, and that it'll never
  replace the nurses.
- **Summary screen**:
  - **IVs and EVs**: on the SKILLS page, A cycles the stat column through stats, IVs (exact
    0-31 values) and EVs; the label and the prompt in the corner change with it
    (`P_SUMMARY_SCREEN_IV_EV_*` in `include/config/summary_screen.h`). Always available.
  - **LEVEL-UP MOVES** page (`PSS_PAGE_LEVEL_MOVES`, after BATTLE MOVES): every move the species
    learns by level with its type and level ("Evo." for moves learned on evolving), five rows at
    a time, starting near the next one. Moves it knows now are in red, the next level in red
    too, and the box at the bottom says "Next: <move> at Lv. N." A browses: up/down scroll the
    list and show each move's power, accuracy, category and description; A or B goes back.
    Contest moves are hidden (`C_HIDE_CONTEST_DATA`), so the page has the contest page's place.
- **Holding a gym** (Guest Leader battles and stand-in shifts): after the request, the leader says
  where they'll be (`observe`, in character) and, in a fade, steps aside to watch (or leaves, on a
  stand-in shift) while the player takes their spot facing the door. Each challenger walks in
  from the door to the player and back out after the battle; afterwards the leader returns
  (`GymCandidate_PlanHold`, `GymCandidate_PlanChallengerWalk`).
- **The legendary of your type** (`src/gym_legend.c`, `data/scripts/gym_legend.inc`): right after
  Oak hands the gym over, the camera shakes, the lights go out, and the most powerful Pokémon of
  the player's type appears on the battle stage (or near the middle of the room without one); the
  camera pans to it. Oak says it must have sensed a new gym of its type, that most leaders don't
  use legendaries but no rule forbids it, and that with challengers getting stronger the player
  might want to catch it. Talking to it starts a level 70 legendary battle, once. If it's
  knocked out, the player runs, or the player loses, it flees the gym; the rival walks in to
  gloat (still sore about the gym) and hints that something that strong would hide in "that cave
  by CERULEAN nobody was allowed into". From then on it's a 1% encounter anywhere in Cerulean Cave
  (`GymLegend_TryCaveEncounter`, in `TryGenerateWildMon`) until it's caught. Once it's caught (on
  the stage or in the cave), the rival comes to the gym to scoff that the player needs a
  legendary to win. `VAR_GYM_LEGEND_STATE` (0x40FE, `GYM_LEGEND_*`), `FLAG_GYM_LEGEND_APPEARED`/
  `CAUGHT` (0x0C3/0x0C4). Its overworld sprite is the
  expansion's Pokémon object (`VAR_OBJ_GFX_ID_C`).
  Normal: Meloetta, Fighting: Koraidon, Flying: Tornadus, Poison: Eternatus, Ground: Groudon,
  Rock: Diancie, Bug: Genesect, Ghost: Giratina, Steel: Dialga, Fire: Ho-Oh, Water: Palkia,
  Grass: Shaymin, Electric: Miraidon, Psychic: Cresselia, Ice: Glastrier, Dragon: Rayquaza,
  Dark: Darkrai, Fairy: Xerneas. Navel Rock's Ho-Oh (an event map) never appears now.
- **Walking leaders**: FireRed's Gym Leader and Elite Four sprites only had standing frames;
  `tools/gym_hack/make_walk_frames.py` added walking frames, so visiting mentors take steps.
- **New decorations message**: when key battles or badges unlock new gym decorations, the next
  step the player takes shows "New GYM decorations available! Check the GYM PC."
  (`GymDecor_CheckNewUnlocks`, `VAR_GYM_DECOR_SEEN` 0x40FD; nothing before the gym opens).
- **The Pallet gym door** opens and closes like other gym doors (`METATILE_PALLET_GYM_DOOR` in
  `include/constants/pallet_town_gym.h`, written by `build_pallet_town.py`; the door table in
  `src/field_door.c`). In Oak's gym scene, the rival and Oak leave over the door mat.
- **The Pokédex is secondary**: Oak hands it over as a GYM LEADER's scouting tool ("a LEADER who
  doesn't know their opponents won't last a week"), says the gym comes first, and his Pokédex
  ratings talk about challengers.
- **Trainer card fix**: a new game's Seedot size record shares a var with FireRed's trainer card
  icons, which crashed the game when flipping the card; the card now ignores invalid species.
- **Oak's aides in every gym** (`data/scripts/gym_aides.inc`): a scientist near each gym's
  entrance (beside the usual guide) gives tips for running a gym of your own, tied to that gym's
  mechanic: Pewter (the gym is a path; challengers walk from the door to your BATTLE STAGE, so
  walls shape it), Cerulean (water steers challengers, and suits a Water gym), Vermilion
  (puzzles), Celadon (plants as walls, show your type), Fuchsia (invisible walls), Saffron (keep
  challengers guessing), Cinnabar (reward type knowledge), Viridian (spin and stop tiles). After
  the badge, the aide says what it unlocked: the decoration in the GYM PC (badges 2, 3, 4, 5, 8)
  or the Pallet Mart item (badges 4, 6, 7).
- **Oak's lab**: the three-ball table is replaced by two shelves of Poké Balls (new art in the lab
  tileset, palette 09). Checking the shelf opens the type menu.

- **Johto (in progress)**: replaces the Sevii Islands as the post-game area. Olivine, Routes 34-40,
  Ecruteak (Burned Tower, Bell Tower), National Park, Goldenrod, Ilex Forest and Azalea, with all
  their interiors (89 maps), come from **Pokémon HnS** (pokehns-expansion), imported by
  `tools/gym_hack/import_johto.py`: their layouts and tilesets (attributes converted to FireRed's
  4-byte format), tileset animations, and daytime wild Pokémon at post-game levels (about 50-66);
  not their scripts (NPCs come later). Maps are named `*_Johto` (`MAP_GOLDENROD_CITY`...), in
  `gMapGroup_Johto`; warps out of the area are dropped. `tools/mapjson` accepts `REGION_JOHTO` maps
  in FireRed builds.
  - **Guards** stand where Johto goes on (Azalea to Route 33, Route 36 to Violet and the Ruins,
    Route 40 to Route 41 and the Trainer Hill, the Route 42 and Ruins gates): stepping toward them
    sets them off ("Hey! You're not from around here... Oh! A GYM LEADER CONVENTION badge!") and
    the player is turned back (`Johto_EventScript_BorderGuard`).
  - **The greeter** at Olivine's ferry dock hands over the convention goodie bag on arrival
    (`VAR_JOHTO_STATE` 0x40FF): the convention badge, Johto on the TOWN MAP (`FLAG_JOHTO_MAP` 0x0C5;
    the TOWN MAP too if missing), 5 Ultra Balls and 3 Full Restores.
  - **The Johto map** is HnS's region map with everything outside the convention area dimmed
    (`tools/gym_hack/build_johto_map.py`; `REGION_MAP_JOHTO`). On the FLY map, R switches between
    KANTO and JOHTO once the player has it; Olivine, Ecruteak, Goldenrod and Azalea are FLY spots
    once visited (`FLAG_WORLD_MAP_*` 0x0C6-0x0C9). Johto counts as Kanto for battle music.
  - **The invitation**: once the gym is the player's (`FLAG_PALLET_GYM_AWARDED`), leaving it sets
    `VAR_JOHTO_STATE` to `JOHTO_INVITE_WAITING` (Pallet Town); the next time they're in the gym,
    their mentor walks in with the GYM LEADER CONVENTION invitation (a letter on the floor if
    there's no mentor) -> `JOHTO_INVITED`.
  - **The ferry**: with the invitation, Vermilion's harbor sailor sails the player to Olivine's
    dock (the Seagallop scene, `SEAGALLOP_OLIVINE_CITY`); the sailor on Olivine's dock sails back.
  - **HGSS music**: 28 HeartGold/SoulSilver songs from HnS (`tools/gym_hack/import_hgss_music.py`):
    the towns, routes and buildings, the convention (the Radio Tower plays the Pokéathlon lobby
    theme, the halls the Gym theme, the Champions' Hall the Pokémon League theme), and Johto's
    battles (`GetBattleBGM` in `src/pokemon.c`: wild, trainers, Gym Leaders, Elite Four with the
    Kanto Gym Leader theme, Champions; victory themes in `src/battle_main.c`). They play on HnS's HGSS
    instrument set (`sound/voicegroups/hgss/`, voicegroups renamed `voicegroup_hg229`...), with its
    key split tables and samples. `MUS_HG_*` come right after the FireRed songs (`END_MUS`); the
    phonemes moved up.
  - **Convention fans** (`tools/gym_hack/johto_npcs.py`): 21 people around Goldenrod (the Radio Tower
    too), Olivine, Ecruteak, Azalea and their Pokémon Centers talk about the convention and their
    favorite leaders. They stand on free spots picked by `convention_halls.place` (in the open, out
    of the way), or spots set by hand (`NPC_SPOTS`). The rest of Johto's people are still to come.
  - **No Sevii Islands**: Bill no longer waits on Cinnabar after the Volcano Badge (saves that had
    the trip waiting drop it); **Moltres** is on Victory Road 2F (as in Red and Blue), Lv 50.
- **The Gym Leader Convention** fills Goldenrod's Radio Tower: every Gym Leader (Kahunas and Trial
  Captains too), Elite Four member and Champion from generations 1-9, 127 trainers in 18 type halls
  and the Champions' Hall (`tools/gym_hack/convention_roster.py`).
  - **The receptionist** in the lobby (1F) lists the halls with how many there the player has
    beaten (a star once cleared) and sends the player there; a hall's door leads back to the lobby.
    Halls can be visited in any order.
  - **The halls** are HnS maps (`tools/gym_hack/convention_halls.py`, run by `import_johto.py`):
    NORMAL Goldenrod Gym, FIRE Seafoam Gym, WATER the Whirl Islands' waterfall cave, GRASS Celadon,
    ELECTRIC Vermilion (its beam barrier left off), ICE Mahogany (the ice slide), FIGHTING the
    Saffron Dojo, POISON Fuchsia, GROUND Viridian, FLYING Violet, PSYCHIC Will's room, BUG the
    National Park, ROCK Pewter, GHOST the Burned Tower, DRAGON Blackthorn (its lava bridges built),
    DARK Karen's room, STEEL Olivine, FAIRY the Ecruteak Dance Theater, CHAMPIONS the Sinjoh Ruins'
    temple. Grass, Ground, Rock, Psychic and Dark are made bigger (`RESHAPE`: columns and rows
    repeated, with tile swaps so emblems and boxes aren't). Each hall's first trainer stands where
    the gym's leader stood, the others where its trainers stood (on the ice they're what stops the
    slide), then on free spots picked so nobody blocks a path and everyone can be talked to.
    `MAP_CONVENTION_HALL_*`, in `gMapGroup_Johto`.
  - **Battles**: talk to anyone. Teams are full, fully evolved and itemized
    (`tools/gym_hack/convention_teams.py`, built into `src/data/gym_convention.h` by
    `build_convention.py`): Gym Leaders Lv 75 (ace 76), Elite Four 78 (79), Champions 85-88, IVs 31,
    natures by their moves. Battles use the challenger slot (`TRAINER_GYM_CHALLENGER`,
    `GymChallenge_SetupFixed`), no whiteout, and the party is healed after every battle, won or
    lost. Rematches open once everyone in that hall is beaten (until then, the ones already
    beaten just talk, so it's clear who's left). Blue uses the rival's name.
  - **Trophies**: clearing a hall wins its trophy, clearing all 19 the Grand Trophy
    (`FLAG_CONVENTION_TROPHY_*` 0x0CA-0x0DD), sent to the gym's storage as decorations (category
    TROPHIES). They're small, one picture in many colors: the gym's video memory is full.
  - **Save**: whom the player has beaten is a bit per trainer after the challenge records
    (`GymChallengeSave.conventionBeaten`, with its own magic; the victors list went from 20 to 19
    to make room).
  - **Species**: the convention uses later Pokémon that evolve from Gen 1-3 ones (Rhyperior,
    Magnezone, Sylveon, Kingambit...). They exist (`P_GEN_x_CROSS_EVO_SPECIES`), but the 386 still
    don't evolve into them (`P_GEN_x_CROSS_EVOS` FALSE gates only the evolution links).
    `tools/gym_hack/check_convention.py` checks every team's species is in the built ROM.

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
| `src/gym_decor.c`, `include/gym_decor.h`, `data/scripts/gym_decor.inc` | Gym PC, decoration shop, edit mode, save data, type unlocks, practice trainer spots |
| `tools/gym_hack/decor_art.py` | Pixel art for the type decorations and trainer icons (`python3 tools/gym_hack/decor_art.py sheet.png`) |
| `data/scripts/gym_practice.inc` | Practice trainer battles |
| `src/gym_candidate.c`, `data/scripts/gym_candidate.inc` | Candidacy: approvals, Guest Leader battles, mentors and their gifts, Oak's asides, trainers who join |
| `tools/gym_hack/type_item_art.py`, `test/battle/gym_hack/type_items.c` | The mentors' type items (and the Palm Pilot icon): icons and battle tests |
| `data/scripts/palm_pilot.inc`, `tools/gym_hack/build_heal_pad.py` | The Palm Pilot and the Pokémon Center heal pads |
| `src/gym_legend.c`, `data/scripts/gym_legend.inc` | The legendary of the player's type in their gym |
| `src/stat_items.c`, `data/scripts/stat_items.inc` | Stat training items, EV editor, Pallet Mart stock, boss gifts |
| `src/gym_challenge.c`, `data/scripts/gym_challenge.inc` | Post-game gym challengers, rewards, records, statue |
| `data/scripts/gym_aides.inc` | Oak's aides in the eight gyms (objects in each gym's map.json) |
| `src/gym_badge.c`, `tools/gym_hack/badge_art.py` | The player's badge per type, its art, the pedestal swap |
| `tools/gym_hack/playtest/` | Headless emulator harness used to playtest scenes |
| `data/maps/PalletTown_House1_Frlg/` etc. | New Pallet interiors: two neighbors, Pokémon Center, Mart |
| `src/item_use.c` (bottom) | Using the HM key items from the bag |
| `data/maps/IndigoPlateau_PokemonCenter_1F_Frlg/` | League door team check |
| `tools/gym_hack/` | Helper scripts: map preview renderer, rival team generator, shelf art |
| `tools/gym_hack/import_johto.py`, `src/johto.c`, `data/scripts/johto.inc` | Johto: imported maps, guards, the greeter |
| `tools/gym_hack/build_johto_map.py`, `src/region_map.c` | The Johto map (dimmed outside the convention area), FLY between regions |
| `src/gym_convention.c`, `data/scripts/gym_convention.inc` | The Gym Leader Convention: receptionist, hall battles, wins, trophies |
| `tools/gym_hack/convention_roster.py`, `convention_teams.py`, `build_convention.py` | Its trainers: who's in which hall, teams and lines -> `src/data/gym_convention.h` |
| `tools/gym_hack/import_convention_sprites.py`, `convention_halls.py`, `check_convention.py` | Their sprites (Emerald Rogue), the halls (HnS gyms), the species check |
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
   - [x] Stat training items: bottle caps, Ability Capsule, Cilantro, Multivitamin (shop + gifts)
   - [x] Post-game gym challenges: random/famous/townsfolk challengers, streaks, rewards, statue
   - [x] The player's badge per type (art, award to challengers, pedestal decoration)
   - [x] Finale: Oak in the Champion's room, then Oak's challenge for the gym after the credits
   - [x] Challengers and the ninth badge after becoming Leader (the challenges and the player's badge above)
6. Johto and the Gym Leader Convention (replaces the Sevii Islands)
   - [x] Johto maps imported (Olivine to Goldenrod, Ilex, Azalea, Route 40)
   - [x] Border guards, post-game wild levels, convention goodie bag, Johto map and FLY
   - [x] The invitation (mentor), Vermilion-Olivine ferry; Sevii trip removed; Moltres to Victory Road
   - [x] Convention Center (Radio Tower): 18 type halls + Champions' Hall, 127 leaders (Emerald
         Rogue sprites), trophies (hall maps reviewed and kept as they are, Burned Tower and Sinjoh temple included)
   - [x] HGSS music
   - [x] Convention fans around Johto
   - [ ] The rest of Johto's people (random, faithful to HGSS, or quests: to decide)
   - [x] Title screen and credits: "Pokémon Canvas" (`tools/gym_hack/build_title.py`: CANVAS on a
         brush stroke of type colors under the POKéMON logo); the credits open with the hack's own
         (creator, AI disclosure, pret/RHH, HnS, Emerald Rogue, not-affiliated note) before FireRed's staff
7. Regional safari zones and the full Pokédex (post-game, an idea for later)
   - [ ] A safari zone per region (Hoenn, Sinnoh, Unova, Kalos, Alola, Galar, Paldea) where the
         Pokémon past the first 386 can be caught, with puzzles opening sub-areas with legendaries.
         All species through Gen 9 are already in the ROM (the convention and challengers use them)
         and the National Dex already counts 1025; the work is maps, encounter tables (~640
         species, ~30 wild slots per map) and about 70 legendary encounters.
   - [ ] Evolutions into later Pokémon (Magnezone, Sylveon...) turned on in the post-game only, so
         the main game stays 386 (`P_GEN_x_CROSS_EVOS` gates the links); special evolution items
         and conditions obtainable.
   - [ ] Space: about 4.4 MB of the 32 MB is free (October 2026). Ways to make room:
         - Delete the Sevii Islands (unreachable since the ferry change): ~0.5-0.8 MB (159 map
           layouts ~145 KB, 9 Sevii-only tilesets ~70 KB, their events, trainers, scripts, text).
           Needs care: the trainer table, story flags, Celio and the Ruby/Sapphire quest, and the
           Pokémon Center network machine point into them.
         - Drop the Emerald songs the hack never plays (keep the ones a Hoenn safari would use).
         - Build the safaris from tilesets already in the ROM; reuse songs.
         - HnS has map material to start from (its Safari Zone, the Alola islands, Sinjoh Ruins).
         - Music is low priority for Nick (he mostly plays with the sound off, except for big
           battles and nostalgic Kanto areas): the safaris can reuse existing songs everywhere, no
           new instrument sets. If space runs short, music is the first thing to cut (the HGSS set
           is ~0.8 MB: Johto could fall back to fewer songs or FireRed's).
         - Rough costs (measured on Johto): a map layout ~2 bytes per tile (Route 34, 60x90:
           10.5 KB); a tileset ~12 KB (26 KB at most); a safari region of 8-10 maps with 3-4 new
           tilesets ~200 KB without new music.
