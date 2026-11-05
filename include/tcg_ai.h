#ifndef GUARD_TCG_AI_H
#define GUARD_TCG_AI_H

// Pokémon Canvas: the Trading Card Game duel's AI opponent (src/tcg/ai_*.c), ported from the Game
// Boy Color game's engine/duel/ai/ (pret/poketcg) under the same names (TcgAi_ prefixed). Its WRAM
// (wAIDuelVars and the rest) is struct TcgAi: one for each duelist, so the AI can play both sides
// in the headless runs (the game's AI only ever played one); TcgAi_DoAction picks the turn holder's.
#include "tcg.h"

// wOpponentDeckID: the game's deck IDs (constants/deck_constants.asm, the *_DECK_IDs)
#define TCG_DECKID_SAMS_NORMAL              0x01
#define TCG_DECKID_LEGENDARY_MOLTRES        0x0C
#define TCG_DECKID_LEGENDARY_ZAPDOS         0x0D
#define TCG_DECKID_LEGENDARY_ARTICUNO       0x0E
#define TCG_DECKID_LEGENDARY_DRAGONITE      0x0F
#define TCG_DECKID_FIRST_STRIKE             0x10
#define TCG_DECKID_ROCK_CRUSHER             0x11
#define TCG_DECKID_GO_GO_RAIN_DANCE         0x12
#define TCG_DECKID_ZAPPING_SELFDESTRUCT     0x13
#define TCG_DECKID_FLOWER_POWER             0x14
#define TCG_DECKID_STRANGE_PSYSHOCK         0x15
#define TCG_DECKID_WONDERS_OF_SCIENCE       0x16
#define TCG_DECKID_FIRE_CHARGE              0x17
#define TCG_DECKID_MUSCLES_FOR_BRAINS       0x1C

struct TcgAi
{
    u8 deckId;                          // wOpponentDeckID
    // wAIDuelVars
    u8 peekedPrizes;                    // wAIPeekedPrizes
    u8 pokedexCounter;                  // wAIPokedexCounter
    u8 barrierFlagCounter;              // wAIBarrierFlagCounter
    const u8 *cardListAvoidPrize;       // wAICardListAvoidPrize (TCG_CARD_*, 0-terminated)
    const u8 *cardListArenaPriority;    // wAICardListArenaPriority
    const u8 *cardListBenchPriority;    // wAICardListBenchPriority
    const u8 *cardListPlayFromHandPriority;  // wAICardListPlayFromHandPriority
    const u8 *cardListRetreatBonus;     // wAICardListRetreatBonus (card, $80 + score)
    const u8 *cardListEnergyBonus;      // wAICardListEnergyBonus (card, max energy, $80 + score)
    u8 retreatScore;                    // wAIRetreatScore
    // the rest
    u8 tempLoadedAttackEnergyCost, tempLoadedAttackEnergyNeededType, tempLoadedAttackEnergyNeededAmount;
    u8 tempCardRetreatCost, tempCardId, tempCardType;   // wTempCardRetreatCost, wTempCardID, wTempCardType
    u8 score;                           // wAIScore
    union
    {
        u8 playAreaAIScore[TCG_MAX_PLAY_AREA_POKEMON];  // wPlayAreaAIScore
        u8 firstAttackAIScore;          // wFirstAttackAIScore
    };
    u8 playerColor, playerWeakness, playerResistance;  // wAIPlayer*
    u8 playerPrizeCount, opponentPrizeCount;
    u8 tempCardIdToLook;                // wTempCardIDToLook
    u8 playAreaCardToSwitch;            // wAIPlayAreaCardToSwitch
    u8 plusPowerAttack;                 // wAIPlusPowerAttack
    u8 playEnergyCardForRetreat;        // wAIPlayEnergyCardForRetreat
    u8 energyAttachLogicFlags;          // wAIEnergyAttachLogicFlags (AI_ENERGY_FLAG_*)
    u8 executeProcessedAttack;          // wAIExecuteProcessedAttack
    u8 retreatFlags;                    // wAIRetreatFlags
    u8 triedAttack;                     // wAITriedAttack
    u8 tempPlayAreaAIScore[TCG_MAX_PLAY_AREA_POKEMON];  // wTempPlayAreaAIScore
    u8 tempAIScore;                     // wTempAIScore
    u8 playAreaEnergyAIScore[TCG_MAX_PLAY_AREA_POKEMON];    // wPlayAreaEnergyAIScore
    u8 samePokemonEnergyScore[TCG_MAX_PLAY_AREA_POKEMON];   // wSamePokemonEnergyScore
    u8 cannotDamage;                    // wAICannotDamage
    u8 tempAI;                          // wTempAI
    u8 curCardCanAttack;                // wCurCardCanAttack
    u8 tempAIPokemonCard;               // wTempAIPokemonCard
    u8 curCardCanKO;                    // wCurCardCanKO
    u8 samePokemonCardId;               // wSamePokemonCardID
    u8 samePokemonEnergyScoreHandled[TCG_MAX_PLAY_AREA_POKEMON];
    u8 firstAttackDamage, secondAttackDamage;  // wAIFirstAttackDamage, wAISecondAttackDamage
    u8 attackIsNonDamaging;             // wAIAttackIsNonDamaging
    u8 retreatedThisTurn;               // wAIRetreatedThisTurn
    u8 venusaurDeckIndex, venusaurPlayAreaLocation;    // wAIVenusaurLv67*
    union
    {
        u8 wce06;
        u8 numberOfEnergyTransCards;    // wAINumberOfEnergyTransCards
        u8 defendingPokemonWeakness;    // wAIDefendingPokemonWeakness
        u8 setupBasicPokemonCount;      // wAISetupBasicPokemonCount
    };
    u8 wce07;
    union
    {
        u8 wce08[7];
        u8 setupEnergyCount;            // wAISetupEnergyCount
    };
    u8 wce0f[7];
    u8 trainerCardToPlay;               // wAITrainerCardToPlay
    u8 trainerLogicCard;                // wAITrainerLogicCard
    u8 trainerCardPhase;                // wAITrainerCardPhase
    u8 trainerCardParameter;            // wAITrainerCardParameter
    u8 wce1a, wce1b, wce1c, wce1d, wce1e, wce1f;
    u8 previousAIFlags, currentAIFlags; // wPreviousAIFlags, wCurrentAIFlags
};

