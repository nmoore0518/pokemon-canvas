// Pokémon Canvas: the Trading Card Game AI's energy attaching, ported from the Game Boy Color game's
// engine/duel/ai/energy.asm (pret/poketcg) (TcgAi_ prefixed): each Pokémon in play scored (from $80)
// for an Energy card, and one attached to the best, if it scores $85 or more.
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

static void BackUpScores(void)
{
    memcpy(gTcgAi->tempPlayAreaAIScore, gTcgAi->playAreaAIScore, TCG_MAX_PLAY_AREA_POKEMON);
    gTcgAi->tempAIScore = gTcgAi->score;
}

static void RetrievePlayAreaAIScoreFromBackup1(void)
{
    memcpy(gTcgAi->playAreaAIScore, gTcgAi->tempPlayAreaAIScore, TCG_MAX_PLAY_AREA_POKEMON);
    gTcgAi->score = gTcgAi->tempAIScore;
}

static void AddScore(u32 a)
{
    if (a >= 0x80)
        TcgAi_Encourage(a - 0x80);
    else
        TcgAi_Discourage(0x80 - a);
}

// the hand's Energy, the turn holder's Pokémon in play scored for it, but nothing played (the best:
// hTempPlayAreaLocation_ff9d). TRUE: there's one worth it.
bool32 TcgAi_ProcessButDontPlayEnergy_SkipEvolution(void)
{
    gTcgAi->energyAttachLogicFlags = TCG_AI_ENERGY_FLAG_DONT_PLAY | TCG_AI_ENERGY_FLAG_SKIP_EVOLUTION;
    BackUpScores();
    return TcgAi_ProcessEnergyCards();
}

bool32 TcgAi_ProcessButDontPlayEnergy_SkipEvolutionAndArena(void)
{
    gTcgAi->energyAttachLogicFlags = TCG_AI_ENERGY_FLAG_DONT_PLAY | TCG_AI_ENERGY_FLAG_SKIP_EVOLUTION | TCG_AI_ENERGY_FLAG_SKIP_ARENA_CARD;
    BackUpScores();
    return TcgAi_ProcessEnergyCards();
}

// Func_16488: the same, counting evolutions
bool32 TcgAi_ProcessButDontPlayEnergy(void)
{
    gTcgAi->energyAttachLogicFlags = TCG_AI_ENERGY_FLAG_DONT_PLAY;
    BackUpScores();
    if (TcgAi_CreateEnergyCardListFromHand())
    {
        RetrievePlayAreaAIScoreFromBackup1();
        return FALSE;
    }
    return TcgAi_ProcessEnergyCards();
}

// an Energy card from the hand to the best Pokémon for it. TRUE: one was played.
bool32 TcgAi_ProcessAndTryToPlayEnergy(void)
{
    gTcgAi->energyAttachLogicFlags = 0;
    if (TcgAi_CreateEnergyCardListFromHand())
        return FALSE;
    return TcgAi_ProcessEnergyCards();
}

// (the energy the attack needs, scored; and the evolution in the hand's, if it has one)
static void DetermineAIScoreOfAttackEnergyRequirement(u32 attack)
{
    struct TcgAiEnergyNeeded need;
    u32 surplus;
    gTcg->selectedAttack = attack;
    if (TcgAi_CheckEnergyNeededForAttack(&need))
    {
        // .not_enough_energy
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_IGNORE_THIS_ATTACK_F))
            TcgAi_Discourage(5);
        if (need.basic && TcgAi_LookForCardIDInHand(need.energyCard) != 0xFF)
            TcgAi_Encourage(4);
        else if (need.colorless)
            TcgAi_Encourage(3);
        else
            goto check_evolution;
        // .check_total_needed: one more will do it
        if ((u8)(need.basic + need.colorless) == 1)
        {
            TcgAi_Encourage(3);
            if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
            {
                TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
                if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) <= (u8)gTcg->damage)
                {
                    TcgAi_Encourage(20);
                    if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
                        TcgAi_Encourage(10);
                }
            }
        }
        goto check_evolution;
    }
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ATTACHED_ENERGY_BOOST_F))
    {
        if (gTcg->loadedAttack.extra != TCG_MAX_ENERGY_BOOST_IS_LIMITED)
        {
            TcgAi_Encourage(gTcg->loadedAttack.extra);
            goto check_evolution;
        }
        if (!TcgAi_CheckIfNoSurplusEnergyForAttack(&surplus) && surplus >= 3)
        {
            TcgAi_Discourage(5);
            goto check_evolution;
        }
        goto encourage_more;
    }
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_DISCARD_ENERGY_F))
    {
        // (Zapdos Lv64's isn't in our decks)
        if (TcgAi_CheckIfNoSurplusEnergyForAttack(&surplus))
            goto encourage_more;
        TcgAi_Discourage(5);
    }
    goto check_evolution;

