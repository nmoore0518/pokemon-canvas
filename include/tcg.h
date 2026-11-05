#ifndef GUARD_TCG_H
#define GUARD_TCG_H

// Pokémon Canvas: the Pokémon Trading Card Game duel (src/tcg/), ported from the Game Boy Color game
// (pret/poketcg) to stay faithful to it: the same duel variables at the same offsets (a 256-byte
// page per duelist, DUELVARS_* in constants/tcg.h), the same helpers, the same effect routines.
// tools/gym_hack/tcg/PORT_NOTES.md says what's ported from where.
#include "constants/tcg.h"
#include "constants/tcg_cards.h"

#define TCG_MAX_PLAY_AREA       6
#define TCG_NUM_ENERGY_TYPES    7   // (FIRE..COLORLESS)
#define TCG_DUEL_TEMP_LIST_SIZE 80

// (the duelists: hWhoseTurn)
#define TCG_PLAYER   0
#define TCG_OPPONENT 1

struct TcgAttack
{
    u8 cost[TCG_NUM_ENERGY_TYPES];
    const u8 *name;
    const u8 *description[2];
    u8 damage;
    u8 category;
    u8 effects;         // (TCG_EFFECTS_*: gTcgEffectCommands)
    u8 flags[3];
    u8 extra;
    u8 animation;
};

struct TcgCard
{
    u8 type;
    const u8 *name;
    const u8 *description;
    u16 gfx;
    // Pokémon
    u8 hp;
    u8 stage;
    const u8 *preEvoName;
    struct TcgAttack attacks[2];
    u8 retreatCost;
    u8 weakness;
    u8 resistance;
    const u8 *category;
    u16 dexNumber;
    u8 level;
    u8 length[2];
    u16 weight;
    u8 aiInfo;
    // Trainer and Energy
    u8 effects;
    const u8 *description2;
};

struct TcgEffectCommand
{
    u8 type;            // (TCG_EFFECTCMDTYPE_*)
    bool32 (*func)(void);   // (TRUE: "carry": failed / can't)
};

extern const struct TcgCard gTcgCards[TCG_NUM_CARDS];
extern const u8 *const gTcgTexts[];   // (TCG_TEXT_*: constants/tcg_text.h)
extern const struct TcgEffectCommand *const gTcgEffectCommands[TCG_NUM_EFFECTS];
extern const u8 *const gTcgDecks[TCG_NUM_DUEL_DECKS];