extern struct TcgAi *gTcgAi;            // (the AI that's acting: TcgAi_DoAction sets it)
void TcgAi_SetDeck(u32 duelist, u32 deckId);
struct TcgAi *TcgAi_ForDuelist(u32 duelist);

// AIDoAction: an AIACTION_* for the turn holder's AI
u32 TcgAi_DoAction(u32 action);

// src/tcg/ai_core.c (engine/duel/ai/core.asm, damage_calculation.asm, init.asm)
bool32 TcgAi_CheckIfAnyAttackKnocksOutDefendingCard(void);
bool32 TcgAi_CheckIfAnyDefendingPokemonAttackDealsSameDamageAsHP(void);
u32 TcgAi_FindHighestBenchScore(void);
void TcgAi_Encourage(u32 a);
void TcgAi_Discourage(u32 a);
void TcgAi_LoadDefendingPokemonColorWRAndPrizeCards(void);
bool32 TcgAi_TryUseAttack(void);
bool32 TcgAi_CheckIfEnergyIsUseful(u32 deckIndex);
u32 TcgAi_PickRandomBenchPokemon(void);
void TcgAi_PlayInitialBasicCards(void);
bool32 TcgAi_CheckIfSelectedAttackIsUnusable(void);
struct TcgAiEnergyNeeded { u8 basic, colorless, energyCard; };
bool32 TcgAi_CheckEnergyNeededForAttack(struct TcgAiEnergyNeeded *out);
bool32 TcgAi_CheckEnergyNeededForAttackAfterDiscard(struct TcgAiEnergyNeeded *out);
u32 TcgAi_ConvertColorToEnergyCardID(u32 color);
bool32 TcgAi_CheckIfCardCanBePlayed(u32 deckIndex);
bool32 TcgAi_CreateEnergyCardListFromHand(void);
u32 TcgAi_LookForCardIDInHand(u32 cardId);         // (a deck index; 0xFF: not found)
u32 TcgAi_LookForCardIDInHandList(u32 cardId);     // (LookForCardIDInHandList_Bank5: 0xFF not found)
u32 TcgAi_LookForCardIDInPlayArea(u32 cardId, u32 start);  // (_Bank5: a PLAY_AREA_*, 0xFF not found)
bool32 TcgAi_AttachEnergyInHandToCardInPlayArea(u32 energyId, u32 pokemonId);
bool32 TcgAi_AttachEnergyInHandToCardInBench(u32 energyId, u32 pokemonId);
u32 TcgAi_ConvertHPToDamageCounters(u32 hp);
u32 TcgAi_CalculateBDividedByA(u32 b, u32 a);
u32 TcgAi_CountNumberOfEnergyCardsAttached(u32 location);
u32 TcgAi_LookForCardIDInLocation(u32 location, u32 cardId);   // (a deck index; 0xFF: not found)
u32 TcgAi_CountOppEnergyCardsInHandAndAttached(void);
u32 TcgAi_RemoveCardIDInList(u8 *list, u32 cardId);   // (the deck index removed; 0xFF: none)
bool32 TcgAi_TrySetUpBossStartingPlayArea(void);
bool32 TcgAi_CheckDamageToMrMime(u32 location);
bool32 TcgAi_CheckIfActiveCardCanKnockOut(void);
bool32 TcgAi_CanArenaCardUseNonResidualAttack(void);
bool32 TcgAi_LookForEnergyNeededInHand(void);
bool32 TcgAi_LookForEnergyNeededForAttackInHand(void);
void TcgAi_SortTempHandByIDList(void);
bool32 TcgAi_CheckEnergyFlagsNeededInList(u32 flags);
u32 TcgAi_GetAttacksEnergyCostBits(u32 deckIndex);
u32 TcgAi_CheckForEvolutionInList(u32 deckIndex);   // (the evolution's deck index; 0xFF: none)
u32 TcgAi_CheckForEvolutionInDeck(u32 deckIndex);
bool32 TcgAi_CheckIfArenaCardIsFullyPowered(void);
u32 TcgAi_CountNumberOfSetUpBenchPokemon(void);
bool32 TcgAi_SelectSpecialAttackParameters(void);
bool32 TcgAi_CheckIfNoSurplusEnergyForAttack(u32 *surplus);
u32 TcgAi_CheckCardEvolutionInHandOrDeck(u32 deckIndex);
bool32 TcgAi_CheckIfCanDamageDefendingPokemon(u32 location);
bool32 TcgAi_CheckIfDefendingPokemonCanKnockOut(u32 *largestDamage);
bool32 TcgAi_CheckIfOpponentHasBossDeckID(void);
bool32 TcgAi_CheckIfNotABossDeckID(void);
bool32 TcgAi_ChooseRandomlyNotToDoAction(void);
bool32 TcgAi_CheckForBenchIDAtHalfHPAndCanUseSecondAttack(u32 cardId);
void TcgAi_RaiseAIScoreToAllMatchingIDsInBench(u32 cardId);
void TcgAi_HandleAIEnergyScoringForRepeatedBenchPokemon(void);
void TcgAi_EstimateDamage_VersusDefendingCard(u32 attack);
void TcgAi_EstimateDamage_FromDefendingPokemon(u32 attack);
void TcgAi_InitAIDuelVars(void);
void TcgAi_InitAITurnVars(void);

