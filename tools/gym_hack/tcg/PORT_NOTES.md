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
  src/tcg/ai.c: TcgAi_KOSwitch stand-in (weak) until the AI's ported.
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
- src/tcg/ai.c: a plain stand-in AI (weak symbols), enough to run whole duels headless: test/tcg.c
  plays each of the 7 player decks against Rod's, AI vs AI, to the end.

- src/tcg/effects.c (engine/duel/effect_functions.asm): all 189 effect routines our cards use, and
  the helpers they call (QueueStatusCondition with Snorlax's Thick Skinned, ApplyAndAnimateHPRecovery,
  ApplyExtraWaterEnergyDamageBonus, LookForCardsInDeck, the AI picks for energy/Amnesia/bench targets,
  HandlePokemonAndEnergySelectionScreen, HandlePlayerSelection2HandCards, Mirror Move...). The game's
  HRAM union at $ffa0 (hTemp_ffa0, hTempPlayAreaLocation_ffa1, hTempRetreatCostCards, hTempList,
  hAIPkmnPowerEffectParam...) is a union in struct TcgDuel too, since the routines rely on the aliasing.

## Next session: start here
- engine/duel/ai/: Rod's AI (and the player-deck AI used for headless AI-vs-AI runs). The game's AI
  is generic (engine/duel/ai/core.asm, attacks.asm, energy.asm, trainer_cards.asm, retreat.asm,
  boss_deck_set_up.asm, special_attacks.asm, pkmn_powers.asm...) plus per-deck tables
  (decks/legendary_dragonite.asm). It drives the duel through Tcg_AIMakeDecision. src/tcg/ai.c's
  stand-in (weak) goes once it's ported.

## To do (in order)
1. ~~home/duel.asm 1221-2400~~ (done).
2. ~~engine/duel/core.asm: the duel loop and the menus' rules~~ (done; the menus' screens are part of 5).
3. ~~engine/duel/effect_functions.asm: the 189 routines~~ (done).
4. engine/duel/ai/: Rod's AI (decks/legendary_dragonite.asm and the general AI it uses).
5. The screens (the game's layouts at 160x144 in the middle of the GBA's), its fonts and gfx.
6. The Saffron house, Rod, the deck picker, the energy-card prizes (src/energy_cards.c: done).