// The duel's state (the game's WRAM, as much of it as the duel uses)
struct TcgDuel
{
    u8 vars[2][256];                // wPlayerDuelVariables, wOpponentDuelVariables (DUELVARS_*)
    u8 deck[2][TCG_DECK_SIZE];      // wPlayerDeck, wOpponentDeck: each deck index's card (TCG_CARD_*)
    u8 whoseTurn;                   // hWhoseTurn (TCG_PLAYER, TCG_OPPONENT)
    u8 duelTempList[TCG_DUEL_TEMP_LIST_SIZE];   // wDuelTempList ($ff-terminated)
    u8 duelTurns;                   // wDuelTurns
    u8 duelFinished;                // wDuelFinished
    u8 duelInitialPrizes;           // wDuelInitialPrizes
    u8 tempCardIndex;               // hTempCardIndex_ff98
    u8 tempPlayAreaLocation;        // hTempPlayAreaLocation_ff9d
    u8 tempPlayAreaLocationFfa1;    // hTempPlayAreaLocation_ffa1
    u8 tempList[16];                // hTempList
    u8 preEvolutionPokemonCard;     // wPreEvolutionPokemonCard
    const struct TcgCard *loadedCard1, *loadedCard2;   // wLoadedCard1, wLoadedCard2
    u8 loadedCard1Id, loadedCard2Id;
    struct TcgAttack loadedAttack;  // wLoadedAttack (a copy: the duel changes its animation)
    const struct TcgEffectCommand *loadedAttackEffectCommands;  // wLoadedAttackEffectCommands
    u8 selectedAttack;              // wSelectedAttack
    u8 playerAttackingCardIndex, playerAttackingCardId, playerAttackingAttackIndex;  // wPlayerAttacking*
    u16 damage;                     // wDamage (bit 15: UNAFFECTED_BY_WEAKNESS_RESISTANCE)
    u8 damageEffectiveness;         // wDamageEffectiveness (1 << WEAKNESS, 1 << RESISTANCE)
    u8 noDamageOrEffect;            // wNoDamageOrEffect (NO_DAMAGE_OR_EFFECT_*; bit 7: its text shown)
    u8 isDamageToSelf;              // wIsDamageToSelf
    u16 dealtDamage;                // wDealtDamage
    u8 alreadyPlayedEnergy;         // wAlreadyPlayedEnergy
    u8 whoseTurnReal;               // wWhoseTurn (whose turn it really is: whoseTurn is swapped about)
    u8 tempTurnDuelistCardId;       // wTempTurnDuelistCardID
    u8 tempNonTurnDuelistCardId;    // wTempNonTurnDuelistCardID
    u8 tempCardId;                  // wTempCardID_ccc2
    u8 tempCardIndexFf9f;           // hTempCardIndex_ff9f
    u8 tempPlayAreaLocationCceb;    // wTempPlayAreaLocation_cceb
    u8 temp;                        // hTemp_ffa0
    u8 effectFailed;                // wEffectFailed (EFFECT_FAILED_*)
    u8 defendingWasForcedToSwitch;  // wDefendingWasForcedToSwitch
    u8 metronomeEnergyCost;         // wMetronomeEnergyCost
    u8 noEffectFromWhichStatus;     // wNoEffectFromWhichStatus
    u8 gotHeadsFromSandAttackOrSmokescreenCheck;
    u8 confusionAttackCheckWasUnsuccessful;
    u8 attachedEnergies[TCG_NUM_TYPES];    // wAttachedEnergies (Tcg_GetPlayAreaCardAttachedEnergies)
    u8 totalAttachedEnergies;              // wTotalAttachedEnergies
    u8 arenaCardLastTurnStatus[2];         // wPlayerArenaCardLastTurnStatus, wOpponent...
    u8 duelFinishParam;             // wDuelFinishParam
    u8 numberPrizeCardsToTake;      // wNumberPrizeCardsToTake
    u8 coinTossNumHeads, coinTossTotalNum, coinTossNumTossed;  // wCoinToss*
    u8 statusConditionQueue[25];    // wStatusConditionQueue: 3 bytes each (TCG_QUEUE_DUELIST, keep, add)
    u8 statusConditionQueueIndex;   // wStatusConditionQueueIndex
    // the text's arguments (the game's wTxRam2, wTxRam2_b, wTxRam3, wTxRam3_b): <RAMTEXT>s print
    // txRam2[0] then [1] (NULL: defaultText), <RAMNUM>s txRam3[0] then [1]
    const u8 *txRam2[2];
    u16 txRam3[2];
    u8 defaultText[24];             // wDefaultText
    u16 hlText;                     // (the text id the game's routines return in hl, for their caller)
    u16 deText;                     // (and in de: a coin toss's text)
    u8 confusionRetreatCheckWasUnsuccessful;   // wConfusionRetreatCheckWasUnsuccessful
    u8 tempRetreatCostCards[8];     // hTempRetreatCostCards ($ff-terminated)
    u8 energyCardsRequiredToRetreat, numRetreatEnergiesSelected;
    u8 selectedDuelSubMenuItem;     // wSelectedDuelSubMenuItem
    u8 duelistType;                 // wDuelistType (the turn holder's)
    u8 skipDuelistIsThinkingDelay;  // wSkipDuelistIsThinkingDelay
    u8 opponentTurnEnded;           // wOpponentTurnEnded
    u8 duelResult;                  // wDuelResult (TCG_DUEL_WIN, TCG_DUEL_LOSS)
    bool8 headless;                 // (no screens, no waiting: the tests, AI-vs-AI runs)
};