encourage_more:
    TcgAi_Encourage(2);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ATTACHED_ENERGY_BOOST_F))
    {
        u32 hp;
        TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
        hp = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP);
        if (hp > (u8)gTcg->damage && hp <= (u8)(gTcg->damage + 10))
        {
            // (another energy, 10 more damage: a knock-out)
            TcgAi_Encourage(20);
            if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
                TcgAi_Encourage(10);
        }
    }

check_evolution:
    if (gTcgAi->tempAI != 0xFF)
    {
        u8 *slot = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), saved = *slot;
        *slot = gTcgAi->tempAI;
        if (TcgAi_CheckEnergyNeededForAttack(&need) && !Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_IGNORE_THIS_ATTACK_F))
        {
            if (need.basic && TcgAi_LookForCardIDInHand(need.energyCard) != 0xFF)
                TcgAi_Encourage(2);
            else if (need.colorless)
                TcgAi_Encourage(1);
        }
        *slot = saved;
    }
}

// the Pokémon in play with the best wPlayAreaAIScore (the first of those tied; $85 or more, unless
// it's only the bench): hTempPlayAreaLocation_ff9d. TRUE: found.
static bool32 FindPlayAreaCardWithHighestAIScore(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), c, best, bestScore = 0;
    if (gTcgAi->energyAttachLogicFlags & TCG_AI_ENERGY_FLAG_SKIP_ARENA_CARD)
    {
        if (n <= 1)
            return FALSE;
        for (best = 1, c = 1; c < n; c++)
            if (gTcgAi->playAreaAIScore[c] > bestScore)
            {
                bestScore = gTcgAi->playAreaAIScore[c];
                best = c;
            }
        gTcg->tempPlayAreaLocation = best;
        return TRUE;
    }
    for (best = 0, c = 0; c < n; c++)
        if (gTcgAi->playAreaAIScore[c] > bestScore)
        {
            bestScore = gTcgAi->playAreaAIScore[c];
            best = c;
        }
    if (bestScore < 0x85)
        return FALSE;
    gTcg->tempPlayAreaLocation = best;
    return TRUE;
}

