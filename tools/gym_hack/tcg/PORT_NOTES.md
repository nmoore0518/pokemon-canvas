# The Trading Card Game duel: port notes

Pokémon Canvas's TCG mini-game (Grandmaster Rod, in Saffron) is a port of the Game Boy Color
game's duel, from the pret/poketcg disassembly (`git clone https://github.com/pret/poketcg`).
Nick's brief: a faithful recreation, with the original art; the original 160x144 screens centred
on the GBA's with a border.

## How it's laid out
- `tools/gym_hack/tcg/extract_cards.py <poketcg>`: the cards in the 8 decks (Rod's Legendary
  Dragonite, the six club masters', a Colorless deck of ours), their text, effect commands, decks
  → `src/data/tcg/*.h`, `include/constants/tcg_cards.h`, `include/tcg_effect_functions.h`, and
  `src/tcg/effects_stubs.c` (every effect routine as a weak stub until it's ported).
- `tools/gym_hack/tcg/extract_constants.py <poketcg>`: the game's constants → `include/constants/tcg.h`
  (TCG_ prefixed; DUELVARS_* at the game's own offsets).
- `include/tcg.h`: the duel state (`gTcg`: duel variables as a 256-byte page per duelist, like the
  game's WRAM) and the API.
- `graphics/tcg/cards/`: the card art (the game's gfx/cards, with its GBC palettes).

## Conventions
- Functions keep the game's names, prefixed `Tcg_`. "Returns carry" becomes `return TRUE`.
- Deck indexes 0-59 and `gTcg->whoseTurn` work as `hWhoseTurn` does: `Tcg_TurnVar(DUELVARS_X)` is
  `GetTurnDuelistVariable`.

## Ported
- home/duel.asm, lines 60-1340: CopyDeckData (n/a), CountPrizes, ShuffleDeck/ShuffleCards,
  DrawCardFromDeck, ReturnCardToDeck, SearchCardInDeckAndAddToHand, AddCardToHand,
  RemoveCardFromHand, MoveHandCardToDiscardPile, PutCardInDiscardPile, MoveDiscardPileCardToHand,
  CheckPrizeTaken, Create{DiscardPile,Deck,ArenaOrBenchEnergy,Hand}CardList, SortHandCardsByID,
  SortCardsInListByID, RemoveCardFromDuelTempList, CountCardsInDuelTempList, GetCardIDFromDeckIndex,
  LoadCardDataToBuffer1/2 (ConvertSpecialTrainerCardToPokemon: not needed, no Clefairy Doll or
  Mysterious Fossil in these decks), EvolvePokemonCard(IfPossible), CheckIfCanEvolveInto(_BasicToStage2),
  ClearAllStatusConditions, PutHand(Pokemon)CardInPlayArea, MovePlayAreaCardToDiscardPile,
  EmptyPlayAreaSlot, Shift*PokemonToFirstPlayAreaSlots, SwapArenaWithBenchPokemon,
  SwapPlayAreaPokemon, Get(Non)TurnDuelistVariable.

## Design for the rest
- The game's rules code calls screens and text directly (DrawWideTextBox_WaitForInput, animations,
  TossCoin's coin screen). The port keeps the logic exactly and routes those through a small UI hook
  layer (`TcgUi_*`: message, coin toss, animation, card list / play area selection), so the duel can
  also run headless: for unit tests, and for AI-vs-AI runs to check each player deck is a fair fight
  against Rod.
- Link-duel calls (SetOppAction_SerialSendDuelData, ExchangeRNG, SendAttackDataToLinkOpponent) are
  dropped: only the AI opponent exists here.

- src/tcg/duel_attack.c: the rest of home/duel.asm (GetPlayAreaCardAttachedEnergies through
  CheckLoadedAttackFlag: ProcessPlayedPokemonCard, CopyAttackDataAndDamage*, UseAttackOrPokemonPower,
  PlayAttackAnimation_DealAttackDamage, HandleAfterDamageEffects, confusion and recoil, PlayTrainerCard,
  ApplyDamageModifiers_*, PlusPower/Defender, SubtractHP, knock-out text, DealDamageToPlayAreaPokemon,
  PrintFailedEffectText, retreat cost), all of home/substatus.asm, card_color.asm, effect_commands.asm,
  coin_toss.asm, and from engine/duel/core.asm: ApplyStatusConditionQueue, HandleDestinyBondAndBetweenTurnKnockOuts
  (prizes, KO replacement, the win/lose/tie table), ClearNonTurnTemporaryDuelvars, UpdateArenaCardLastTurnDamage,
  _TossCoin's logic, PrintThereWasNoEffectFromStatusText; AIPickPrizeCards (engine/duel/ai/core.asm).
- The duel's text: tools/gym_hack/tcg/extract_text.py → include/constants/tcg_text.h (TCG_TEXT_<name>,
  the game's label without "Text") and src/data/tcg/text.h (the 354 texts home/ and engine/duel/ use).
  Tags kept in the strings: <RAMNAME>, <RAMTEXT> (gTcg->txRam2[0] then [1]; NULL: defaultText),
  <RAMNUM> (txRam3[0], [1]), energy symbols. Routines that return a text id in hl set gTcg->hlText.
- include/tcg_ui.h / src/tcg/ui.c: the screen hooks, under the game's names (no-ops for now).
- test/tcg.c: damage math (weakness, resistance, PlusPower, Defender, Invisible Wall, the "sla d"
  halving, Growl, double damage), SubtractHP, attached energy, a whole attack with knock-out, prize,
  bench replacement and the win, the status queue.
- Left out (cards not in our decks, so their checks can't be true): Muk's Toxic Gas, Kabuto Armor,
  Mew Lv8's Neutralizing Shield, Haunter Lv17's Transparency, Venomoth's Shift, Dodrio's Retreat
  Aid, Aerodactyl's Prehistoric Power, Omanyte's Clairvoyance, Clefairy Doll / Mysterious Fossil.

- src/tcg/duel_core.c (engine/duel/core.asm): StartDuel + MainDuelLoop (sudden death on a tie),
  HandleTurn (draw: an empty deck loses), HandleDuelSetup (mulligans, placing Pokémon, prizes, the
  first-player coin), ShuffleDeckAndDrawSevenCards, InitializeDuelVariables, InitTurnDuelistPrizes,
  HandleBetweenTurnsEvents (poison, sleep, paralysis; PlusPower and Defender discarded), the duel menu
  (PrintDuelMenuAndHandleInput, DuelMenu_Hand/Attack/PkmnPower/Retreat/Done, PlayEnergyCard with Rain
  Dance, PlayPokemonCard, CheckAbleToRetreat, DisplayRetreatScreen's choosing, AttemptRetreat,
  CheckIfEnoughEnergiesForGivenAttack, PrintAndLoadAttacksToDuelTempList, DisplayPlayAreaScreenToUsePkmnPower),
  and AIMakeDecision with the OppAction_* routines the AI drives.
- The design: like the game, the rules code waits for its screens (blocking calls), so it reads as the
  game does. On the GBA the duel will run as a loop of its own: the UI's waits do the frames (read
  keys, sprites, wait for VBlank; sound runs in the VBlank interrupt).
- The player's choices are UI hooks returning a choice or TCG_SELECTION_CANCEL (the game's carry).
- test/tcg.c plays each of the 7 player decks against Rod's, AI vs AI, headless, to the end.

- src/tcg/effects.c (engine/duel/effect_functions.asm): all 189 effect routines our cards use, and
  the helpers they call (QueueStatusCondition with Snorlax's Thick Skinned, ApplyAndAnimateHPRecovery,
  ApplyExtraWaterEnergyDamageBonus, LookForCardsInDeck, the AI picks for energy/Amnesia/bench targets,
  HandlePokemonAndEnergySelectionScreen, HandlePlayerSelection2HandCards, Mirror Move...). The game's
  HRAM union at $ffa0 (hTemp_ffa0, hTempPlayAreaLocation_ffa1, hTempRetreatCostCards, hTempList,
  hAIPkmnPowerEffectParam...) is a union in struct TcgDuel too, since the routines rely on the aliasing.

- The AI (engine/duel/ai/), include/tcg_ai.h: struct TcgAi is the game's wAI* WRAM, one per
  duelist (sTcgAi[2]; gTcgAi: the one acting, set by TcgAi_DoAction from whoseTurn), so the AI can
  play both sides headless. deckId is the game's deck ID (TCG_DECKID_*; TcgAi_SetDeck).
  - src/tcg/ai_core.c: core.asm, damage_calculation.asm, init.asm. ai_common.c: common.asm.
    ai_attacks.c: attacks.asm, special_attacks.asm. ai_energy.c: energy.asm. ai_pokemon.c:
    hand_pokemon.asm, retreat.asm. ai_trainer_cards.c: trainer_cards.asm and the logic table.
    ai.c: deck_ai.asm (AIDoAction), decks/legendary_dragonite.asm, decks/general.asm,
    boss_deck_set_up.asm.
  - Rod plays decks/legendary_dragonite.asm: his lists, the boss opening hand and play area, his
    Pokémon Trader trades, Kangaskhan's first Energy. The player's decks (only ever AI-driven in the
    headless runs) get decks/general.asm's AIMainTurnLogic: the game's own opponents for those decks
    had per-deck logic that isn't ported (it never plays against the player here).
  - Kept bugs: the avoid-prize check in SetUpBossStartingHandAndDeck ('cp a': does nothing); Rod's
    retreat list never stored; Professor Oak's "+10 for a non-Pokémon" counts the first Basic-stage
    Energy or Trainer; the energy-bonus list bug in ProcessEnergyCards.
  - Not ported: the AI's Pokémon Powers (pkmn_powers.asm: Energy Trans, Rain Dance, Damage Swap, Peek,
    Heal, Cowardice: TcgAi_HandleAIPkmnPowers / HandleAIEnergyTrans are no-ops; only the player's
    decks have those Powers). The trainer logic table only has Rod's Trainers (Professor Oak,
    Pokémon Trader, Breeder, Energy Retrieval, Super Energy Retrieval, Switch, Gambler; Gambler's
    AIDecide is FALSE for Rod in the game too): in the headless runs the player's side doesn't play
    the other Trainers. Checks for cards not in our decks (Mewtwo mill, Mr. Mime, Mysterious Fossil...)
    are left out as elsewhere.
  - Headless AI-vs-AI (70 duels): Rod won about half, against the general AI on the player's decks,
    with 2-9 Trainers played a duel; every card accounted for at the end.
  - Tests: Rod's opening hand (2+2, 4+4), his Professor Oak decision both ways, an attack.

- The screens (in progress): the duel draws as the GBC game did, onto an emulated GBC display
  (include/tcg_gbc.h, src/tcg/gbc.c): its VRAM ($8000-$9FFF, two banks: tiles, BG maps and CGB
  attributes), OAM, SCX/SCY/WX/WY, LCDC, CGB palettes (the same BGR555 as the GBA's). Shown on the
  GBA in mode 0: BG0 the BG map, BG1 the window (in WIN0), BG2 white (the LCD off), OBJ the sprites;
  WIN1 is the 160x144 in the middle, the border the backdrop outside it. GBC tiles convert to 4bpp as
  they're written (BG: colours 0-3 as 1-4, so 0 stays opaque; OBJ: as is); the BG maps to GBA entries
  as they're written, uploaded at VBlank; OAM at VBlank when the game asks (wVBlankOAMCopyToggle).
  The duel runs as its own loop (blocking, like the game): Tcg_DoFrame waits for the VBlank.
  - Ported on it, under the game's names: home/lcd, frames, input, palettes, setup, objects, bg_map,
    tiles (gbc.c); process_text, print_text, text_box, write_number (text.c: the half-width tile
    pairs and their most-recently-used list, TX_RAM1/2/3 through the text headers); menus (menus.c);
    engine/copy_card_name.asm (duel_attack.c).
  - The text is the game's own bytes now (tools/gym_hack/tcg/tcg_charmap.py: its charmaps and text
    macros), and the graphics its own (tools/gym_hack/tcg/extract_gfx.py: rgbgfx's conversion, laid
    out as gfx.asm is: gTcgFontsGfx + TCG_GFX_OFFSET_<label>, gTcgCardGfx).
  - The hooks in include/tcg_ui.h: src/tcg/ui.c's stand-ins are weak; a ported screen replaces its
    stand-in, and returns at once (or the stand-in's choice) when gTcg->headless.
  - Not decided yet: the sound (the GBC game's music on the GBA's GB sound channels, or FireRed's).

  - Screens ported so far: SetupDuel, the main scene and HUDs (duel_screens.c); the duel menu and
    its shortcuts, box messages, the turn screen, the card lists (hand, placing Pokémon, discard
    piles, Select/Check and Play/Check) and the graphics loaders of home/tiles.asm (duel_lists.c);
    the card pages, the large card picture, DisplayCardDetailScreen (card_page.c).
  - The way in: special StartTcgDuel (VAR_0x8004 the deck) -> CB2_TcgDuel (src/tcg/tcg_main.c).
    To look at it in the emulator: boot, then poke gMain.callback2 = CB2_TcgDuel | 1 (the scratchpad's
    tcg_run.py does that and takes screenshots from a key script).

## Next session: start here
- Still stand-ins (src/tcg/ui.c, weak): the play area screens (engine/menus/play_area.asm and
  core.asm's PrintPlayAreaCardList, OpenPlayAreaScreenForSelection/Viewing,
  PrintPlayAreaCardInformationAndLocation, DrawYourOrOppPlayAreaScreen), the attack menu
  (DuelMenu_Attack's screen, HandleAttackMenuInput, the attack pages), retreat/energy discard
  screens, the duel check menu, prizes (DrawPlayAreaToPlacePrizeCards, SelectPrizeCards), the
  draw-card and shuffle animations, the coin toss, the AI's "thinking" box and delays, the duel
  result screen, and the attack/status animations (engine/duel/animations, engine/gfx sprite
  animations). Then the sound, the Saffron house and Rod, the deck picker, the energy prizes.

## To do (in order)
1. ~~home/duel.asm 1221-2400~~ (done).
2. ~~engine/duel/core.asm: the duel loop and the menus' rules~~ (done; the menus' screens are part of 5).
3. ~~engine/duel/effect_functions.asm: the 189 routines~~ (done).
4. ~~engine/duel/ai/: Rod's AI~~ (done; see above for what isn't).
5. The screens (the game's layouts at 160x144 in the middle of the GBA's), its fonts and gfx.
6. The Saffron house, Rod, the deck picker, the energy-card prizes (src/energy_cards.c: done).