extern struct TcgDuel *const gTcg;

// a status condition queue entry's duelist (the game's PLAYER_TURN/OPPONENT_TURN: never 0)
#define TCG_QUEUE_DUELIST(duelist) (0xC2 + (duelist))

// src/tcg/duel_home.c: the game's home/duel.asm
u8 *Tcg_TurnVar(u32 offset);
u8 *Tcg_NonTurnVar(u32 offset);
void Tcg_SwapTurn(void);
u32 Tcg_GetCardIdFromDeckIndex(u32 deckIndex);
const struct TcgCard *Tcg_Card(u32 cardId);
void Tcg_LoadCard1(u32 deckIndex);
void Tcg_LoadCard2(u32 deckIndex);
u32 Tcg_CountPrizes(void);
void Tcg_ShuffleDeck(void);
bool32 Tcg_DrawCardFromDeck(u8 *deckIndex);
void Tcg_ReturnCardToDeck(u32 deckIndex);
void Tcg_SearchCardInDeckAndAddToHand(u32 deckIndex);
void Tcg_AddCardToHand(u32 deckIndex);
void Tcg_RemoveCardFromHand(u32 deckIndex);
void Tcg_MoveHandCardToDiscardPile(u32 deckIndex);
void Tcg_PutCardInDiscardPile(u32 deckIndex);
void Tcg_MoveDiscardPileCardToHand(u32 deckIndex);
bool32 Tcg_IsPrizeTaken(u32 prize);
bool32 Tcg_CreateDiscardPileCardList(void);
bool32 Tcg_CreateDeckCardList(void);
u32 Tcg_CreateArenaOrBenchEnergyCardList(u32 location);
bool32 Tcg_CreateHandCardList(void);
void Tcg_SortHandCardsByID(void);
void Tcg_SortCardsInDuelTempListByID(void);
bool32 Tcg_RemoveCardFromDuelTempList(u32 deckIndex);
u32 Tcg_CountCardsInDuelTempList(void);
bool32 Tcg_CheckIfCanEvolveInto(u32 location, u32 deckIndex, bool32 *playedThisTurn);
bool32 Tcg_CheckIfCanEvolveInto_BasicToStage2(u32 location, u32 deckIndex);
bool32 Tcg_EvolvePokemonCardIfPossible(void);
void Tcg_EvolvePokemonCard(void);
void Tcg_ClearAllStatusConditions(void);
bool32 Tcg_PutHandPokemonCardInPlayArea(u32 deckIndex);
void Tcg_PutHandCardInPlayArea(u32 deckIndex, u32 location);
void Tcg_MovePlayAreaCardToDiscardPile(u32 location);
void Tcg_EmptyPlayAreaSlot(u32 location);
void Tcg_ShiftAllPokemonToFirstPlayAreaSlots(void);
void Tcg_ShiftTurnPokemonToFirstPlayAreaSlots(void);
void Tcg_SwapArenaWithBenchPokemon(u32 location);
void Tcg_SwapPlayAreaPokemon(u32 a, u32 b);
u32 Tcg_Random(u32 n);