// each Pokémon in play scored for an Energy card (wPlayAreaAIScore), and the Energy played on the best
// (unless AI_ENERGY_FLAG_DONT_PLAY). TRUE: there's one worth it.
bool32 TcgAi_ProcessEnergyCards(void)
{
    u32 b, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    memset(gTcgAi->playAreaEnergyAIScore, 0x80, sizeof(gTcgAi->playAreaEnergyAIScore));
    // (HandleLegendaryArticunoEnergyScoring: not Rod's)
    for (b = 0; b < n; b++)
    {
        u32 card, hpCounters;
        gTcg->tempPlayAreaLocation = b;
        gTcgAi->score = 0x80;
        gTcgAi->tempAI = 0xFF;
        if (!(gTcgAi->energyAttachLogicFlags & TCG_AI_ENERGY_FLAG_SKIP_EVOLUTION))
        {
            Tcg_CreateHandCardList();
            gTcgAi->curCardCanAttack = card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + b);
            if (!TcgAi_CheckEnergyFlagsNeededInList(TcgAi_GetAttacksEnergyCostBits(card)))
                goto store_score;
            card = TcgAi_CheckForEvolutionInList(gTcgAi->curCardCanAttack);
            if (card != 0xFF)
            {
                gTcgAi->tempAI = card;
                TcgAi_Encourage(2);
            }
            else if (TcgAi_CheckForEvolutionInDeck(gTcgAi->curCardCanAttack) != 0xFF)
            {
                TcgAi_Encourage(1);
            }
        }
        // .check_venusaur (Muk's Toxic Gas: no Muk)
        if (Tcg_CountTurnDuelistPokemonWithActivePkmnPower(TCG_CARD_VENUSAUR_LV67))
            TcgAi_Encourage(1);
        if (b == TCG_PLAY_AREA_ARENA)
        {
            // (the Mewtwo mill flag: never set)
            TcgAi_Encourage(4);
            hpCounters = TcgAi_ConvertHPToDamageCounters(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP));
            if (hpCounters < 3
             && (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & (hpCounters == 2 ? TCG_DOUBLE_POISONED : TCG_POISONED)))
            {
                // (poison will knock it out)
                TcgAi_Discourage(10);
            }
            else if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
            {
                TcgAi_Discourage(10);
            }
            else
            {
                goto ai_score_bonus;
            }
            if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) == 1)
                TcgAi_Encourage(6);
        }
        else
        {
            hpCounters = TcgAi_ConvertHPToDamageCounters(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + b));
            if (hpCounters < 3)
                TcgAi_Discourage(3 - hpCounters);
        }
    ai_score_bonus:
        if (gTcgAi->cardListEnergyBonus)
        {
            const u8 *l;
            u32 id = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + b));
            for (l = gTcgAi->cardListEnergyBonus; *l; l += 3)
            {
                if (l[0] != id)
                    continue;
                Tcg_GetPlayAreaCardAttachedEnergies(b);
                if (gTcg->totalAttachedEnergies >= l[1])
                {
                    TcgAi_Discourage(10);
                    goto store_score;   // (the game's bug: it means .check_boss_deck)
                }
                AddScore(l[2]);
                break;
            }
        }
        // .check_boss_deck
        if (!TcgAi_CheckIfNotABossDeckID())
        {
            TcgAi_HandleAIEnergyScoringForRepeatedBenchPokemon();
            AddScore(gTcgAi->playAreaEnergyAIScore[b]);
        }
        TcgAi_Encourage(1);
        DetermineAIScoreOfAttackEnergyRequirement(0);
        DetermineAIScoreOfAttackEnergyRequirement(1);
    store_score:
        gTcgAi->playAreaAIScore[b] = gTcgAi->score;
    }
    if (!FindPlayAreaCardWithHighestAIScore())
    {
        if (gTcgAi->energyAttachLogicFlags)
            RetrievePlayAreaAIScoreFromBackup1();
        return FALSE;
    }
    if (gTcgAi->energyAttachLogicFlags)
    {
        RetrievePlayAreaAIScoreFromBackup1();
        return TRUE;
    }
    TcgAi_CreateEnergyCardListFromHand();
    return TcgAi_TryToPlayEnergyCard();
}

// TRUE: the Pokémon at hTempPlayAreaLocation_ff9d has an evolution (hand or deck) that would still need
// energy for wSelectedAttack (*need: what)
static bool32 CheckIfEvolutionNeedsEnergyForAttack(struct TcgAiEnergyNeeded *need)
{
    u8 *slot = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), saved;
    u32 evolution;
    bool32 needs;
    Tcg_CreateHandCardList();
    evolution = TcgAi_CheckCardEvolutionInHandOrDeck(*slot);
    if (evolution == 0xFF)
        return FALSE;
    saved = *slot;
    *slot = evolution;
    needs = TcgAi_CheckEnergyNeededForAttack(need);
    *slot = saved;
    return needs;
}

// the Energy an attack that discards energy, or grows with it, wants (FALSE: Zapdos Lv64's: none)
static bool32 GetEnergyCardForDiscardOrEnergyBoostAttack(struct TcgAiEnergyNeeded *need)
{
    const u8 *cost;
    static const u8 sEnergy[] = {TCG_CARD_FIRE_ENERGY, TCG_CARD_GRASS_ENERGY, TCG_CARD_LIGHTNING_ENERGY,
                                 TCG_CARD_WATER_ENERGY, TCG_CARD_FIGHTING_ENERGY};
    u32 i, id;
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    id = gTcg->loadedCard2Id;
    if (gTcg->selectedAttack && (id == TCG_CARD_CHARIZARD || id == TCG_CARD_EXEGGUTOR))
    {
        need->basic = FALSE;
        need->colorless = TRUE;
        return TRUE;
    }
    cost = gTcg->loadedCard2->attacks[gTcg->selectedAttack ? 1 : 0].cost;
    need->energyCard = TCG_CARD_PSYCHIC_ENERGY;
    for (i = 0; i < ARRAY_COUNT(sEnergy); i++)
        if (cost[i])
        {
            need->energyCard = sEnergy[i];
            break;
        }
    need->basic = TRUE;
    need->colorless = FALSE;
    return TRUE;
}