// src/tcg/ai_common.c (common.asm)
bool32 TcgAi_HandleAIAntiMewtwoDeckStrategy(void);
u32 TcgAi_PickEnergyCardToDiscard(u32 location);
u32 TcgAi_PickAttachedEnergyCardToRemove(u32 location);
bool32 TcgAi_PickTwoAttachedEnergyCards(void);
u32 TcgAi_CountOppEnergyCardsInHand(void);
u32 TcgAi_LookForCardIDInDeck_GivenCardIDInHandAndPlayArea(u32 deckCard, u32 handPlayCard);
u32 TcgAi_LookForCardIDInHandAndPlayArea(u32 handCard, u32 playCard);
u32 TcgAi_LookForCardIDInDeck_GivenCardIDInHand(u32 deckCard, u32 handCard);
u32 TcgAi_LookForCardIDToTradeWithDifferentHandCard(u32 deckCard, u32 handCard);
bool32 TcgAi_CheckIfHasCardIDInHand(u32 cardId);
u32 TcgAi_CountPokemonCardsInHandAndInPlayArea(void);
u32 TcgAi_FindDuplicatePokemonCards(void);
bool32 TcgAi_CheckIfAttackIsHighRecoil(void);

// src/tcg/ai_attacks.c (attacks.asm, special_attacks.asm)
bool32 TcgAi_ProcessButDontUseAttack(void);
bool32 TcgAi_ProcessAndTryToUseAttack(void);
bool32 TcgAi_ProcessAttacks(void);
u32 TcgAi_HandleSpecialAIAttacks(void);

// src/tcg/ai_energy.c (energy.asm)
bool32 TcgAi_ProcessButDontPlayEnergy(void);
bool32 TcgAi_ProcessButDontPlayEnergy_SkipEvolution(void);
bool32 TcgAi_ProcessButDontPlayEnergy_SkipEvolutionAndArena(void);
bool32 TcgAi_ProcessAndTryToPlayEnergy(void);
bool32 TcgAi_ProcessEnergyCards(void);
bool32 TcgAi_TryToPlayEnergyCard(void);

// src/tcg/ai_pokemon.c (hand_pokemon.asm, retreat.asm, pkmn_powers.asm, boss_deck_set_up.asm)
bool32 TcgAi_DecidePlayPokemonCard(void);
bool32 TcgAi_DecideWhetherToRetreat(void);
u32 TcgAi_DecideBenchPokemonToSwitchTo(void);    // (a PLAY_AREA_*; 0xFF: none)
bool32 TcgAi_TryToRetreat(void);
void TcgAi_ProcessRetreat(void);


// src/tcg/ai.c (deck_ai.asm, decks/, boss_deck_set_up.asm, pkmn_powers.asm)
bool32 TcgAi_HandleAIPkmnPowers(void);      // (not ported: FALSE)
void TcgAi_HandleAIEnergyTrans(u32 mode);   // (not ported)
void TcgAi_SetUpBossStartingHandAndDeck(void);

// src/tcg/ai_trainer_cards.c (trainer_cards.asm)
void TcgAi_ProcessHandTrainerCards(u32 phase);

#endif // GUARD_TCG_AI_H