// src/tcg/duel_attack.c: attacks, damage and knock-outs (home/duel.asm, home/substatus.asm,
// home/card_color.asm, home/effect_commands.asm, home/coin_toss.asm, parts of engine/duel/core.asm)
void Tcg_LoadTxRam2(const u8 *text);
void Tcg_LoadTxRam3(u16 n);
void Tcg_CopyCardNameAndLevel(void);
void Tcg_GetPlayAreaCardAttachedEnergies(u32 location);
u32 Tcg_CountCardIDInLocation(u32 location, u32 cardId);
bool32 (*Tcg_CheckMatchingCommand(const struct TcgEffectCommand *commands, u32 type))(void);
bool32 Tcg_TryExecuteEffectCommandFunction(u32 type);
void Tcg_ProcessPlayedPokemonCard(void);
void Tcg_CopyAttackDataAndDamage_FromCardID(u32 cardId, u32 deckIndex, u32 attack);
void Tcg_CopyAttackDataAndDamage_FromDeckIndex(u32 deckIndex, u32 attack);
void Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars(void);
bool32 Tcg_UseAttackOrPokemonPower(void);
bool32 Tcg_PlayAttackAnimation_DealAttackDamage(void);
bool32 Tcg_HandleAfterDamageEffects(void);
u16 Tcg_ApplyTransparencyIfApplicable(u16 damage);
bool32 Tcg_CheckSelfConfusionDamage(void);
bool32 Tcg_PlayTrainerCard(void);
void Tcg_LoadNonPokemonCardEffectCommands(void);
void Tcg_DealRecoilDamageToSelf(u32 damage);
void Tcg_DealConfusionDamageToSelf(u32 damage);
void Tcg_PlayAttackAnimation_DealAttackDamageSimple(u32 duelist, u32 location, u32 effectiveness, u8 *hp, u16 damage);
u16 Tcg_ApplyDamageModifiers_DamageToTarget(void);
u32 Tcg_TranslateColorToWR(u32 color);
u16 Tcg_ApplyDamageModifiers_DamageToSelf(void);
u16 Tcg_ApplyAttachedPlusPower(u32 location, u16 damage);
u16 Tcg_ApplyAttachedDefender(u32 location, u16 damage);
bool32 Tcg_SubtractHP(u8 *hp, u16 damage);
bool32 Tcg_PrintPlayAreaCardKnockedOutIfNoHP(u32 location);
void Tcg_PrintKnockedOutIfHLZero(const u8 *hp);
void Tcg_PrintKnockedOut(void);
void Tcg_DealDamageToPlayAreaPokemon_RegularAnim(u32 location, u16 damage);
void Tcg_DealDamageToPlayAreaPokemon(u32 location, u16 damage);
void Tcg_DrawDuelMainScene_PrintPokemonsAttackText(void);
void Tcg_PrintPokemonsAttackText(void);
bool32 Tcg_PrintFailedEffectText(void);
u32 Tcg_GetPlayAreaCardRetreatCost(void);
void Tcg_MoveCardToDiscardPileIfInPlayArea(u32 cardId);
u32 Tcg_GetCardDamageAndMaxHP(u32 location, u32 *maxHp);
bool32 Tcg_CheckLoadedAttackFlag(u32 flag);
u16 Tcg_HandleDoubleDamageSubstatus(u16 de);
u16 Tcg_HandleDamageReduction(u16 de);
u16 Tcg_HandleDamageReductionExceptSubstatus2(u16 de);
u16 Tcg_HandleDamageReductionOrNoDamageFromPkmnPowerEffects(u16 de);
void Tcg_HandleStrikesBack_AgainstDamagingAttack(u16 de);
bool32 Tcg_HandleCantAttackSubstatus(void);
bool32 Tcg_HandleAmnesiaSubstatus(void);
bool32 Tcg_HandleSandAttackOrSmokescreenSubstatus(void);
bool32 Tcg_CheckSandAttackOrSmokescreenSubstatus(void);
bool32 Tcg_HandleNoDamageOrEffectSubstatus(void);
bool32 Tcg_CheckNoDamageOrEffect(void);
bool32 Tcg_CheckIsIncapableOfUsingPkmnPower(u32 location);
u32 Tcg_CountTurnDuelistPokemonWithActivePkmnPower(u32 cardId);
u32 Tcg_CountPokemonWithActivePkmnPowerInBothPlayAreas(u32 cardId);
u32 Tcg_GetLoadedCard1RetreatCost(void);
bool32 Tcg_CheckUnableToRetreatDueToEffect(void);
bool32 Tcg_CheckCantUseTrainerDueToEffect(void);
bool32 Tcg_IsPrehistoricPowerActive(void);
bool32 Tcg_IsClairvoyanceActive(void);
void Tcg_ClearDamageReductionSubstatus2(void);
void Tcg_UpdateSubstatusConditions_StartOfTurn(void);
void Tcg_UpdateSubstatusConditions_EndOfTurn(void);
bool32 Tcg_IsRainDanceActive(void);
bool32 Tcg_CheckRainDanceScenario(void);
void Tcg_HandleDestinyBondSubstatus(void);
void Tcg_HandleStrikesBack_AgainstResidualAttack(void);
u32 Tcg_GetPlayAreaCardColor(u32 location);
u32 Tcg_GetArenaCardColor(void);
u32 Tcg_GetArenaCardWeakness(void);
u32 Tcg_GetPlayAreaCardWeakness(u32 location);
u32 Tcg_GetArenaCardResistance(void);
u32 Tcg_GetPlayAreaCardResistance(u32 location);
void Tcg_HandleEnergyBurn(void);
u32 Tcg_TossCoinATimes(u32 count, u16 textId);
bool32 Tcg_TossCoin(u16 textId);
bool32 Tcg_ApplyStatusConditionQueue(void);
void Tcg_ApplyStatusConditionToArenaPokemon(const u8 *entry);
void Tcg_TakePrizeCard(u32 prize);
void Tcg_AIPickPrizeCards(void);
bool32 Tcg_HandleDestinyBondAndBetweenTurnKnockOuts(void);
bool32 Tcg_HandleBetweenTurnKnockOuts(void);
bool32 Tcg_HandleConfusionDamageToSelf(void);
void Tcg_ClearNonTurnTemporaryDuelvars(void);
void Tcg_UpdateArenaCardLastTurnDamage(void);
u16 Tcg_PrintThereWasNoEffectFromStatusText(void);

