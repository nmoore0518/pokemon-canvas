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
- The post-game training trainers: `tools/gym_hack/build_training_houses.py` (the `TRAINERS` list:
  name, class, house, spot, team). Edit it and run it; it rewrites their party entries, ids,
  scripts and map objects.
- All the game's dialogue as two Word files to edit (story and named characters; everyone else):
  `python3 tools/gym_hack/dialogue/build_dialogue_docs.py` -> `build/dialogue/`. Every text gets an
  ID (`tools/gym_hack/dialogue/dialogue_ids.json` keeps them stable); `build/dialogue/dialogue_index.json`
  maps each ID back to its script label or C file:line.

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
- **The basement** (`PalletTown_PlayersHouse_B1F_Frlg`): Dad's workshop, down a stairwell on the
  house's 1F (x 7-9, rows 2-3: the 2F's stairwell blocks; the rug, table and Mom moved a row down,
  the whiteout spot with them). Concrete, pipes, two caged lamps and dark corners; the Ability
  Synthesizer (a belt into a falling stream of glowing liquid, a vat), the Ball Maker (a hopper,
  a slot, a hand crank), the Ball Changer (two pedestals - the balls on them are object events, so
  they jump across - and a console with a lever), a wall switch that's only there with
  `FLAG_BASEMENT_SWITCH_SHOWN` (thrown: `FLAG_BASEMENT_SWITCH_ON`), four loose pages, a workbench,
  lab table, desk, bookshelf, chalkboard, crates and a trash can. Drawn in
  `tools/gym_hack/basement_art.py`, built by `tools/gym_hack/build_basement.py` (its own tileset,
  `gTileset_PlayersBasement`: 379 of 384 tiles used). The machines animate by setting their blocks
  frame by frame (`frames.inc`, generated).
