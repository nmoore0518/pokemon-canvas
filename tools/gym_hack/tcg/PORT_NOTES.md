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

## Next session: start here
- home/duel.asm 1339-1850 has been read (ProcessPlayedPokemonCard, CopyAttackDataAndDamage*,
  UpdateArenaCardIDsAndClearTwoTurnDuelVars, UseAttackOrPokemonPower, PlayAttackAnimation_DealAttackDamage,
  HandleAfterDamageEffects, HandleConfusionDamageToSelf, UsePokemonPower, ApplyTransparencyIfApplicable,
  CheckSelfConfusionDamage, PlayTrainerCard, LoadNonPokemonCardEffectCommands,
  DealRecoilDamageToSelf / DealConfusionDamageToSelf) but not yet ported. They need, from elsewhere:
  TryExecuteEffectCommandFunction / CheckMatchingCommand (home), the substatus handlers
  (HandleNoDamageOrEffectSubstatus, HandleTransparency, CheckSandAttackOrSmokescreenSubstatus,
  HandleStrikesBack_AgainstResidualAttack, HandleDestinyBondAndBetweenTurnKnockOuts: engine/duel/core.asm),
  ApplyStatusConditionQueue, UpdateArenaCardLastTurnDamage, ClearNonTurnTemporaryDuelvars(_CopyStatus),
  TossCoin, CheckCantUseTrainerDueToEffect, CheckIsIncapableOfUsingPkmnPower, ClearChangedTypesIfMuk.
- Then home/duel.asm 1848-2400 (damage modifiers, SubtractHP, knock-out text, DealDamageToPlayAreaPokemon,
  retreat cost, CheckLoadedAttackFlag) and on through the list below.

## To do (in order)
1. home/duel.asm 1221-2400: GetPlayAreaCardAttachedEnergies, CountCardIDInLocation,
   ProcessPlayedPokemonCard, CopyAttackDataAndDamage, UseAttackOrPokemonPower, damage and
   knock-outs (DealDamageToPlayAreaPokemon, ApplyDamageModifiers_*, SubtractHP), confusion, recoil,
   PlayTrainerCard, retreat cost, CheckLoadedAttackFlag.
2. engine/duel/core.asm: the duel loop (MainDuelLoop, HandleTurn, HandleDuelSetup, between turns,
   prizes, knock-outs, sudden death), the player's menus (hand, attack, retreat, check).
3. engine/duel/effect_functions.asm: the 189 routines in include/tcg_effect_functions.h.
4. engine/duel/ai/: Rod's AI (decks/legendary_dragonite.asm and the general AI it uses).
5. The screens (the game's layouts at 160x144 in the middle of the GBA's), its fonts and gfx.
6. The Saffron house, Rod, the deck picker, the energy-card prizes (src/energy_cards.c: done).