// src/tcg/duel_core.c: the duel's loop, setup, between turns, the duel menu's rules, the AI's actions
// (engine/duel/core.asm)
void Tcg_InitializeDuelVariables(void);
void Tcg_InitTurnDuelistPrizes(void);
void Tcg_InitVariablesToBeginDuel(void);
void Tcg_InitVariablesToBeginTurn(void);
void Tcg_SetAllPlayAreaPokemonCanEvolve(void);
bool32 Tcg_ShuffleDeckAndDrawSevenCards(void);
void Tcg_HandleDuelSetup(void);
void Tcg_HandleBetweenTurnsEvents(void);
bool32 Tcg_CheckIfActiveCardParalyzedOrAsleep(void);
bool32 Tcg_HasNoAlivePokemonInBench(void);
bool32 Tcg_CheckIfEnoughEnergiesToRetreat(void);
bool32 Tcg_CheckAbleToRetreat(void);
void Tcg_DiscardRetreatCostCards(void);
bool32 Tcg_AttemptRetreat(void);
bool32 Tcg_CheckIfEnoughEnergiesForGivenAttack(u32 deckIndex, u32 attack);
u32 Tcg_PrintAndLoadAttacksToDuelTempList(void);
u32 Tcg_StartDuel(void);
void Tcg_SetDecks(const u8 *playerDeck, const u8 *opponentDeck);
bool32 Tcg_AIMakeDecision(u32 action);

// src/tcg/ai.c: the AI opponent (engine/duel/ai/)
u32 TcgAi_KOSwitch(void);       // (AIDoAction_KOSwitch: the bench Pokémon to replace a knocked-out one)
void TcgAi_StartDuel(void);     // (AIDoAction_StartDuel: its Active and Benched Pokémon)
void TcgAi_DoTurn(void);        // (AIDoAction_Turn)

#endif // GUARD_TCG_H