- **Dad's machines** (`src/basement_machines.c`, `include/constants/basement_machines.h`, the basement's
  `scripts.inc`). The wall switch sets the mode once it's there: up = Evil Mode, thrown = Nice Mode
  (`FLAG_BASEMENT_NICE_MODE`, the same flag as `FLAG_BASEMENT_SWITCH_ON`); it asks "Switch machines
  from Evil Mode to Nice Mode?" (or back). Party Pokémon only (`ChoosePartyMon`), never an Egg.
  - **Ability Synthesizer**: 3 Strong Acid (Evil) or 3 Apple Juice (Nice) and a Pokémon in; the screen
    reads REMOVING ABILITIES. / IMPLANTING SYNTHETIC ABILITY.; it comes out with its synthetic ability.
    Evil: friendship to 0 ("in a lot of pain... very unhappy"). Nice: friendship +20
    (`SYNTH_NICE_FRIENDSHIP`; it loved the juice too much to notice). It won't take a Pokémon that
    already has it, or a flagged one (below); the fuel is only used when it runs.
  - **Ball Maker**: a Pokémon of Lv. 50+ in, a Type Ball of its primary type (`GetSpeciesType(.., 0)`,
    so the hack's swapped types count: Pidgeot makes a Flying Ball) out. Evil: GRINDING. EXTRACTING.
    POKéMON ELIMINATED. [TYPE] BALL CREATED. - the Pokémon is gone (its held item goes to the bag;
    it won't take your last one). Nice: SCANNING. EXTRACTING SMALL DNA SAMPLE. - it loses 5 levels
    (`BALL_MAKER_NICE_LEVELS`, moves kept) and comes back.
  - **Ball Changer** (same in both modes): pick a Pokémon, then a Ball from the bag (the bag opens on
    the Poké Balls pocket, `Bag_ChooseBall`); the balls on the pedestals jump across, the Pokémon is
    in the new Ball and the old one goes back in the bag.
  - **Strong Acid / Apple Juice** (`TryGiveWildMonLabItem`, before the usual held-item roll): wild
    Poison Pokémon hold Strong Acid 25% of the time, wild Grass Pokémon Apple Juice 25% (the Applin
    line 50%); a Poison/Grass one rolls for the acid first. Plain items (sell for 100).
- **Synthetic abilities** (`tools/gym_hack/synthetic_abilities.txt` -> `build_synthetic_abilities.py` ->
  `src/data/pokemon/synthetic_abilities.h`; the review page: `synthetic_abilities_page.py`). One
  existing ability per species, a little better than its own. A Pokémon has it when its ability slot
  (`MON_DATA_ABILITY_NUM`) is 3 (`ABILITY_NUM_SYNTHETIC`; 0-1 regular, 2 hidden):
  `GetAbilityBySpecies` / `GetSpeciesAbility` look it up, so battle, the summary screen, evolution
  (it gets the evolved species' one) and form changes all follow it. Not passed on by breeding;
  the Ability Capsule, Ability Patch and the stat-training ability change don't work on it.
  Lines tagged `flag` in the list would lose a form-changing ability (Stance Change, Schooling,
  Disguise, Zen Mode...): those species are in `gSpeciesSyntheticBlocked`, the Synthesizer refuses
  them ("ERROR. ABILITY CANNOT BE REMOVED."), and a synthetic Pokémon that evolves into one (Doublade
  -> Aegislash) keeps that species' own first ability. The Pokédex PDF lists each one ("none" for
  the blocked).
- **Type Balls** (`BALL_TYPE_NORMAL`..`BALL_TYPE_FAIRY` = 28-45, `ITEM_NORMAL_BALL`.. ; only from the
  Ball Maker, can't be sold). 3.5x catch rate on a Pokémon of that type (either of its types), 1x
  otherwise. A Pokémon in a Type Ball of one of its types gets its lowest base stat x1.5 (rounded
  down; every stat tied for lowest; never Shedinja's 1 HP) in `CalculateMonStats`
  (`GetTypeBallStatBoosts`): stats are recalculated when it's caught in one or the Ball Changer
  moves it. Random balls (`BALL_RANDOM`) never pick one. Art from `tools/gym_hack/type_ball_art.py`
  (bag icons with a type emblem; the battle ball is the Poké Ball's recolored), which also draws
  the Strong Acid and Apple Juice icons.
  - The bag's Poké Balls pocket holds 46: 28 in SaveBlock1's bag, 18 more at the end of SaveBlock2
    (`bagPokeBalls2`; older saves have zeros there, i.e. empty slots).
- **Dad's story** (`data/scripts/dad_story.inc`, `src/dad_story.c`, `include/constants/dad_story.h`;
  the text in `tools/gym_hack/story/dad_story.txt` -> `build_story_text.py` -> `data/text/dad_story.inc`,
  wrapped to the box by pixel width). `VAR_DAD_STORY` (0x40DB) steps through it:
  - **Send-off** (1): set after the mentor leaves Oak's lab; Mom waits outside the lab door the first
    time the player walks out, gives her speech and walks home.
  - **Nugget Bridge** (2-4): joining Team Rocket is forced (NO gets "Come on, join us!" and the
    question again); after the Move Extractor the rival walks up (`LOCALID_ROUTE24_RIVAL`), says Mom
    has something, and the player is warped home; Mom gives Dad's Type Ball (the player's gym type,
    `DadStory_GetGymTypeBall`) and asks about the Move Extractor.
  - **Silph Co. 11F** (4-6): no President or secretary; a Strong Acid ball (x3) by the door. Giovanni
    gives the Master Ball and the Multivitamin, Mom runs in, takes the player home ("Why were you
    with him?"), moves the bookcase (`DadStory_EventScript_HouseOnLoad` hides the stairwell with
    `setmetatile` until `FLAG_BASEMENT_REVEALED`) and shows the basement. Its four pages and Dad's
    Notebook open as full pages (`ShowNotePage`, `NOTE_PAGE_*`: A for the next page, B closes).
  - **Viridian Gym** (6-8): after the first battle Giovanni hands over the badge, TM26 and the silver
    cap at once ("You're just like your father..."), then his last battle (`TRAINER_GIOVANNI_DAD_FINAL`,
    no whiteout: losing heals the party and he waits for a rematch). His team is synthesized in
    `CreateNPCTrainerParty` (`DadStory_SynthesizeParty`: synthetic abilities, Type Balls of their
    types). Dad's ace in his story depends on the player's type (`sDadsAce`). Mom's Chansey makes him
    vanish; Mom gives **Dad's Chansey** (`SPECIES_CHANSEY_DAD`: Chansey with 100 Atk / Sp. Atk, Triage,
    Normal Ball, Lv. 50, perfect IVs, no EVs, OT DAD, can't breed, evolve or be traded) holding
    **Dad's Notebook** (a key-ish item: can't be sold, tossed, stolen or knocked off; read it from the
    bag). The basement switch appears (`FLAG_BASEMENT_SWITCH_SHOWN`) and Giovanni's approval is set.
  - After VIRIDIAN (8-10): Mom owns up to what she knew, then the player is warped home to the
    CHANSEY statue, where Mom and Oak remember Dad; Oak explains the statue (the town put it up for
    him) and that the League counts Viridian's approval as given (`DadStory_EventScript_Memorial`,
    `DAD_STORY_REMEMBERED`; an older save at `DAD_STORY_DONE` gets it the next time it enters Pallet).
    From then on ARCHER is the Viridian Gym's leader (`LOCALID_VIRIDIAN_GYM_ARCHER`): he tells the
    player where he came from once (`DAD_STORY_MET_ARCHER`): he's on probation with the League; Dad
    brought him into Rocket's labs and he let Dad walk out the night he left; Rocket took Dad's
    unfinished crystal and Archer finished it - the TERA ORB, which he gives the player - and he asks
    to be forgiven (YES/NO). A save that met him before the Orb gets that part on its next visit.
    Mom's talk at home changes after Viridian. Dad's Notebook has a third page: the crystal.
  - Saves already past a step skip its scene (e.g. past Silph: the stairs are simply there).
  - Giovanni isn't one of the famous gym challengers any more.
  - In the Gym Leader Convention, ARCHER (a former Team Rocket executive) holds Giovanni's place in
    the Ground hall with Giovanni's old team: the League let him take over the Viridian Gym.
- **Terastallization** (`src/gym_tera.c`, `include/gym_tera.h`, `data/scripts/gym_tera.inc`):
  - The TERA ORB (from Archer, above) never needs charging (`B_TERA_ORB_ALWAYS_CHARGED`); one
    Pokémon a battle, as usual.
  - The player's Pokémon Terastallize into the gym's type (`VAR_PLAYER_TYPE`) in every one-type
    battle: the gym leaders and rival (`GymHack_IsRuleBattleTrainer`), the Elite Four and Champion,
    and TRAINER_GYM_CHALLENGER battles set up as one-type (`GymChallenge_IsOneTypeBattle`: the gym's
    challengers and mentor, guest-leader challengers, Oak). Decided when the trainer's party is made
    (`GymTera_SetUpTrainerParty`, from `CreateNPCTrainerParty`); `GetBattlerTeraType` asks
    `GymTera_PlayerBattleType` for the player's side (not in tests).
  - Everywhere else a Pokémon uses its own Tera type (its "free" Tera type), but only if that's a
    mastered type (`FLAG_GYM_TYPE_MASTERED` + type) or the gym's type; otherwise the gym's type
    (`GymTera_FreeType`). Archer sets it, from the gym's type and the mastered ones, once the player
    has mastered one. Oak's mastery visit explains it (`GymSwitch_EventScript_OakTeraTip`).
  - The summary's SKILLS page: the RIBBON box is the TERA box (`tools/gym_hack/tera_label_art.py`
    redrew the label), showing the free Tera type once the player has the Orb.
  - Opponents only Terastallize once the player has the Orb, and only the important ones, with their
    ace (highest level; the later slot on a tie): in one-type battles (the rival, leaders, Elite Four,
    Champion, the gym's famous challengers and mentor, Oak) into a type that resists the gym's type
    (one of its own if it can, else one it has moves of, better if it hits the gym's type hard:
    `CounterType`); in free battles (the tournament, Bill, the Convention: `GymChallenge_SetTeraMode
    (GYM_TERA_TEAM)`) into the type its team is built around. Everyone else stays `TYPE_MYSTERY`.
- **Outfits** (`src/outfits.c`, `include/constants/outfits.h`, `data/scripts/outfits.inc`; colors from
  `tools/gym_hack/outfit_art.py`). An outfit is a set of colors for the player's own sprites, so it
  covers every one of them (walking, running, bike, surf, fishing, field moves, reflections and the
  battle back pic) for both boy and girl. The player's overworld palettes point at RAM
  (`gOutfitPlayerPalettes`, refreshed from the outfit whenever they're looked up) and
  `GetTrainerBackPicPalette` hands the back pic the outfit's. `VAR_PLAYER_OUTFIT` (0x40DC) is the one
  worn; `VAR_OUTFITS_UNLOCKED` (0x40E5) / `_2` (0x4091) which are unlocked. The wardrobe (the green
  cabinet in the player's room, x 4-5) lists the ones the player has, and the player shows each as
  it's picked (`DYN_MULTICHOICE_CB_OUTFIT`).
  - CASUAL CLOTHES: always. ROCKET UNIFORM: Dad's, in the basement crates (`Outfits_EventScript_BasementCrates`).
  - The 18 gym uniforms (the clothes in the type's Type Ball colors): the player's type's comes with
    the mentor's first gift after Mt. Moon (older saves get it at the wardrobe), and the first change of the
    gym's type (after a mastery) gives every type's (`Outfits_EventScript_AllGymUniforms`). The lines are placeholders.
  - Chest marks: every outfit but the casual clothes has a 4x3 mark on the chest (the Rocket R, a
    gym uniform's type emblem in the Type Ball's emblem color, on a patch of the jacket's color),
    drawn on the walking and running frames that face the screen. Those frames are their own sheets
    (`graphics/object_events/pics/people/outfits/`, generated), which the player's walking sprite
    uses (`Outfit_ObjectImages`, wherever the game sets an object's frames). The mark is color 13,
    so the player's own sheets keep their bag in one shade (14).
  - The crate scene doesn't say whose uniform it is (the player doesn't know about Dad yet).
  - New outfits go at the end of `OUTFITS` (saves keep the numbers); run the script after changing it.
- **Renaming from the summary**: the first time the player talks to the Name Rater (Lavender Town),
  he says they can rename a Pokémon any time (`FLAG_NAME_RATER_TIP`); from then on the summary
  screen's first page shows RENAME (A) for the party's own Pokémon (not traded ones, Eggs or the
  PC's: the expansion's `P_SUMMARY_SCREEN_RENAME`, gated in `ShouldShowRename`).
- **The opening** skips the controls guide and the Pikachu pages (`CANVAS_SKIP_CONTROLS_GUIDE` in
  `src/oak_speech.c`): New Game goes straight to Oak.
  Houses take a roof color (`house(x, y, "blue")`: red, blue, green or purple). The Chansey statue,
  benches and street lamps are pixel art in `tools/gym_hack/pallet_art.py` (`statue()`, `bench()`,
  `lamp()` in the layout); the script adds their tiles and palettes to the Pallet Town tileset.
- **Type menu order = difficulty.** Oak's list runs from the type with the strongest possible
  team to the weakest: the highest total base stats of six different species with that primary
  type (Gen 1-3; of the legendaries only Kanto's count: Johto's and Hoenn's are post-game, in the
  Exotic Safari). Oak says so before the menu, and picking one of the last four
  (`GYM_HACK_HARD_STARTERS_FROM`) gets a "bold choice" warning. A test keeps the order right.

  | # | Type | Best six (BST total) |
  |---|---|---|
  | 1 | Psychic | 3306 (Mewtwo, Mew, Espeon, Gardevoir, Alakazam, Hypno) |
  | 2 | Fire | 3258 (Moltres, Arcanine, Typhlosion, Charizard, Blaziken, Flareon) |
  | 3 | Normal | 3255 (Slaking, Snorlax, Blissey, Porygon2, Ursaring, Tauros) |
  | 4 | Water | 3220 (Milotic, Kingdra, Gyarados, Swampert, Lapras, Feraligatr) |
  | 5 | Grass | 3090 (Sceptile, Exeggutor, Venusaur, Meganium, Vileplume, Victreebel) |
  | 6 | Electric | 3080 (Zapdos, Jolteon, Ampharos, Electrode, Electabuzz, Raichu) |
  | 7 | Rock | 3075 (Tyranitar, Omastar, Kabutops, Golem, Cradily, Armaldo) |
  | 8 | Poison | 3002 (Crobat, Nidoqueen, Nidoking, Muk, Weezing, Swalot) |
  | 9 | Bug | 2970 (Shuckle, Scyther, Scizor, Pinsir, Heracross, Forretress) |
  | 10 | Steel | 2955 (Metagross, Aggron, Steelix, Skarmory, Lairon, Metang) |
  | 11 | Ice | 2905 (Articuno, Walrein, Glalie, Jynx, Piloswine, Sealeo) |
  | 12 | Ground | 2885 (Flygon, Donphan, Claydol, Rhydon, Sandslash, Gligar) |
  | 13 | Flying | 2841 (Aerodactyl, Pidgeot, Xatu, Dodrio, Swellow, Noctowl) |
  | 14 | Dragon | 2830 (Salamence, Dragonite, Altaria, Shelgon, Dragonair, Dratini) |
  | 15 | Fighting | 2799 (Machamp, Hariyama, Primeape, Hitmontop, Hitmonlee, Hitmonchan) |
  | 16 | Dark | 2745 (Umbreon, Houndoom, Absol, Sneasel, Mightyena, Murkrow) |
  | 17 | Ghost | 2560 (Gengar, Dusclops, Banette, Misdreavus, Haunter, Gastly) |
  | 18 | Fairy | 2206 (Clefable, Granbull, Togetic, Clefairy, Snubbull, Togepi) |
- **Johto's and Hoenn's legendaries** (Raikou, Entei, Suicune, Lugia, Ho-Oh, Celebi, the Regis,
  Latias, Latios, Kyogre, Groudon, Rayquaza, Jirachi, Deoxys) are no longer in Kanto's wild
  tables (their slots went to the table's neighbouring Pokémon); they go in the Exotic Safari
  (Ho-Oh, Groudon and Rayquaza stay gym-only). Kanto's legendaries stay where they are.
  `check_obtainable.py` checks the other 370 can be caught before the post-game (Kanto only: the
  unreachable Sevii Islands and post-game Johto don't count).
- **Sevii-only Pokémon moved to Kanto**: Lunatone and Solrock (Mt. Moon 1F, Lv 10, 4% each),
  Spinda (Route 3, Lv 6, 4%), Trapinch (Diglett's Cave, Lv 20, 5%), Ponyta (Route 7, Lv 20, 5%),
  Yanma (Route 13, Lv 25, 5%), Slugma (Pokémon Mansion 2F, Lv 28, 4%), Skarmory (Route 23, Lv 40, 4%).
  The wild planner (`planning/wild-pokemon-planner.xlsx`) covers Kanto and Sevii only; Johto's
  tables come from `import_johto.py` and the Exotic Safari's from `build_exotic_safari.py`.
- **Post-game training houses** (`tools/gym_hack/build_training_houses.py`): after the first Hall
  of Fame (`FLAG_HIDE_POSTGAME_GOSSIPERS`), 18 trainers, two in every town but Pallet (Viridian
  Lv 18-22 rising to Cinnabar Lv 57-61), and 6 in the **Pallet Training Hall** (Lv 63-82: a new
  house in the clearing east of Oak's lab, the Saffron dojo's layout, with a nurse who heals).
  Each asks before battling and can be fought any number of times (their trainer flag is cleared
  first); no type rule. Most teams: Chansey/Blissey/Audino (plain attacking moves, no Soft-Boiled)
  plus a mix of types. Every town (and the hall, three of six) also has a trainer whose whole team
  every attacking type can hit: no Normal, Ghost, Flying, Ground, Steel, Dark or Fairy types and no
  immunity abilities (Levitate, Flash Fire, Water Absorb...; abilities are set where a species has
  one in slot 1 or 2). The script checks this from the species data and refuses to write otherwise.
  Lady/Gentleman/Beauty classes, so they pay well.
- **Lucky Egg**: at the Chansey statue in Pallet Town (the statue's script gives it; a hidden item
  on the same tile, after the statue's signs, is there for the Itemfinder). **Amulet Coin**: an
  item ball on Pokémon Mansion 3F (besides Oak's aide's on Route 16).
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
  description; you choose how many (UP/DOWN one at a time, LEFT/RIGHT ten, up to 99; floors and
  wall styles are bought once), and they go to the gym's storage. SELL DECORATIONS sells what's in
  storage (not placed) for half price, in any number; PUT ALL AWAY sends every placed piece back to
  storage (floors and the wall style stay). DECORATE starts edit mode: walk anywhere in
  the gym (you can walk through things while decorating) and the selected piece is shown in front
  of you, whole, tinted red where it can't go (anywhere between the side walls, from the back wall
  down to the bottom row) (`ShowRedOverlay`: see-through sprites), with the
  kept-clear area (entrance, statues, PC) shaded. A places it, START turns it, SELECT picks up
  the one in front (it's then the piece in hand, the same way round, to put down elsewhere; L/R
  leaves it in storage), L/R switch pieces, B finishes. Nothing can go on the walls, the doormat, the
  statues or the PC, or on a wall's front face (except another wall), and if you've boxed yourself
  in when you finish, you're put back at the PC. Invisible walls are shown while decorating. The
  help box sits at the bottom of the screen.
  - **Turning** (START): 51 pieces have drawn sides and backs (`tools/gym_hack/decor_art_turned.py`:
    `TURNS` lists each piece's views in order: a drawing, the front flipped, or a drawing flipped
    for the other side). A turned piece takes its turned shape (the couch is 3x2 from the front,
    2x3 from the side). Round and symmetric things (plants, barrels, trophies...) don't turn. The
    two chairs turn into each other. The way a piece faces is saved in bits 0-1 of its placement's
    `room` byte (old saves: 0, the front).
  - **Walls and pool water join up**: a GYM WALL is one square; its top joins the walls (and the
    back wall) around it, and its front face is drawn on the square below its bottom edge (solid,
    and only another wall can go there). POOL WATER's edges join the same way, so a pool of any
    shape has a rim only round its outside. Older saves' 1x2 walls become one square plus its face.
  - **Floors** (FLOORS category, each bought once): GYM TILES (the gym's own, free), WOODEN, STONE,
    RED CARPET, SAND (badge 1), METAL GRATING (badge 3), GRASS (badge 4), STEEL (badge 6), LAVA
    (badge 7: nobody can walk on it, and it can't go in the kept-clear area), ICE (after the
    Champion). Pick one in DECORATE (or switch to it with L/R): A paints the square in front, holding
    A paints every square you face as you walk, and SELECT fills the whole area of the floor in
    front. Floors go under decorations (flat ones and the gaps round others show it). Each square's
    floor is saved (4 bits each).
  - **Wall styles** (WALLS category, each bought once): GYM, BRICK, WOODEN, STONE (badge 1), HEDGE
    (badge 4), STEEL (badge 6). Choosing one in DECORATE makes it the style of every wall: the back
    wall, the side walls (no wall along the bottom, as in the other gyms) and the walls you place,
    which join up with them. Pieces an older save had in the side columns go back to storage. Floor and wall art: `tools/gym_hack/gym_room_art.py`.
  - Catalog: `DECORATIONS` in `tools/gym_hack/build_gym_decor.py` (name, price, unlock flag,
    description, and the blocks that draw it). Run `python3 tools/gym_hack/build_pallet_gym.py`
    after changing it (then `git checkout data/maps/PalletTown_Gym_Frlg/map.json
    data/layouts/layouts.json`: it rewrites them without the later objects). New entries go at the
    end (saves store decorations by position).
  - Unlocks: an entry's `unlock` is a flag, usually a badge (`FLAG_BADGE02_GET` = Misty's water).
    It's just a flag, so any trainer's defeat flag works too.
  - **How the room is drawn** (`src/gym_room.c`): the gym's tileset can't hold every decoration
    on every floor, so the room is put together as it's drawn. Each square's look (its floor, a
    decoration in the way it faces, a wall or water joined to its neighbors, a wall's face) becomes
    one of the gym tileset's 384 blocks, which are kept in RAM (`gGymRoomMetatiles`: the tileset's
    `metatiles` point there), and the art it uses is copied into free video memory (915 tiles).
    Identical squares share a block, art already in video memory is reused, and when either runs
    low the room is put together afresh with only what's in it. The art bank (1,556 tiles), the
    looks and the catalog are generated into `src/data/gym_decor.h`. Art goes into video memory
    straight away while decorating; when the gym loads, after the tileset's own (queued) copy is
    done (`GymRoom_TilesetAnim`, the gym tileset's animation callback). A piece that would need
    more than there's room for in one room is refused. A room with every decoration in the game
    placed (128 pieces) still fits.
  - Furniture comes from the indoor tileset, Viridian gym pieces (spinners, stop tiles, statues)
    as they are, and pieces from other gyms (Cerulean water, the Vermilion trash can) redrawn in
    the shared palettes. Spinners and water keep their behavior.
  - Save data: what you own, where it's placed, each square's floor and the wall style live where
    Emerald keeps secret bases (`gSaveBlock1Ptr->secretBases`, 3,200 bytes that FireRed never
    uses; `struct GymDecorSaveData`, 2,860 bytes, then the gym challenge's records).
    `src/gym_decor.c` marks it with a magic number and clears it on a new game or an older save.
    Record mixing never touches it.
  - Shop and storage are split into categories: GENERAL, TRAINERS and one per type (only the
    categories with something in them are listed), and FLOORS and WALLS.
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
  - Capacity: up to 256 kinds of decoration (255 of each in storage, 168 kinds so far, floors and
    wall styles included), and 560 placed pieces (more than the room's ~500 free squares).
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
- **Lucky Egg and Amulet Coin levels** (`src/item_upgrades.c`): both have a level, 1-10, kept
  for the player (`VAR_LUCKY_EGG_LEVEL` 0x4092, `VAR_AMULET_COIN_LEVEL` 0x4093; 0 counts as 1).
  Each level adds 0.5x: the Lucky Egg 1.5x EXP at level 1 up to 6x at 10, the Amulet Coin 2x
  prize money up to 6.5x (battle prize money and Pay Day). The bag's description shows the level.
  How to earn upgrades is still to be decided: scripts call `special ItemUpgrade_Upgrade` with
  VAR_0x8004 = the item (VAR_RESULT = the new level, 0 if maxed) and `ItemUpgrade_BufferLevel`.
- **Stat training items** (`src/stat_items.c`, `data/scripts/stat_items.inc`), all used up when
  used, all chosen from the bag and then a Pokémon:
  - Silver Bottle Cap (50,000): pick one stat; its IV becomes 31.
  - Gold Bottle Cap (150,000): every IV becomes 31.
  - Ability Capsule (100,000): switch between the species' two regular Abilities (the hidden
    one is the Ability Patch's job: 250,000, sold at the same mart from the same badge).
  - Cilantro (40,000): pick a nature from all 25. It sets the stat nature (like a mint), and the
    summary screen shows the new nature.
  - Multivitamin (100,000): an editing screen for all six EVs. Left/Right move 4 at a time, L/R
    set none or the most allowed, and it never lets a stat pass 252 or the total pass 510. It
    shows what each stat would be. Cancelling, or changing nothing, doesn't use it up.
  - Sold only at the Pallet Town Mart (`GymHack_PalletMart`): Silver Bottle Cap after Erika's
    badge, Ability Capsule and Ability Patch after Sabrina's, Cilantro after Blaine's, Gold Bottle Cap and
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
    Ability Capsule, Cilantro, Multivitamin or a Master Ball), and 1 win in 150 a MOVE EXTRACTOR.
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
    IVs, EVs, held items, competitive moves and the smartest AI, no healing items; one level
    above the player's average (`GYM_OAK_FINALE_MARGIN`), never below 65. Champion battle music.
    `GymChallenge_GenerateOakFinale` in `src/gym_challenge.c`.
  - Losing sends the player home (no money lost), and the scene plays again when they step out.
  - Winning: Oak apologizes and hands over the gym (`FLAG_PALLET_GYM_AWARDED`, which opens the
    gym challenges and the badge pedestal), the League makes the player's badge official, and Oak
    introduces his aide and explains the challenge system before leaving.
  - There's no National Pokédex scene: the Pokédex is the National one from the moment Oak gives
    it (starters and wild Pokémon come from every region), and the save menu, continue screen and
    trainer card count every species caught. Older saves are switched over on their next step
    (`GymDecor_CheckNewUnlocks`).
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
    Normal→Brock, Steel→Surge, Bug→Koga, Fairy→Sabrina, Dark→Agatha, Flying→Lance; Ground goes to
    Bruno (Giovanni, the villain of Dad's story, mentors no one). A mentor greets the player as
    someone they know: a Gym Leader mentor's gym battle opens with their own line
    (`tools/gym_hack/story/mentor_story.txt`, `GymCandidate_BufferMentorPreBattle`), and an Elite
    Four mentor's line replaces their usual intro.
  - **The rival** is the other candidate: his Route 22, Cerulean, S.S. Anne, Pokémon Tower and
    Silph Co. lines are about the race for approvals and the Pallet gym.
  - **Oak's doubts** grow quietly: lines in his lab as approvals come in (picked the gym's floor
    tiles himself; "I was quite the TRAINER in my day"; checking the gym's lights at night), Daisy
    noticing him visiting the gym at night in his old cap (4+ approvals), and the rival
    mentioning that Gramps keeps asking about the gym.
  - **Trainers who join**: one trainer per type on the road asks to join your gym after you beat
    them (and when you talk to them again, until they do). They go to the gym PC's TRAINERS
    storage and are placed like the bought trainers; in practice battles they use a team of their
    own favorite type. While one is placed in the gym, they're gone from their spot on the road
    (`GymCandidate_IsPlacedRecruitScript`, checked when objects spawn); put them back in storage
    and they return. Andrea (Normal, Route 8), Hitoshi (Fighting, Dojo), Perry (Flying, Route
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
  - More are ultra rare prizes: 1 gym challenge win in 150 (`GYM_CHALLENGE_EXTRACTOR_ODDS`; the
    challenger leaves it instead of the usual item), and 1 tournament match won in 100
    (`TOURNEY_EXTRACTOR_ODDS`; Bill's simulator too), in `include/constants/gym_hack.h`.
  - Three for sure, from the Team Rocket grunt on Nugget Bridge (`VAR_MOVE_EXTRACTOR_STAGE`):
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
  (`GymLegend_TryCaveEncounter`, in `TryGenerateWildMon`) until it's caught. (Cerulean Cave's
  guard steps aside after the Hall of Fame - FireRed only moved him from the Sevii Islands, which
  this hack can't reach, so before v83 the cave was shut for good.) Once it's caught (on
  the stage or in the cave), the rival comes to the gym to scoff that the player needs a
  legendary to win. `VAR_GYM_LEGEND_STATE` (0x40FE, `GYM_LEGEND_*`), `FLAG_GYM_LEGEND_APPEARED`/
  `CAUGHT` (0x0C3/0x0C4). Its overworld sprite is the
  expansion's Pokémon object (`VAR_OBJ_GFX_ID_C`).
  Normal: Meloetta, Fighting: Koraidon, Flying: Tornadus, Poison: Eternatus, Ground: Groudon,
  Rock: Diancie, Bug: Genesect, Ghost: Giratina, Steel: Dialga, Fire: Ho-Oh, Water: Palkia,
  Grass: Shaymin, Electric: Miraidon, Psychic: Cresselia, Ice: Glastrier, Dragon: Rayquaza,
  Dark: Darkrai, Fairy: Xerneas. Navel Rock's Ho-Oh (an event map) never appears now.
- **Changing the gym's type** (`src/gym_switch.c`, `data/scripts/gym_switch.inc`): after the gym
  is the player's, a streak of 15 wins (`GYM_SWITCH_FIRST_STREAK`; 10 for every later type) masters its type. The streak's last
  battle is the player's mentor with their full Gym Leader Convention team, at least 3 levels above
  the player's average, no healing items (`GYM_CHALLENGER_BOSS`; the aide announces them).
  Beating them earns a new type, saved as a "ticket" (`switchTickets`/`switchesEarned` in
  `GymChallengeSave`, `FLAG_GYM_RUN_MASTERED` 0x0DE), and the player can keep going: after each win
  the aide offers KEEP GOING / CALL IT A DAY / TAKE A BREAK. When the streak ends (a loss, or
  calling it a day), Oak walks in, says the player has mastered the type, and offers a new one:
  YES opens a menu of types the gym hasn't had, the badge changes (and the pedestal), and the new
  type's legendary arrives at once, as in the finale. NO keeps the ticket: Oak offers it again in
  his lab ("Have you changed your mind?"). In the lab Oak also switches the gym back to any type it
  has had, free (marked {STAR} in the menu; `FLAG_GYM_TYPE_OWNED` 0x100 + type); a new type chosen
  there sends its legendary to the gym for the next visit (`GYM_LEGEND_PENDING`; the aide sees it
  arrive). Before any mastery, the aide and Oak say what streak it takes. Each type's legendary is
  kept: caught ones stay caught, and one that had appeared but wasn't caught goes to Cerulean Cave
  when the gym changes type (`FLAG_GYM_LEGEND_CAUGHT_TYPE` 0x114 + type,
  `FLAG_GYM_LEGEND_LOOSE_TYPE` 0x128 + type); the cave's 1% encounter picks one of the loose ones.
  The rival's type never changes. The gym's type can't change while a streak is going (taking a
  break keeps it going): Oak in the lab says to come back once it's over, so a switch always
  follows a finished streak (a loss, or calling it a day after mastering).
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
| `tools/gym_hack/build_basement.py`, `basement_art.py` | The basement under the player's house: its art, tileset, map, the machines' frames, the stairwell on 1F |
| `data/scripts/dad_story.inc`, `src/dad_story.c`, `tools/gym_hack/story/` | Dad's story: its scenes, note pages, Dad's ace, Giovanni's last team, Dad's Chansey; the story text and its generator |
| `src/basement_machines.c`, `include/basement_machines.h`, `include/constants/basement_machines.h` | Dad's machines (Synthesizer, Ball Maker, Ball Changer), Type Ball effects, Strong Acid / Apple Juice on wild Pokémon |
| `tools/gym_hack/synthetic_abilities.txt`, `build_synthetic_abilities.py`, `synthetic_abilities_page.py` | Each species' synthetic ability, the generated table, the review page |
| `tools/gym_hack/type_ball_art.py` | Type Ball, Strong Acid and Apple Juice art (`python3 tools/gym_hack/type_ball_art.py sheet.png`) |
| `tools/gym_hack/pallet_art.py` | Chansey statue, bench and lamp art |
| `tools/gym_hack/build_pallet_gym.py`, `data/maps/PalletTown_Gym_Frlg/` | The gym inside and Oak's gym scene |
| `tools/gym_hack/build_gym_decor.py` | Decoration catalog and the gym tileset that draws it |
| `src/gym_decor.c`, `include/gym_decor.h`, `data/scripts/gym_decor.inc` | Gym PC, decoration shop, edit mode, save data, type unlocks, practice trainer spots |
| `tools/gym_hack/decor_art.py` | Pixel art for the type decorations and trainer icons (`python3 tools/gym_hack/decor_art.py sheet.png`) |
| `tools/gym_hack/decor_art_turned.py` | Sides and backs of the pieces that turn (`python3 tools/gym_hack/decor_art_turned.py sheet.png`) |
| `tools/gym_hack/gym_room_art.py` | Floors, wall styles and pool edges |
| `src/gym_room.c`, `include/gym_room.h` | Puts the gym's room together: blocks in RAM, art in video memory, floors, joined walls and water |
| `data/scripts/gym_practice.inc` | Practice trainer battles |
| `src/gym_candidate.c`, `data/scripts/gym_candidate.inc` | Candidacy: approvals, Guest Leader battles, mentors and their gifts, Oak's asides, trainers who join |
| `tools/gym_hack/type_item_art.py`, `test/battle/gym_hack/type_items.c` | The mentors' type items (and the Palm Pilot icon): icons and battle tests |
| `data/scripts/palm_pilot.inc`, `tools/gym_hack/build_heal_pad.py` | The Palm Pilot and the Pokémon Center heal pads |
| `src/gym_legend.c`, `data/scripts/gym_legend.inc` | The legendary of the player's type in their gym |
| `src/gym_switch.c`, `data/scripts/gym_switch.inc` | Mastering a type and changing the gym's type (Oak, the mentor battle, the lab menu) |
| `tools/gym_hack/build_exotic_safari.py`, `data/maps/ExoticSafari_*` | The Exotic Pokémon Safari: Fuchsia door, the hall of region doors (plaques drawn by the script), the zones and their Pokémon |
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
| `src/leader_academy.c`, `src/team_builder.c`, `data/scripts/leader_academy.inc` | The Leader Academy: BP, shops, candidates, instructors, rentals, the team builder |
| `tools/gym_hack/build_academy.py`, `build_academy_data.py`, `academy_text.py` | Its maps (campus, Route 11's gap), battle data (`src/data/academy.h`) and lines |
| `.github/workflows/build-rom.yml` | Automatic ROM builds |

## Building locally

```
make firered -j$(nproc)                                   # -> pokefirered.gba
make check BUILD=firered TESTS="GymHack" -j$(nproc)       # run this hack's automated tests
python3 tools/gym_hack/render_map.py <MapFolder> out.png  # preview a map
```

## Balance pass (October 2026, from the bot playthrough)

- [x] **No battle items in post-game battles**, to mirror competitive play: Oak's battle for the
      gym, the mastery mentor, the Elite Four rematches and the Champion (first and rematch, all
      types: `tools/gym_hack/gen_rival_teams.py`). Random/famous challengers, the convention and the
      tournament never had any. The story's trainers keep theirs.
- [x] **At most one legendary on a random challenger's team**, on `GYM_CHALLENGER_LEGEND_PERCENT`
      (40%) of teams (it was any slot, ~1 in 7: ~0.7 a team, teams of 2-3).
- [x] **First mastery streak 15** (`GYM_SWITCH_FIRST_STREAK`; later types stay 10).
- [x] **Oak's battle for the gym at average + 1** (`GYM_OAK_FINALE_MARGIN`; was + 5: the bot went
      0 for 4 at an even 68 vs 73).
- [x] **The Champion**: no Full Restores (the IVs stay). After two losses to him in a row
      (`VAR_CHAMPION_LOSSES`, 0x4094, counted in his room), the League's door guard in the Indigo
      Plateau lobby offers the way straight to him: no Elite Four again.
- [x] Fixed on the way: the door guard blocked the League ("Lorelei is absent") whenever the
      National Dex was on, as in FireRed before the Sevii Islands. With the National Dex from the
      start (v92), the League was shut for every new save. The door is always open now.
- Confirmed as intended: gym challengers draw from Gen 1-3 (Kanto trainers); convention teams
  are each trainer's canonical team (their own region's Pokémon); the tournament draws from the
  convention and the Gauntlet's characters (every region).

## Quality of life (v99-v100)

- [x] **TMs are reusable** (`I_REUSABLE_TMS`): like HMs, never used up, and the mart only sells
      one of each.
- [x] **A ten-pocket bag with room for every item** (v100; v99 had five bigger pockets): ITEMS
      (evolution and training items, valuables, fossils...), MEDICINE, POKé BALLS, BATTLE ITEMS,
      HOLD ITEMS, MEGA STONES (in the order of their Pokémon), BERRIES, TMs & HMs, TOOLS (key items
      that do something: bikes, rods, the Palm Pilot, the HM tools...; registrable to SELECT) and
      KEY ITEMS. The item data still has FireRed's five pockets; `GetItemPocket` (`src/item.c`)
      sorts ITEMS by `sortType` and key items by whether they have a field use (Z-Crystals go with
      the hold items). Every pocket holds one of everything that can go in it
      (`test/gym_hack.c` adds one of every item). The save has no single place big enough, so a
      pocket can come in two pieces (`BagPocket.splitAt`/`itemSlots2`, behind
      `BagPocket_GetSlotData`/`SetSlotData`), and the pockets fill: SaveBlock1's old bag bytes (Key
      Items, Tools, Balls, Berries), SaveBlock3 (TMs, Mega Stones, most Items), SaveBlock1's
      `pokeblocks` (Medicine) and `berryTrees` (Hold Items, the rest of the Items: there are no
      berry trees in Kanto, so `BerryTreeTimeUpdate` isn't called) and 116 bytes added at the end of
      SaveBlock1 (Battle Items). `Bag_UpgradeLayout` moves a FireRed-layout (v98) or v99 bag over
      on loading (`SaveBlock3.bagLayout`: 0xB3 now). Long pocket names print in the narrow font;
      the ten pocket squares are drawn over the background's five.
  (v99's bag, for the record: every Key Item (90), Poké Ball (28), TM/HM (108) and Berry (68), and 297 kinds of items
      (was 30/30/16/64/46). Since v99 SAVE_LINK writes the whole slot: every sector carries a
      piece of SaveBlock3.)
- [x] **The Leader Academy on the Town Map and for FLY** (see the academy above), the mart
      student out of the shelf.

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
   - [x] Changing the gym's type: mastery streak (20, then 10) ending with the mentor, Oak's offer, free switch-back in the lab, each type's legendary
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
         brush stroke of type colors under the POKéMON logo; Smeargle in Charizard's place; no flames); the credits open with the hack's own
         (creator, AI disclosure, pret/RHH, HnS, Emerald Rogue, not-affiliated note) before FireRed's staff
7. Regional safari zones and the full Pokédex (post-game, an idea for later)
   - [x] The Exotic Pokémon Safari's entrance: a second door on Fuchsia's Safari Zone building (sign:
         "NEW! EXOTIC POKéMON SAFARI"), a hall with a door per region (Johto, Hoenn, Sinnoh, Unova,
         Kalos, Alola, Galar, Paldea; names on plaques above), open to Gym Leaders only; shut doors
         say "COMING SOON". Normal wild battles, the player's own balls (no Safari Balls or step
         limit). `tools/gym_hack/build_exotic_safari.py` builds it all (ZONES lists the open zones).
   - [x] The Paldea zone (Nick's planner, `tools/gym_hack/paldea.py`): eight maps linked by
         walking off an edge (a trigger and a fade), doors and ladders. The MEADOW (the hub, from
         the hall's door; normal/fairy) -> west the FOREST (Tagtree Thicket; bug/grass) -> south the
         DESERT (Asado: Route 111's desert, its metatiles copied into a FireRed tileset,
         `convert_emerald`; sandstorm; ground/rock) -> the Desert Ruins' door -> the CRATER
         (Area Zero: Cerulean Cave 1F, ancient paradoxes) -> ladder -> the CRATER DEPTHS (B1F,
         future paradoxes); meadow -> east the LAKE (Casseroya; water, Chi-Yu's island) -> north
         the SNOWY MOUNTAIN (Glaseado: HnS's Mt. Silver summit plus a snowfield; its snow has
         encounters and footprints; ice); meadow -> north the ACADEMY (the Pokémon Mansion's 1F;
         "indoor" Pokémon: Gimmighoul, Poltchageist, Varoom...). Wild Pokémon Lv 48-58; the
         starters are 1% slots (Sprigatito meadow/forest, Fuecoco desert, Quaxly lake and Super
         Rod). Pre-evolutions come at the same high levels (nothing needs breeding).
   - [x] Paldea's legendaries (Lv 70, `FLAG_EXOTIC_LEGEND_*`): Ogerpon (meadow), Wo-Chien
         (forest), Ting-Lu (desert), Chi-Yu (lake island), Chien-Pao (summit), Terapagos and
         Pecharunt (depths) stand in one place. A battle that doesn't catch one makes it disappear
         for the rest of the visit (a temp flag); it's back the next time the area loads. Each waits
         for something first (until then, talking to its spot gives a hint):
         - the Treasures of Ruin: their 4 stakes pulled (purple Wo-Chien, yellow Chien-Pao, green
           Ting-Lu, blue Chi-Yu; 16 stakes, two in each area, never in the legendary's own;
           `STAKES` in paldea.py; sprites by `tools/gym_hack/exotic_safari_art.py`)
         - Ogerpon: its 3 masks found (Wellspring on the lake's island, Hearthflame in the desert,
           Cornerstone in the crater; the player keeps them for its forms)
         - Pecharunt: the Loyal Three caught
         - Terapagos: every gym type mastered (`FLAG_GYM_TYPE_MASTERED` + type, set by the mastery
           battle; the gym's last type now has a mastery battle too, and Oak's visit after it hints
           at the crater). Arceus (Sinnoh) waits for the same.
         Okidogi, Munkidori and Fezandipiti ROAM: each time the player enters an area, each uncaught
         one is there with a 1 in 8 chance, wandering a random reachable grass/sand/cave spot away
         from the player (`ExoticSafari_AreaOnTransition`, the spots from the builder); battled and
         not caught, it runs off.
   - [x] The Galar zone (round 1, for Nick's review; `tools/gym_hack/galar.py`): the MEADOW (Postwick's
         countryside: HnS's Johto farmland pieces - wheat, canal, farmhouses, fenced pond; the hub)
         -> east the LAKE (the Wild Area's Lake District: Hoenn's Safari Zone south-east, river,
         waterfall and lake) -> north-west the FOREST (Glimwood Tangle: Ilex Forest with copies of
         its tilesets recolored to a night glow, glowing mushrooms drawn here - big ones on the floor,
         giant ones in place of trees, small ones dotted about - their palette pulsing,
         `InitTilesetAnim_Glimwood`) / north the CITY (Circhester: Olivine's terraced stone houses
         and cobbles, snowed over: tileset copies with greens turned to snow) -> north the STADIUM
         (Wyndon: the Battle Dome's lobby and battle room, its banner saying WYNDON) / east the MINE
         (Galar Mine: Granite Cave 1F and B1F). Legendaries: Regieleki (meadow), Zarude and
         Enamorus (forest), Zacian and Zamazenta (city), Regidrago (mine B1F), Kubfu (pitch); Spectrier
         and Calyrex roam. Plain encounters for now (puzzles: round 2).
   - [x] Galar's evolution items, found then sold at the kiosk: Tart and Sweet Apples (meadow),
         Chipped Pot, Galarica Wreath, Strawberry Sweet (forest; then all seven Sweets: each evolves
         Milcery like a stone, into its vanilla-cream Alcremie), Galarica Cuff (lake), Razor Claw
         (city), Black Augurite and Peat Block (mine), Cracked Pot (stadium); Kubfu's two scrolls
         (pitch) are one of a kind. Level-ups in place of other conditions: Sirfetch'd 40,
         Runerigus 34, Grapploct 35, Overqwil 40, Basculegion 40, Wyrdeer 40; no time of day for
         Obstagoon, Frosmoth, Sneasler, Ursaluna.
   - [x] Galar round 2 (Nick's review). Legendary puzzles (`CONDITIONS` in galar.py; `SealLeft`,
         `ExoticSafari_TryAppear` - a static comes as soon as its seal breaks, even mid-visit):
         Zacian/Zamazenta at their plinths once the Rusted Sword (forest) / Rusted Shield (mine
         B1F) is found (kept for the Crowned forms; never sold); Zarude once Glimwood's 8 dark
         mushrooms (4 big, 4 giant) are touched and lit (`ExoticSafari_Lamp`, flags 0x221-0x228);
         Enamorus at the pink glade for a lead with the most friendship; Kubfu on the pitch once the
         dojo master (Bea, her Convention team) in the lobby is beaten; Regieleki once the meadow's 7
         stepping stones (a lightning bolt, new glowing metatiles in a copy of the farmland tileset)
         are stepped on from the top of the bolt down - out of order puts them out
         (`ExoticSafari_Stone`); Regidrago for the Dragon Fang (mine 1F) and a Dragon-type lead.
         Galarian Articuno, Zapdos and Moltres roam with Spectrier; Calyrex roams only once Spectrier
         is caught (roamers' `needFlag`; hide flags FLAG_TEMP_1B-1F). Pecharunt counts only Paldea's
         roamers. A static that ran stays away for the visit (`ExoticSafari_MarkGone`, VAR_TEMP_8).
   - [x] Regional forms wild in their own generation's zone: Galar has Galarian Meowth, Ponyta,
         Rapidash, Slowpoke, Slowbro, Farfetch'd, Weezing, Mr. Mime, Corsola, Zigzagoon, Linoone and
         Hisuian Growlithe, Arcanine, Voltorb, Electrode, Typhlosion, Samurott, Lilligant, Zorua,
         Zoroark, Braviary, Sliggoo, Decidueye (with the earlier ones); Paldea adds Bloodmoon
         Ursaluna (forest). Goodra (both forms): level 50, no rain needed.
   - [x] Map edges: tree lines carried on by aligned border blocks (no cut trees), Paldea's drawn
         tree strips whole; Mt. Silver's slopes widened, their sky turned to snow (peak corners
         redrawn over snow); the desert's cut cliffs/boulders at its edges tidied; Glimwood's
         south-east glade opened up (two trees cleared).
   - [x] Glimwood's mushrooms drawn under the player (walking up to one from below, the player is in
         front); the giant ones stand in clearings: two whole trees cleared (`clear_trees`), the
         trees above ending in their trunks, the ones below starting with their tops (`retile`).
   - [x] The Alola zone (round 1, for Nick's review; `tools/gym_hack/alola.py`): islands linked by sea
         (surf off an area's edge onto the next one's water). RESORT (Hano Beach: the Sevii Islands'
         Resort Gorgeous, palm-garden grass added; its mansion door leads back to the hall; the hub)
         -> west the OCEAN (Seafolk Village: Pacifidlog's rafts and houses out at sea, converted,
         its sinking logs plain floor) -> west the RUINS (Ruins of Conflict: Ruin Valley's jungle);
         east the BEACH (Big Wave Beach: Treasure Beach, a coast drawn at its foot); north the SNOWY
         MOUNTAIN (Lanakila: Sevault Canyon with snowy copies of its metatiles above a ragged
         snowline, green below, the sea at its foot); south the VOLCANO (Wela: Mt. Ember over Kindle
         Road's beach and sea, its two caves joined). Alolan forms and the Ultra Beasts wild;
         Type: Null, Magearna, Meltan (resort), Cosmog (beach), Zeraora (ocean), Marshadow (ruins),
         Solgaleo and Necrozma (volcano), Lunala (mountain) stand; the Tapus roam. Plain
         encounters for now (puzzles: round 2). Evolutions: Cosmoem (Sun/Moon Stone), Rockruff
         (Sun/Moon/Dusk Stone: Midday/Midnight/Dusk), Meltan level 50; Sun, Moon and Dusk Stones
         found in the volcano, mountain and ruins.
   - [x] Alola round 2 (Nick's review). Beach things: Emerald's Route 109 umbrellas and deck chairs and
         a drawn palm tree (`tile_import.py`: metatiles copied between tilesets, their shadows redrawn
         in the target's sand colors; tilesets compacted to make room) set about on the sand and
         grass (`alola.scatter`, never cutting a way off); the resort's island and Big Wave Beach
         widened (`paint_beach`: sand with its shore edges worked out). Seafolk Village's Pokémon
         Center is an open platform with an umbrella and deck chairs. Puzzles: Type: Null from the
         resort's caretaker once Rowlet, Litten and Popplio are caught; Cosmog once the four Tapus are
         caught; Solgaleo/Lunala for the Sun Flute (ruins) / Moon Flute (village); Necrozma once both
         are caught; Magearna for four machine parts (beach, ruins, volcano, mountain); Meltan for a
         Steel-type lead, Marshadow a Fighting/Ghost one; Zeraora only in Seafolk Village's
         thunderstorms (one visit in four: `ExoticSafari_AreaOnTransition`). Relic flags 0x330-0x335.
   - [x] The Kalos zone (round 1, for Nick's review; `tools/gym_hack/kalos.py`). CITY (Lumiose:
         Saffron with Silph Co. cleared into a plaza and a drawn Prism Tower, `tile_import.
         add_prism_tower`, its lights pulsing - `TilesetAnim_Lumiose`; flower gardens; the hub, its
         gates leading out; the homes' doors lead nowhere) -> north the WOODS (Winding Woods: a
         generated maze of tree walls and tall grass, `woods_rows`, in dim blue-green copies of the
         forest tilesets) -> north the SNOWY PATH (Mamoswine Road: Route 9 under snow, its road
         trodden snow, its rock frosted); west the CAVE (Glittering Cave: Mt. Moon 1F in cold blue);
         east the DESERT (Lumiose Badlands: Hoenn's Route 113 as dust and red rock) -> east the
         OCEAN (Azure Bay: Five Island's Water Labyrinth, a gap opened in the islet's rocks).
         Kalos Pokémon wild (Flabébé colors, Pumpkaboo sizes, Vivillon patterns, the starters,
         fossils); Zygarde (city), Hoopa (cave), Yveltal (badlands), Volcanion (islet) stand.
         Tyrunt/Amaura evolve at 39 with no time condition. `reachable` now jumps ledges.
   - [x] Kalos round 2 (Nick's review). Lumiose made bigger (two rows and columns put in inside its
         fence ring: `_insert_cols`, trees and fence posts kept in step), small tall-grass beds in
         its four corners and at the tower's foot (the gardens round the plaza gone; the house Silph
         Co. stood over given back its right side). The Prism Tower redrawn in the buildings' style:
         seen corner-on, a lit face and a shaded one, girders bent round the near edge, the deck a
         band of glass, standing on a base building with an arched door (`prism_tower_art`, 7x13).
         Winding Woods: the trees standing in clearings get their crowns' tips (Viridian Forest's
         0x281 tip, over each clearing ground) - no more flat tops. Puzzles: eight Zygarde Cells
         about the zone (object `ZYGARDE_CELL`, flags 0x344-0x34B); with three found Zygarde comes
         as its 10% Forme (the player may wait), with all eight whole, with Power Construct
         (`ExoticSafari_LegendForm`, `PRE_BATTLE` scripts); Hoopa comes through a ring when the
         Prison Bottle is taken from its pedestal in Glittering Cave (flag 0x34C; the bottle turns
         it Unbound); Volcanion for a Fire- or Water-type lead with the other type behind it;
         Yveltal once ten Kalos POKéMON (Dex 650-721) are caught. A Furfrou groomer on the plaza
         gives any of the ten trims for ¥500 (`ExoticSafari_TrimFurfrou`).
   - [x] The Unova zone (round 1, for Nick's review; `tools/gym_hack/unova.py`). BRIDGE (Skyarrow
         Bridge: the Sevii Islands' Bond Bridge, the hub; its east end leads back to the hall) ->
         its forest opening the CHASM (Giant Chasm: Pattern Bush under snow) -> its far path the
         TOWER (Dragonspiral Tower: Johto's Burned Tower in gray-green stone); a cave cut into its
         cliff (Route 4's cave mouth) the CAVE (Chargestone Cave: Rock Tunnel B1F charged blue) ->
         its far ladder the CASTLE (Relic Castle: Hoenn's Mirage Tower 1F); one of Tanoby's shrines
         on its beach (`tile_import`) -> stairs down to the RUINS (Abyssal Ruins: two Tanoby
         chambers side by side in deep blue-green, MAP_TYPE_UNDERWATER: the player swims in a
         diving helmet - `OBJ_EVENT_GFX_PLAYER_DIVER`, Emerald's underwater sprite with a glass
         helmet and Red's cap, now FireRed's underwater avatar; underwater battle backgrounds).
         Unova Pokémon wild (Deerling/Sawsbuck seasons, Basculin stripes, the starters, fossils);
         Terrakion and Virizion (bridge), Cobalion and Kyurem (chasm), Reshiram (tower), Zekrom
         (cave), Victini (castle), Keldeo (ruins) stand; Thundurus and Landorus roam (Tornadus,
         Meloetta and Genesect are gym-only). Karrablast/Shelmet evolve with a Linking Cord.
   - [x] Unova round 2 (Nick's review: the layouts kept). Chargestone's boulders and pebbles glow:
         their top layer in a palette of their own that pulses (`unova.cave_glow`,
         `InitTilesetAnim_Chargestone`; converted tilesets can now take an animation callback,
         `tileset_callback`). Puzzles: Cobalion once Terrakion and Virizion are caught, Keldeo once
         all three Swords are (SEAL_CAUGHT: `sCaughtSeals`, the hint names who's left); Reshiram
         for the Light Stone (Relic Castle), Zekrom for the Dark Stone (Abyssal Ruins), Victini for
         the Liberty Pass (Skyarrow Bridge) (SEAL_KEY: `sUnovaKeys`, flags 0x372-0x374); Kyurem once
         Reshiram and Zekrom are caught, then the Chasm's old man gives the DNA Splicers (0x370);
         Landorus roams once Thundurus is caught, and the bridge's shrine keeper gives the Reveal
         Glass once both are (0x371).
   - [x] The Sinnoh zone (round 1, for Nick's review; `tools/gym_hack/sinnoh.py`). LAKE (The Three
         Lakes: drawn pinewoods, three oval lakes each with an island - Verity, Valor, Acuity -
         painted from a water mask with FireRed's edge and inner-corner pieces, `paint_water`; the
         hub) -> west MEADOW (Floaroma Meadow: Hoenn's Route 117, its Day Care gone); south-east
         MARSH (Great Marsh: Hoenn's Safari Zone SW, murky water, rain); north CITY (Snowpoint City:
         Four Island under snow, snowy grass) -> Icefall Cave's mouth up to RUINS (Hall of Origin:
         Sky Pillar's top in white and gold under a violet sky); east VOLCANO (Stark Mountain:
         Jagged Pass in dark basalt and ash, encounters on its whole floor) -> its cave DEPTHS
         (Stark Mountain's inside: the Fiery Path). Sinnoh (Gen 4) Pokémon wild, both Shellos seas,
         the three Burmy cloaks, the starters, fossils, babies, Rotom. Uxie, Mesprit and Azelf on
         the islands, Phione (marsh), Regigigas (city), Heatran (inside), Arceus (hall) stand;
         Manaphy roams (flags 0x378-0x37F). Dialga, Palkia, Giratina, Cresselia, Darkrai and
         Shaymin are gym-only.
   - [x] Sinnoh round 2 (Nick's review): Floaroma Meadow redrawn bigger (66x44): six great flower
         fields in stripes, diagonals and patches, paths between, a pond, a few pines. Its flowers
         are drawn for it (`sinnoh.add_flowers`: one palette of six petal colors - red, yellow,
         pink, purple, white, blue - two tiles a color) and are wild-Pokémon tiles themselves
         (MB_INDOOR_ENCOUNTER: no grass overlay); no tall grass. Puzzles (seals 25-29): Uxie for a
         lead of level 70+, Mesprit for a lead holding a berry, Azelf for a lead with 252 HP EVs;
         Regigigas for Regirock, Regice and Registeel in the party (as in Platinum; they'll come
         with the Hoenn zone); Arceus for the Azure Flute (flag 0x380), which the Snowpoint elder
         gives once every gym type is mastered (a hint before). Rotom's Room (a Snowpoint house):
         a microwave, washer, fridge, fan and mower (object sprites in `exotic_safari_art.py`); each
         opens the party menu and changes the Rotom picked as the Rotom Catalog does - its form,
         its move with the forget-a-move prompt - or lets it back out (`ChooseRotomForAppliance`).
   - [x] No getting stuck, everything reachable: `unstick` (build_exotic_safari.py) opens a ledge
         wherever one would leave the player unable to get back to where they came in, and
         `tools/gym_hack/check_safari_reach.py` (run after every build) walks every zone from the
         hall's doors through every way out and lists any legendary, item or way out that can't
         be reached, or any spot with no way out. It found real ones, now fixed: Galar City's way
         in from the lake was a walled-in corner (now its open south edge), Galar Lake's river to
         the mine was blocked by a boulder, Skyarrow Bridge's cave mouth stood on a beach cut off
         by sea rocks, and Chargestone's and Paldea's crater's ladders onward were in halves of
         their caves the way in doesn't reach (moved to ladders that are).
   - [x] The Hoenn zone (round 1, for Nick's review; `tools/gym_hack/hoenn.py`). FOREST (Fortree
         City on its stilts, with Route 120's north end east of it: tall grass, the ponds under the
         bridge; the hub, arrival at the Pokémon Center's door) -> the mound in the pond (surf to
         it) TOMB (Ancient Tomb, steel grey); west OCEAN (Pacifidlog Town and Route 131 with Sky
         Pillar's islet; Pacifidlog's cliff given a west end, a second cave copied into it, its logs
         plain floor, wild Pokémon on the two beaches) -> Sky Pillar's cave to CRATER (Sootopolis
         City, in through the Cave of Origin's door; wild Pokémon on its lawns) and Pacifidlog's
         cave to ICE (Island Cave, ice blue); east VOLCANO (Mt. Chimney's summit, encounters on its
         whole floor) -> a cave (Jagged Pass's) in its rock to RUINS (Desert Ruins, sandstone). The
         three Regi chambers are Emerald's two-room layout (the Regi up through the inner door),
         recolored. Gen 3 Pokémon wild, the three starters rare. Regirock, Regice, Registeel,
         Kyogre (open sea), Jirachi (Sootopolis's island), Deoxys (by the lava) stand; Latias and
         Latios roam (flags 0x381-0x388). Groudon and Rayquaza are gym-only. Regigigas can now be
         caught.
   - [x] Hoenn round 2 (Nick's review): the puzzles. The Regi chambers' inner walls (Emerald's cover
         and Braille-wall metatiles, carried through the conversion on two unreachable cells:
         `hoenn.wall_read`; `sRegiWalls`) are sealed on load until the Braille tablet beside the
         Island Cave sees Wailord first and Relicanth last (Emerald's `CheckRelicanthWailord`, the
         Sealed Chamber's rumble; 0x389); then each wall opens for its own Braille - Regirock's for
         a lead that knows Rock Smash, Registeel's for one that knows Flash, Regice's after two
         minutes without a step in front of it (`Task_Stillness`) (0x38A-0x38C). Latias and Latios
         roam once the Soul Dew is found (a Pacifidlog raft; roamer needs can now be any flag);
         Kyogre rises for a lead holding the Blue Orb (Sootopolis's lawn); Deoxys comes once the
         Meteorite is found (Mt. Chimney's summit; it changes Deoxys's Forme); Jirachi wakes once
         the zone's seven other legendaries are caught (seals 30-32; items 0x38D-0x38F).
   - [x] The Johto zone (round 1, for Nick's review; `tools/gym_hack/johto_safari.py`): HnS's own maps
         on their tilesets (`import_johto.py` now brings in New Bark's, Mahogany's, the Ruins of Alph's,
         Dragon's Den's and the Ice Path's too). GRASSLAND (New Bark Town with Route 29 west of it, as
         HGSS joins them; woods in the corners; the hub, in at Elm's lab) -> west RUINS (Ruins of Alph,
         in at the Route 32 gate) -> its north gate FOREST (Ilex Forest, Celebi by the shrine); Route
         29's gate LAKE (Lake of Rage, the Red Gyarados - shiny, level 55, its own script, 0x3D5 -
         in the middle) -> its south-west path ICE (Ice Path 1F: ice to slide across) -> its ladder
         CAVE (Dragon's Den) -> the shrine's door LUGIA (Whirl Islands' Lugia chamber, Lugia on its
         island). Gen 2 Pokémon wild (Unown A, L, P, H and ? in the ruins), the starters rare.
         Raikou, Entei and Suicune roam (flags 0x3D0-0x3D4). Ho-Oh is gym-only.
   - [x] Johto round 2 (Nick's review): woods over Ilex Forest's south gatehouse (it led nowhere).
         Raikou, Entei and Suicune roam once every one of the six areas has been visited (flags
         0x4C0-0x4C6; an old man in New Bark Town keeps count). Lugia: the Dragon's Den elder bars
         the shrine's door until the Silver Wing is found (the Ice Path; a keepsake, 0x4C7). Celebi:
         comes to the Ilex shrine once the GS Ball is found (the Ruins of Alph's west half, 0x4C8;
         seal 33). The ruins' west half (Union Cave's side in HGSS) is reached by its two cave mouths,
         joined, and a gap in the lower plateau's cliff. The Unown: four stone panels at the
         chambers' doors (ESCAPE: an Escape Rope; WATER: a Water Stone; LIGHT: a lead that knows
         Flash; HO-OH: Ho-Oh in the party; 0x4C9-0x4CC); once all are open, every wild Unown there is
         any of the 28 forms (`ExoticSafari_WildSpecies`, from `CreateWildMon`). The Red Gyarados
         stays as it is. (0x0B0-0x0BF looked free but the game sets them: don't use them.)
   - [x] `tools/gym_hack/audit_obtainable.py`: every Pokémon and form in the game - can the
         player get it (wild in a reachable map, gifts, statics, prizes, trades, safari and gym
         legendaries, starters, then evolutions and form changes with the items they can get), and
         what only comes from a one-time source. First run (v66): 1,137 of 1,571 entries
         obtainable; 179 are battle-only or totem forms; the rest are listed for Nick.
   - [x] Every Pokémon obtainable (v67; the audit now finds every storable entry but the 29
         event-only forms, and no one-time choices):
         - Evolutions: Magnezone (Thunder Stone), Lickilicky (level up knowing Rollout), Leafeon
           (Leaf Stone), Glaceon (Ice Stone), Sylveon (friendship + a Fairy move) unlocked one by
           one (the Gen 4/6 cross-evolution switches stay off). Plain trade evolutions (Gigalith,
           Conkeldurr, Alolan Golem, Trevenant, Gourgeist, Escavalier, Accelgor) also at Lv 36, and
           the Linking Cord is sold at Celadon 4F. Gumshoos: Sun Shard; Alolan Raticate: Moon Shard.
           Alcremie's cream is the Milcery's own, from its personality (`ExoticSafari_EvolvedForm`).
         - Wild: a safari slot can be shared ("A|B|C" in a region's land/water tables: the table has
           A, `ExoticSafari_WildSpecies` picks one; `sSlotPools`, and src/data/exotic_safari_pools.json
           for the tools). Unova got its 22 missing lines (and Zorua, Stunfisk, Rufflet/Braviary in
           their Unova forms), Kalos Bunnelby and Klefki, Alola 9, Galar Blipbug. Wild Scatterbug,
           Spewpa, Vivillon (18 patterns), Flabébé's line (5 colors), Pumpkaboo/Gourgeist (4 sizes),
           Deerling/Sawsbuck (4 seasons), Minior (7) and Dudunsparce (2) come in any form.
         - Kubfu: once caught, visiting every Galar area again (0x394-0x39B) brings a second to the
           pitch (0x39C; the seal sign counts the areas left, the dojo master tells of it).
         - Zygarde: 12 Cells (4 more: 0x390-0x393). Its static is 10% with 3-7, 50% with 8-11,
           50% Power Construct (Complete in battle) with all 12; the last Cell leaves the Zygarde
           Cube. Once caught, 1 in 50 wild encounters in the Kalos zone is another Zygarde (Lv 70):
           10% Forme, the 50% with 8 Cells, the Power Construct ones with 12.
         - Form items: hidden items in the safaris (`"hidden"` in an area: bg hidden_item, flags from
           0x4CD): Silvally's 17 Memories about Alola, each where its type is at home; the Adamant
           Crystal (Hall of Origin), Lustrous Globe (lakes), Griseous Core (Stark Mountain) and
           Gracidea (Floaroma meadow). Item balls: the N-Solarizer (Alola volcano), N-Lunarizer
           (Lanakila), Reins of Unity (Freezington). Items given no spot are placed by `pick_spot`.
           Genesect's Drives: item balls in Viridian Forest, the Safari Zone's east, Route 25 and
           Route 12's grass, shown only once Genesect is in the Pokédex as caught
           (`FormItems_ObjectWaits`, from `TrySpawnObjectEvents`; flags 0x39D-0x3A0). Arceus's
           plates: each mastered type's plate rests on the gym floor (a spot hashed from the trainer
           id, the nearest open floor if decorations cover it: `src/form_items.c`), found with A
           (hidden-item script, flags 0x3A4 + type); only the Snowpoint elder hints at it.
   - [x] Title cards (v68): before each Gym Leader Convention battle, a full-screen card
         (`src/title_card.c`, `TitleCard_Show`, reusable for later events): the event and hall at
         the top, the trainer's picture 1.6x sliding in, their name doubled in size across a band
         in the hall's color, then their title, region and games. A/B or 5 s moves on. The lines:
         `tools/gym_hack/convention_cards.py` -> src/data/gym_convention_cards.h
         (`Convention_ShowTitleCard`, from `Convention_EventScript_Battle`).
   - [x] The Gauntlet, part 1 (v70): after the convention's every hall and every type mastered, a
         hooded figure (`OBJ_EVENT_GFX_GAUNTLET_HOODED_FIGURE`, drawn by `gauntlet_art.py`) turns
         up in the gym (VAR_GAUNTLET_STATE 0x40F5, scene from the gym's ON_FRAME table; figure local
         id 17, FLAG_HIDE_GAUNTLET_FIGURE 0x3C0) claiming the Gym Leaders hired them. 78
         challengers (rivals, villains, Frontier Brains, professors... `gauntlet_roster.py`), region
         by region, level 100, full IVs/EVs, the smartest AI with prediction, no more than two
         legendaries a team; any Pokémon may battle. VAR_GAUNTLET_NEXT (0x40F6) is the next one: a
         loss changes nothing. Each gets a title card; after a win, continue or rest. The clipboard:
         beaten/total, a page per region (names beaten, ??? for the rest). Sprites: Emerald Rogue,
         HnS, smogon/sprites and PokéRogue (scaled into 64x64, 15 colors) by `build_gauntlet.py`.
         Part 2 (Bill) picks up at GAUNTLET_ALL_BEATEN.
   - [x] Five more challengers no sprite set had (v71; 83 in all): Volo, Prof. Sycamore, Carmine,
         Prof. Sada and Prof. Turo, their pictures repainted from PokéRogue class pictures by
         `gauntlet_custom.py` (tools/gym_hack/art/, read as "art:<file>").
   - [x] The Gauntlet, part 2 (v72): Bill. After the last challenger (GAUNTLET_ALL_BEATEN) the figure
         asks if you're ready to meet who hired them - the hood comes off (FLAG_GAUNTLET_BILL_REVEALED
         0x3C3; the figure object's look is VAR_OBJ_GFX_ID_D, `Gauntlet_SetFigureGfx`): Bill, his Cell
         Separation System run backwards on himself to become a Clefable went wrong: a monstrous half-thing, CLEFFABILL (why the hood). He wants it ended. His team:
         shiny Mewtwo, Lugia, Ho-Oh, Giratina-Origin, Rayquaza and himself - CLEFFABILL (a species of
         its own, SPECIES_CLEFFABILL: Fairy/Ghost, 150/80/130/170/150/120 = 800, Magic Guard + Life Orb;
         not in the Pokédex, not obtainable, never shiny in his hands; art by `bill_art.py`, his
         walking sprite and the Palm Pilot by `bill_ow.py`); the Deoxys battle music. Lose and he
         waits. Win and he fades away, leaving his Palm Pilot (GAUNTLET_SIMULATOR):
         - WITHDRAW (VAR_GAUNTLET_WITHDRAWS 0x40F4, one per finished run): any Pokémon, shiny, level
           70 - a list of all 1392 species and forms one could keep (`build_withdraw.py` ->
           src/data/dex_forms.h: National Dex order; L/R a page, START the next
           generation) with the shiny picture beside it (`Gauntlet_ChooseWithdraw`);
         - the battle simulator: the whole Gauntlet again from the first challenger, with Bill as
           the 84th; clearing it earns another WITHDRAW. Records: this run's progress by region.
   - [x] More PC boxes and the Type Mastery Hall of Fame (v76). The save's Trainer Hill and recorded
         battle sectors (30, 31: Emerald-only) are ours now (`SaveExtraSectors`/`LoadExtraSectors`
         in src/save.c, written after every full save; the special-sector functions refuse them):
         - 16 boxes (TOTAL_BOXES_COUNT; 14 before). The PC's storage now puts its boxes last
           (`layout` = STORAGE_LAYOUT_BOXES_LAST) and runs past its 9 sectors into sector 31. A save
           from before (layout 0) is rearranged as it loads (`PokemonStorage_UpgradeLayout`).
           Sector 31 has no backup copy (like the Hall of Fame's): the 15th and 16th boxes' last
           Pokémon are the only thing at risk if the power goes out mid-save.
         - Type Mastery Hall of Fame (src/mastery_hall.c, sector 30): for each mastered type, the
           team of its longest streak (after every win of a mastered type, if the streak beats the
           record, the party replaces it). "TYPE MASTERY" on the PC's menu (and the Palm Pilot's)
           once there's one; shown in the Hall of Fame's screen, titled "<TYPE> · STREAK <n>".
           The Elite Four's Hall of Fame (sectors 28-29) is untouched.
   - [x] BILL'S ARCHIVE, the living dex (v77; src/archive.c). A slot for every species and form one
         could keep (src/data/dex_forms.h, 1392, the same list as Bill's WITHDRAW). ARCHIVE in the PC's
         box menu sends a Pokémon there for good (released, but its slot - and whether it was shiny -
         is kept; a full slot turns it away; the same "can't release" rules). Taking one out gives a
         new one at level 5 (random IVs and nature, one of its usual abilities, shiny if the stored
         one was) and empties the slot: never more than one of anything. The screen ("BILL'S
         ARCHIVE" on the PC's menu once Bill's been met, and the Palm Pilot's) is laid out like a
         box: 30 icons a page in dex order - stored or owned in color (a ball if it's in a box or
         the party), not owned as a silhouette, a star on stored shinies - with the one under the
         cursor's picture, name, number and where it is on the left, and how much of the dex is owned
         (stored, boxed or in the party). L/R a page, START the next generation, A take out. Kept by
         species number (two bits each, `struct ArchiveData`) after the Type Mastery Hall of Fame in
         flash sector 30 (`gExtraSave`).
   - [x] The LEADER ACADEMY (v78, first cut; `src/leader_academy.c`, `data/scripts/leader_academy.inc`,
         maps from `tools/gym_hack/build_academy.py`, lines from `academy_text.py`, battle data from
         `build_academy_data.py` -> `src/data/academy.h`). A school for Gym Leader candidates of every
         region, through a gap in Route 11's northern trees (x 38-39; gate guards stand in it until
         the player is admitted). Everything is singles.
         - Admission (`VAR_ACADEMY_STATE`, 0x40E5): with the third badge, walking onto Route 11 has the
           mentor call out (Prof. Oak if there's no mentor), bring the player to the gap and talk the
           guards round; on the campus the headmaster, WISTERIA, walks down to welcome them (the hall,
           the Battle Hall and BP, the Exchange - money too -, the candidates, the Pokémon Center),
           then the mentor leaves. Staff are named for flowers (the professors are trees).
         - The campus (v79) has its own tileset (`gTileset_LeaderAcademy`, built by
           `tools/gym_hack/tileset_composer.py` from art in `academy_art.py`): red brick buildings
           under slate roofs - the main hall (13x9: a clock tower, a white columned portico with a
           rose in its pediment, arched windows), the Battle Hall (a Poké Ball banner), the Exchange
           (a striped awning) and two dorms - plus FireRed's Pokémon Center and mart (Saffron's).
           Lawns everywhere (the general tileset's grass, no wild Pokémon), single trees, flower
           patches and rose beds, iron lamps, benches, cobbled walks, and a Roserade statue in the
           plaza (its overworld sprite on a pedestal; the plaque reads "Grace. Thorns. Victory.").
         - The main hall inside is a library (`gTileset_LeaderAcademyHall`, over the building
           tileset): floorboards, a red carpet to the headmaster's desk (talked to across it), walls
           of bookshelves, stacks, reading tables, globes, and four doors to the lecture halls A-D
           (Viridian's classroom layout; A: Larkspur and Camellia, B: Foxglove and Verbena, C:
           Marigold and Hyacinth, D: Yarrow and Tansy). The other interiors are still FireRed's.
           The name pop-up says LEADER ACADEMY. The Pokémon Center sets the respawn to Vermilion's.
           (v99: its own map section, `MAPSEC_LEADER_ACADEMY`, on the Town Map north of Route 11
           with a town dot, and a FLY destination - `FLAG_WORLD_MAP_LEADER_ACADEMY`, 0x0F0, set on
           the campus, landing at the Pokémon Center - which is now where you respawn,
           `HEAL_LOCATION_LEADER_ACADEMY`. The campus is 4 rows taller at the bottom: eight rows of
           trees and road, so none of its own tileset is drawn while the player's on Route 11 - drawn
           with Route 11's tileset, the cobbles came out as garbage at the top of the screen and stayed
           after walking back up.)
         - MEGA EVOLUTION (v82): after the 8th badge, entering the main hall has the headmaster
           call the player to her desk and hand over the MEGA BRACELET (`ITEM_MEGA_RING` renamed;
           `FLAG_GOT_MEGA_BRACELET`). The guard at door M then steps aside: PRIMROSE's room
           (`LeaderAcademy_MegaRoom`). Her line: a Mega Stone is a witness and only answers a bond
           it has seen proven, so you earn one by winning a battle with that Pokémon alone.
           The trial: pick a party Pokémon (two stones like Charizard X/Y: pick one); she lends the
           stone, the rest of the party is set aside (rental backup), and it fights her 3 Pokémon
           alone. Win to keep the stone; lose and try again any time. Her teams are generated per
           stone (`BuildMegaTrialTeam`, `src/leader_academy.c`, seeded by the stone so they never
           change): levels 55-59 plus a level-60 Mega last; every foe is a fully evolved base form,
           non-legendary, BST 430-560 for the first two, with no type super effective on the
           player's Pokémon in either form, and the Mega's STAB at least neutral on them; moves are
           level-up damaging moves (no OHKO/explosions, nothing super effective) plus one status
           move. Fixed levels on purpose: over-levelling is the way through. Rayquaza needs a stone
           like the rest (the RAYQUAZITE, a new item, `ITEM_RAYQUAZITE`; no Dragon Ascent Mega, so
           it can't Mega Evolve and hold another item), and Kyogre and Groudon take the trial for
           their Blue/Red Orb (the Blue Orb is also in the Hoenn zone). Zinnia's and Bill's
           Rayquazas hold the Rayquazite. A test checks every stone's team.
         - EVENT POKéMON (v84): every form that was only ever an event is now a side quest
           (`src/event_mons.c`, `data/scripts/event_mons.inc`, lines `tools/gym_hack/event_mons_text.py`,
           safari pieces `tools/gym_hack/event_mons_safari.py`). Gifts after the gym is the player's are
           Lv 50 (first stages Lv 5); main-game gifts Lv 5.
           1. Costume Maker (Celadon Condominiums 2F, post-game): Cosplay Pikachu, then costumes from her
              menu: Rock Star (a 7-win Battle Hall streak, `FLAG_EVENT_HALL_STREAK_7`), Belle (20 gym
              decorations placed), Pop Star (the Convention's Fairy hall), Ph.D. (all 8 Academy
              lectures), Libre (the Convention's Fighting hall). Each costume brings its ORAS move.
           2. Cap Collector (the boy in Pallet's house, main game): the CAP CASE (`ITEM_CAP_CASE`); seven
              caps hidden in Kanto where small hidden items were (Viridian Forest, Mt. Moon B2F, Route
              25, Route 9, Route 10, both Underground Paths; signs, `EventMons_EventScript_Cap0-6`), each
              traded for its cap Pikachu, all seven for the Partner Cap one (`VAR_EVENT_CAPS` 0x4091).
           3. Partners: once Electric (Normal) is mastered, Oak's 5-question type quiz gives Partner
              Pikachu (Eevee), Lv 5; the team builder waits for the mastery too (`AVAIL_TYPE_MASTERED`).
           4. Spiky-eared Pichu (Lv 5): a static beside the safari's Ilex shrine, for a party with Celebi.
           5. Vivillon collector (Kalos city): 6 patterns shown -> Fancy Scatterbug, all 18 -> Poké Ball
              Scatterbug (`VAR_EVENT_VIVILLON_LO/HI`); each then turns up wild in Kalos, 1 in 19.
           6. Battle Bond (Kalos ocean): a Froakie once a Kalos trainer's beaten in the tournament; its
              line ends in Greninja's Battle Bond form (`MON_DATA_BATTLE_BOND`, FireRed's unused shadow bit).
           7. AZ (Kalos badlands, once beaten in the tournament, or it's been won): shown a Floette, then with Xerneas,
              Yveltal and Zygarde caught, the Eternal Flower Floette (its Mega: Primrose's Floettite).
           8. Tala (Alola resort): her Lycanroc pack (Lv 58-60); win for an Own Tempo Rockruff.
           9. The artisan (Alola resort): Magearna caught and her 4 paint pots (one each in the Beach,
              Ruins, Volcano and Lanakila) -> Original Color Magearna.
           10. Zarude (Dada): a static in Glimwood's glade for a party with Celebi, Zarude caught.
           11. Eternamax: a static on the stadium's pitch for a party with Zacian, Zamazenta and Eternatus
              (the Darkest Day). It's the pitch's alone: elsewhere an Eternamax in the party is an
              Eternatus again, and on the pitch every Eternatus is Eternamax (`EventMons_OnMapLoad`).
           The statics use the safari's seals (`SEAL_PARTY`, `sPartySeals`). Flags 0x4E2-0x4F3. Pallet's
           Lucky Egg moved to flag 0x4F4: it had shared 0x4A7 with Erika's Silver Cap.
         - Megas in other teams: 57 Gym Leader Convention trainers hold a Mega Stone (Norman,
           Misty, Clay and Shauntal added), and 16 Gauntlet trainers (Maxie, Archie, Zinnia, Wally,
           Lysandre, Serena, Calem, Silver, Lyra, Ethan, Dawn, Riley, Guzma, Arven, Brendan, May).
         - BP: the frontier's `battlePoints`. Earned by beating candidates (1-4 by skill), the
           instructors' demonstrations (3) and Battle Hall streaks (1 + streak/3 a win, +5 at 7).
         - The EXCHANGE: two counters side by side, the same competitive items (Choice items, Life Orb,
           Focus Sash, Assault Vest, Booster Energy, Heavy-Duty Boots, weather rocks...) for money
           (a mart) or BP (32 for a Choice item, 4 for a berry). The BP counter also has Bottle Caps,
           the Ability Capsule and Patch, Cilantro (16 BP), the Multivitamin and eight mints (4 BP).
           Mints cost ¥10,000 and Cilantro (any nature) ¥40,000, everywhere. Signs, the
           headmaster and the clerks all say money works too. The SUPPLY shop (money): caps, ability
           items, Cilantro, Multivitamin, ten mints, vitamins, power items, the Macho Brace, the EV
           berries.
         - Candidates (8, `CANDIDATES`): Koji (Johto, Fighting, rookie), Maren (Hoenn, Water), Elias
           (Sinnoh, Ground), Nadia (Unova, Electric, advanced), Rene (Kalos, Fairy, advanced), Keola
           (Alola, Fire, rookie), Briony (Galar, Dragon, expert), Teo (Paldea, Steel). Each talks about
           their region and their gym dream, then offers a practice battle, any number of times: the
           player's own team against theirs (base forms evolved for the level: the player's average
           -4/-2/+1/+3 by skill; 3-6 Pokémon; items and smarter AI higher up). No money, no white-out;
           both sides healed after.
         - The instructors (in the main hall): LARKSPUR speed and priority, CAMELLIA setup and win
           conditions, FOXGLOVE hazards, VERBENA pivoting, MARIGOLD weather and terrain, HYACINTH
           reading the opponent, YARROW EVs/natures/items, TANSY team building. A four-box singles
           lecture (the first time, straight away; again on request), and a demonstration battle:
           their 3 level-100 sets that show the lesson against 3 rentals the player picks.
         - The BATTLE HALL: rentals (40 level-100 sets, `RENTALS`): pick 3 of 6 offered (each shown
           with its item and moves), then battles against candidates with 3 rentals each, AI getting
           smarter with the streak; keep going or stop after each win. The player's party is copied
           aside (EWRAM) and put back after, won, lost or backed out; the whole run is one locked
           script, so the game can't be saved with rentals in the party.
         - The TEAM BUILDER (Tansy's, on the PC by her desk in Lecture Hall D, usable any time:
           `Academy_EventScript_TeamBuilderPC`; `src/team_builder.c`): 8 teams, kept in flash sector 30 after the
           archive (`struct TeamBuilderData`; a move is 10 bits to fit). A slot: choose a type, then
           one of its Pokémon from a grid of icons (a ball by those the player has: party, box or
           archive; only those with the type as their primary type, the gym's rule; and only those
           the player has or could get by now: `gDexFormAvail` from `tools/gym_hack/build_availability.py`,
           run it after changing wild tables, gifts or legendaries - the main game's from the start,
           Cerulean Cave's after the Hall of Fame, Johto's and the Exotic Safari's once the gym is
           theirs, Terapagos and Arceus once every type is mastered, a gym legendary once the gym has
           had its type; everything once Bill's Palm Pilot can withdraw), then up to 4 of
           the moves it can learn (level, TM/tutor, egg). The edit screen
           shows the team's worst weaknesses and the types it can't hit for 2x; START, the full chart:
           for each of the 18 types, how many are weak, how many resist or are immune, and the best
           hit the team's attacks have. The screen says the limit (8) and that teams are kept when the
           game is saved.
         - Not yet: the lecture halls, Battle Hall, Exchange and shops inside are FireRed's; the battle hall has no streak record; the
           team builder doesn't load a team into the party (it's a planner); abilities (Levitate...)
           aren't in the chart; doubles everywhere is a later, game-wide project.
   - [x] The reachability check (and `unstick`, roamer spots) now follows the game's rules more
         closely (`moves_from`): elevation (a step only to the same height, or by stairs/ladders at
         0, or bridges at 15; surfing on and off at 3) and ice (sliding on until something stops
         it). It found a real one: Galar's Glimwood Tangle glade (Zarude, the Chipped Pot) was raised
         ground with no way up - now level with the rest.
   - [x] Area names: each Exotic Safari area pops up its own name (the place it's based on:
         `tools/gym_hack/safari_names.py`, shared with the guides; `ExoticSafari_GetAreaName`, used
         by the map name pop-up), every time the player walks into an area; the Safari Zone's
         savanna preview screen no longer shows for them.
   - Tools: `convert_layout` (any Emerald or FireRed layout piece -> its own FireRed tileset, palettes
     folded when there are too many - a folded palette's palette folded too now follows through),
     `clone_tileset` (a recolored copy of a tileset). `SAFARI_REACH_DUMP=<dir>` writes each area's
     reachable cells (debugging spots).
   - [x] Evolutions: Paldea's walk/battle/move-count evolutions are level-ups (Pawmot 38,
         Brambleghast/Rabsca/Houndstone 30, Gholdengo 45, Hydrapple 50); the items (Auspicious and
         Malicious Armor, Ice Stone, Unremarkable and Masterpiece Teacups) are item balls in the desert,
         crater, snowfield and academy, and once found the SAFARI KIOSK sells them (`ExoticSafari_Kiosk`).
         All four Squawkabilly plumages are wild (meadow, academy), and Artisan Poltchageist (academy).
         The academy's stairs (they lead nowhere) are behind velvet ropes.
   - [x] The hall: doors in each region's games' two colors (Gold/Silver, Ruby/Sapphire,
         Diamond/Pearl, Black/White, X/Y, Sun/Moon, Sword/Shield, Scarlet/Violet) that slide open
         when a pass holder stands in front of an open zone's door (`src/exotic_safari.c`); a
         counter splits the hall, with the attendant in its gap. After the League the attendant
         sells the EXOTIC PASS (one-time Gym Leader special, ¥10,000: trainers will want exotic
         Pokémon once they see a leader's), the camera shows the doors, and they step aside.
         Before that, "not open yet, come back later". The SAFARI KIOSK on the counter sells
         balls, Max Repels and healing items (and, v101, the Incense); the EVOLUTION KIOSK on the
         other side (v101) the evolution items found in the zones (`ExoticSafari_EvoKiosk`).
   - [x] The Incense (v101; `ITEM_INCENSE`, ¥50,000, the Safari Kiosk only; `src/incense.c`):
         lit, for 600 steps (the Safari Zone's; `VAR_INCENSE_STEPS`, 0x4096) every wild Pokémon
         that's 10% or less of that method's encounters there (its slots added up; an Exotic
         Safari shared slot split between its species) is 5 times as likely, and the common ones
         share what's left in their old proportions. If the rare ones would pass 100% (more than
         20% of rare ones), the common ones get nothing and the rare ones split it all in their
         own proportions. Grass/caves, surfing, Rock Smash and every rod; an ability that picks
         the slot (Static...) still does. Burnt out, it asks to light another
         (`data/scripts/incense.inc`), like a Repel. Its icon is the hack's own (a censer and its
         smoke). The ITEMS pocket got 6 more slots for it (hold items 233, items 227: the
         berry trees' bytes split anew; `Bag_UpgradeLayout` moves a v100 save's).
   - [x] The Gym Charm (key item, from the mentor with the convention invitation; Oak hands it
         over for older saves): 1 in 250 Pokémon of the gym's current type (primary type) are
         Shiny (`GYM_CHARM_SHINY_ODDS`, `src/pokemon.c`). It follows the gym when it changes type.
   - [x] A safari zone per region (Hoenn, Sinnoh, Unova, Kalos, Alola, Galar, Paldea) where the
         Pokémon past the first 386 can be caught, with puzzles opening sub-areas with legendaries.
         All species through Gen 9 are already in the ROM (the convention and challengers use them)
         and the National Dex already counts 1025; the work is maps, encounter tables (~640
         species, ~30 wild slots per map) and about 70 legendary encounters.
         The 18 gym legendaries (`gGymLegends`) stay gym-only: no safari has them, Koraidon and
         Miraidon included.
   - [x] Map seams (v85): `tools/gym_hack/check_seams.py [out_dir]` finds tiles in the hack's own maps
         (the Exotic Safari, Pallet Town, the Academy) that meet a neighbor the way no FireRed, Emerald or
         HnS map ever has, and whose edges clash more than that tile's best original fit (a sea rock's
         ring cut off by sand, bushes standing in the sea); with out_dir, a marked-up picture of each
         map. Tiles are compared by their drawing, so copied, converted and recolored tilesets still
         match. First run: about 140 spots; most are fine as they are (a tree top over grass, a
         building on a lawn). Fixed: the Alola resort's beaches (`paint_beach` clears both kinds of
         sea rock; bushes only on the island's inside, never at the water), Skyarrow's beach corner,
         Pacifidlog and Route 131 joined the way Emerald joins them (the town west of the route: no
         cut rocks or cliff at the seam), and a cave mouth removed from a rock face is closed with
         the face, not floor (`remove_holes`).
   - [x] The tournament (v86), in place of the Gauntlet's one-after-another run (src/tournament.c,
         its board src/tournament_board.c; the story, Bill and the Palm Pilot stay in src/gauntlet.c
         and data/scripts/gauntlet.inc). The hooded figure is now a bookie running an underground
         tournament: 128 entrants, single elimination, seven wins for the title. Each one draws 127
         from the Gauntlet's 83 challengers and the convention's 127 trainers (no one twice: the
         convention's Larry, Lance and Iris are in two halls), plus the player. Seeding: by tier
         (`tier` in `gauntlet_roster.py`'s TIERS; the convention's champions 0, Elite Four 1, leaders
         2), at random within a tier, placed the standard way (seed 1 meets 128; the top two only in
         the final); the player is unseeded among tier 2. Only the seed is saved: the draw and every
         match without the player follow from it (`Tourney_BuildTree`; the stronger tier wins
         more often: weights 9/6/4/3), a round at a time once the player has played theirs.
         - Everyone at level 100 (`GymChallenge_SetChallengerLevel`; the convention's teams are
           75ish otherwise), hardest AI. The team is locked in when the player signs up (the
           personalities: the same Pokémon, no more, no fewer; any order); healed between rounds;
           prize money is the usual trainer battle's by class (a Leader/Elite Four/Champion pays
           ¥10,000 at level 100). A loss ends the run; the rest plays out, and the board shows how.
           A run can be forfeited. Sign up again for a new draw.
         - The board (the Battle Dome's tree, 16 names a page): blocks A-H (each sixteenth's first
           four rounds) and the final 16 (to the champion, named in its title once it's over); L/R
           turns the page. Lit lines: orange, the player's blue (BG2 under the tree's see-through
           lit tiles: `DomeTree_DrawLine` in src/battle_dome.c). From the bookie's menu, between
           rounds, after a loss, and "the last results".
         - The first title: the bookie says the one who put it on wants to meet the player; then
           Bill as before (he put on the tournament to find the strongest trainer; losing to him
           just means try again). After him, the Palm Pilot's battle simulator runs new
           tournaments (same rules, his records). A title there isn't the WITHDRAW yet: then comes
           Bill's simulation (his real team; TOURNEY_BILL, "Face BILL" from the Palm Pilot until
           he's beaten, any number of tries), and beating him earns it (`Tourney_SimBillWon`).
         - Bill's reveal is an experiment gone wrong: he tried to become a Clefable, the machine
           wouldn't stop, and he came out a hideous, powerful half-thing (CLEFFABILL, renamed from
           Clefabill in v88) that is slowly eating what's left of him; the hood hides it.
         - The TOURNAMENT Hall of Fame (PC and Palm Pilot, "TOURNAMENT", in the Hall of Fame's
           screen): the first title's team, kept for good, and the three most recent, each with
           the title's number and whom the final was against; the count of titles is kept too.
           Saved after the PC's storage in flash sector 31 (`struct TourneySave`, its own
           checksum; the sector's checksum is still the storage's, so older saves load as before).
         - The Bond Phenomenon and AZ's quests now wait for a Kalos trainer (AZ) beaten in a
           tournament, or a title (FLAG_TOURNEY_BEAT_KALOS 0x4F5, FLAG_TOURNEY_BEAT_AZ 0x4F6).
         - Found and fixed on the way: after Bill's last battle the game hung on the next warp
           (`fadeoutbgm` then `fadedefaultbgm` waits forever for a faded song to stop); the gym's
           music now just starts again (`Overworld_PlaySpecialMapMusic`).
         - Saves from the middle of an old Gauntlet run start the tournament fresh (VAR_GAUNTLET_NEXT
           is unused now); saves past it (Bill waiting, or the Palm Pilot) carry on.
   - [ ] Space: about 3.5 MB of the 32 MB is free (late October 2026). Ways to make room:
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