// Rod's Charmander and Dratini (and Fire Charge's Growlithe) get a Double Colorless if they have none.
// TRUE: that's the one (hTemp_ffa0).
static bool32 CheckSpecificDecksToAttachDoubleColorless(void)
{
    u32 id = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation)), card;
    switch (gTcgAi->deckId)
    {
    case TCG_DECKID_LEGENDARY_DRAGONITE:
        if (id != TCG_CARD_CHARMANDER && id != TCG_CARD_DRATINI)
            return FALSE;
        break;
    case TCG_DECKID_FIRE_CHARGE:
        if (id != TCG_CARD_GROWLITHE)
            return FALSE;
        break;
    default:
        return FALSE;
    }
    Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
    if (gTcg->attachedEnergies[TCG_COLORLESS])
        return FALSE;
    card = TcgAi_LookForCardIDInHand(TCG_CARD_DOUBLE_COLORLESS_ENERGY);
    if (card == 0xFF)
        return FALSE;
    gTcg->temp = card;
    return TRUE;
}

// an Energy card from the hand (wDuelTempList) onto the Pokémon at hTempPlayAreaLocation_ff9d: the one
// its attacks need. TRUE: played.
bool32 TcgAi_TryToPlayEnergyCard(void)
{
    struct TcgAiEnergyNeeded need;
    const u8 *c;
    gTcgAi->tempAI = 0;
    gTcg->selectedAttack = 0;
    if (TcgAi_CheckEnergyNeededForAttack(&need) && (need.basic || need.colorless))
        goto check_deck;
second_attack:
    gTcg->selectedAttack = 1;
    if (TcgAi_CheckEnergyNeededForAttack(&need) && (need.basic || need.colorless))
        goto check_deck;
    // .check_discard_or_energy_boost
    gTcgAi->tempAI = 1;
    gTcg->selectedAttack = 0;
    TcgAi_CheckEnergyNeededForAttack(&need);
    if (!Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ATTACHED_ENERGY_BOOST_F)
     && !Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_DISCARD_ENERGY_F))
    {
        gTcg->selectedAttack = 1;
        TcgAi_CheckEnergyNeededForAttack(&need);
        if (!Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ATTACHED_ENERGY_BOOST_F)
         && !Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_DISCARD_ENERGY_F))
        {
            if (!CheckIfEvolutionNeedsEnergyForAttack(&need))
                return FALSE;
            TcgAi_CreateEnergyCardListFromHand();
            goto check_deck;
        }
    }
    if (!GetEnergyCardForDiscardOrEnergyBoostAttack(&need))
        return FALSE;

check_deck:
    if (CheckSpecificDecksToAttachDoubleColorless())
        goto play_energy_card;
    if (need.basic)
    {
        u32 card = TcgAi_LookForCardIDInHand(need.energyCard);
        gTcg->temp = card;
        if (card != 0xFF)
            goto play_energy_card;
    }
    // .colorless_energy
    if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
    {
        if (need.colorless == 0)
            goto check_if_done;
        if (need.colorless == 2)
        {
            for (c = gTcg->duelTempList; *c != 0xFF; c++)
            {
                gTcg->temp = *c;
                if (TurnCardId(*c) == TCG_CARD_DOUBLE_COLORLESS_ENERGY)
                    goto play_energy_card;
            }
        }
    }
    // .look_for_any_energy
    Tcg_ShuffleCards(gTcg->duelTempList, Tcg_CountCardsInDuelTempList());
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        if (TcgAi_CheckIfOpponentHasBossDeckID() && TurnCardId(*c) == TCG_CARD_DOUBLE_COLORLESS_ENERGY)
            continue;
        gTcg->temp = *c;
        goto play_energy_card;
    }
check_if_done:
    if (gTcgAi->tempAI)
        return FALSE;
    if (gTcg->selectedAttack == 0)
        goto second_attack;
    return FALSE;

play_energy_card:
    gTcg->tempPlayAreaLocationFfa1 = gTcg->tempPlayAreaLocation;
    Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_ENERGY);
    return TRUE;
}
