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
    const struct TcgAttack *loadedAttack;              // wLoadedAttack
    u8 loadedAttackCardId;
    u8 selectedAttack;              // wSelectedAttack
    u8 playerAttackingCardIndex, playerAttackingCardId, playerAttackingAttackIndex;  // wPlayerAttacking*
    u16 damage;                     // wDamage (bit 15: UNAFFECTED_BY_WEAKNESS_RESISTANCE)
    u8 damageEffectiveness;         // wDamageEffectiveness
    u8 noDamageOrEffect;            // wNoDamageOrEffect
    u8 isDamageToSelf;              // wIsDamageToSelf
    u8 dealtDamage;                 // wDealtDamage
    u8 alreadyPlayedEnergy;         // wAlreadyPlayedEnergy
    u8 gotHeadsFromConfusionCheck;  // wGotHeadsFromConfusionCheck...
    u8 coinTossNumHeads, coinTossTotalNum;  // wCoinTossNumHeads, wCoinTossTotalNum
    u8 effectFunctionsFeedbackIndex;
    u8 afterDamageEffect;
    u8 statusConditionQueue[24];
    u8 statusConditionQueueIndex;
};

extern struct TcgDuel *const gTcg;

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

#endif // GUARD_TCG_H
