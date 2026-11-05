// Pokémon Canvas: the Trading Card Game AI's core, ported from the Game Boy Color game's
// engine/duel/ai/core.asm, damage_calculation.asm and init.asm (pret/poketcg), routine by routine
// (TcgAi_ prefixed). The game's carry is TRUE; where it returns a deck index or location "if found",
// these return it, or 0xFF for not found. Checks for cards not in our decks are left out (noted).
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"
#include "tcg_ui.h"

static EWRAM_DATA struct TcgAi sTcgAi[2] = {0};
EWRAM_DATA struct TcgAi *gTcgAi = NULL;     // (TcgAi_DoAction sets it)

void TcgAi_SetDeck(u32 duelist, u32 deckId)
{
    sTcgAi[duelist & 1].deckId = deckId;
    gTcgAi = &sTcgAi[duelist & 1];
}

struct TcgAi *TcgAi_ForDuelist(u32 duelist)
{
    return &sTcgAi[duelist & 1];
}

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

// the game's "half the max HP" (rrca: the low bit goes to the top)
static inline u8 HalfHP(u32 hp)
{
    return ((hp >> 1) | (hp << 7)) & 0xFF;
}

// ---- damage_calculation.asm -----------------------------------------------------------------

// the damage (wDamage's low byte, and wAIMinDamage, wAIMaxDamage) to the defender from the turn holder's
// Pokémon at hTempPlayAreaLocation_ff9d: weakness, resistance, PlusPower, Defender, substatuses, and
// poison's 10 or 20 to come. (As the game: only wDamage's low byte; over 255, 255.)
static void CalculateDamage_VersusDefendingPokemon_One(u8 *value)
{
    u16 de = *value;
    u32 loc = gTcg->tempPlayAreaLocation;
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc));
    gTcg->tempTurnDuelistCardId = gTcg->loadedCard2Id;
    Tcg_SwapTurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    gTcg->tempNonTurnDuelistCardId = gTcg->loadedCard2Id;
    Tcg_SwapTurn();
    if (Tcg_HandleNoDamageOrEffectSubstatus())
    {
        de = 0;
    }
    else
    {
        u32 wr;
        if (loc == TCG_PLAY_AREA_ARENA)
            de = Tcg_HandleDoubleDamageSubstatus(de);
        // (UNAFFECTED_BY_WEAKNESS_RESISTANCE: the high byte, never set here: de is the low byte)
        wr = Tcg_TranslateColorToWR(Tcg_GetPlayAreaCardColor(loc));
        Tcg_SwapTurn();
        if (Tcg_GetArenaCardWeakness() & wr)
            de <<= 1;
        if (Tcg_GetArenaCardResistance() & wr)
            de -= 30;
        Tcg_SwapTurn();
        de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA + loc, de);
        Tcg_SwapTurn();
        de = Tcg_ApplyAttachedDefender(TCG_CARD_LOCATION_ARENA, de);
        de = Tcg_HandleDamageReduction(de);
        if (de & 0x8000)
            de = 0;
        switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_DOUBLE_POISONED)
        {
        case 0:
            break;
        case TCG_POISONED:
            de += 10;
            break;
        default:
            de += 20;
            break;
        }
        Tcg_SwapTurn();
    }
    *value = (de >> 8) ? 0xFF : de;
}

static void CalculateDamage_VersusDefendingPokemon(void)
{
    u8 damage = gTcg->damage;
    CalculateDamage_VersusDefendingPokemon_One(&gTcg->aiMinDamage);
    CalculateDamage_VersusDefendingPokemon_One(&gTcg->aiMaxDamage);
    CalculateDamage_VersusDefendingPokemon_One(&damage);
    gTcg->damage = (gTcg->damage & 0xFF00) | damage;
}

// (a benched Pokémon's estimate: without the Active Pokémon's substatuses and changed resistance)
static void WithoutArenaSubstatuses(void (*calculate)(void))
{
    u8 *s1 = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1);
    u8 *s2 = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2);
    u8 *res = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_RESISTANCE);
    u8 a = *s1, b = *s2, c = *res;
    *s1 = *s2 = *res = 0;
    calculate();
    *s1 = a;
    *s2 = b;
    *res = c;
}

// the AI's estimate of the damage of the turn holder's Pokémon at hTempPlayAreaLocation_ff9d's
// attack `attack` to the defender (wDamage, wAIMinDamage, wAIMaxDamage)
void TcgAi_EstimateDamage_VersusDefendingCard(u32 attack)
{
    gTcg->selectedAttack = attack;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), attack);
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
    {
        gTcg->damage = 0;
        gTcg->aiMinDamage = gTcg->aiMaxDamage = 0;
        return;
    }
    gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI);
    if ((gTcg->aiMinDamage | gTcg->aiMaxDamage) == 0)
        gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
        CalculateDamage_VersusDefendingPokemon();
    else
        WithoutArenaSubstatuses(CalculateDamage_VersusDefendingPokemon);
}

// the other way: the defender's attack against the turn holder's Pokémon at hTempPlayAreaLocation_ff9d
// (poison's 20 or 40 to come)
static void CalculateDamage_FromDefendingPokemon_One(u8 *value)
{
    u16 de = *value;
    u32 loc = gTcg->tempPlayAreaLocation, wr, weakness, resistance;
    Tcg_SwapTurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    gTcg->tempTurnDuelistCardId = gTcg->loadedCard2Id;
    Tcg_SwapTurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc));
    gTcg->tempNonTurnDuelistCardId = gTcg->loadedCard2Id;
    Tcg_SwapTurn();
    de = Tcg_HandleDoubleDamageSubstatus(de);
    wr = Tcg_TranslateColorToWR(Tcg_GetArenaCardColor());
    Tcg_SwapTurn();
    weakness = loc == TCG_PLAY_AREA_ARENA ? *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_WEAKNESS) : 0;
    if (!weakness)
    {
        Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc));
        weakness = gTcg->loadedCard2->weakness;
    }
    if (weakness & wr)
        de <<= 1;
    resistance = loc == TCG_PLAY_AREA_ARENA ? *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_RESISTANCE) : 0;
    if (!resistance)
    {
        Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc));
        resistance = gTcg->loadedCard2->resistance;
    }
    if (resistance & wr)
        de -= 30;
    Tcg_SwapTurn();
    de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA, de);
    Tcg_SwapTurn();
    de = Tcg_ApplyAttachedDefender(TCG_CARD_LOCATION_ARENA + loc, de);
    if (loc == TCG_PLAY_AREA_ARENA)
        de = Tcg_HandleDamageReduction(de);
    if (de & 0x8000)
        de = 0;
    if (loc == TCG_PLAY_AREA_ARENA)
    {
        switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_DOUBLE_POISONED)
        {
        case 0:
            break;
        case TCG_POISONED:
            de += 20;
            break;
        default:
            de += 40;
            break;
        }
    }
    *value = (de >> 8) ? 0xFF : de;
}

static void CalculateDamage_FromDefendingPokemon(void)
{
    u8 damage = gTcg->damage;
    CalculateDamage_FromDefendingPokemon_One(&gTcg->aiMinDamage);
    CalculateDamage_FromDefendingPokemon_One(&gTcg->aiMaxDamage);
    CalculateDamage_FromDefendingPokemon_One(&damage);
    gTcg->damage = (gTcg->damage & 0xFF00) | damage;
}

void TcgAi_EstimateDamage_FromDefendingPokemon(u32 attack)
{
    u8 loc;
    Tcg_SwapTurn();
    gTcg->selectedAttack = attack;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), attack);
    Tcg_SwapTurn();
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
    {
        gTcg->damage = 0;
        gTcg->aiMinDamage = gTcg->aiMaxDamage = 0;
        return;
    }
    gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    Tcg_SwapTurn();
    loc = gTcg->tempPlayAreaLocation;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI);
    gTcg->tempPlayAreaLocation = loc;
    Tcg_SwapTurn();
    if ((gTcg->aiMinDamage | gTcg->aiMaxDamage) == 0)
        gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
        CalculateDamage_FromDefendingPokemon();
    else
        WithoutArenaSubstatuses(CalculateDamage_FromDefendingPokemon);
}

// ---- core.asm -------------------------------------------------------------------------------

// TRUE: one of the attacks of the turn holder's Pokémon at hTempPlayAreaLocation_ff9d knocks out the
// defender (wSelectedAttack: which)
bool32 TcgAi_CheckIfAnyAttackKnocksOutDefendingCard(void)
{
    u32 attack;
    for (attack = 0; attack < 2; attack++)
    {
        TcgAi_EstimateDamage_VersusDefendingCard(attack);
        if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) <= (u8)gTcg->damage)
            return TRUE;
    }
    return FALSE;
}

// TRUE: one of the defender's attacks brings the turn holder's Pokémon at hTempPlayAreaLocation_ff9d to
// exactly 0 HP (wSelectedAttack)
bool32 TcgAi_CheckIfAnyDefendingPokemonAttackDealsSameDamageAsHP(void)
{
    u32 attack;
    for (attack = 0; attack < 2; attack++)
    {
        TcgAi_EstimateDamage_FromDefendingPokemon(attack);
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->tempPlayAreaLocation) == (u8)gTcg->damage)
            return TRUE;
    }
    return FALSE;
}

// the benched Pokémon with the highest wPlayAreaAIScore (the last of those tied): hTempPlayAreaLocation_ff9d
u32 TcgAi_FindHighestBenchScore(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), c, best = 0, bestScore = 0;
    for (c = 1; c < n; c++)
    {
        if (gTcgAi->playAreaAIScore[c] >= bestScore)
        {
            bestScore = gTcgAi->playAreaAIScore[c];
            best = c;
        }
    }
    gTcg->tempPlayAreaLocation = best;
    return best;
}

void TcgAi_Encourage(u32 a)
{
    u32 s = gTcgAi->score + a;
    gTcgAi->score = s > 255 ? 255 : s;
}

void TcgAi_Discourage(u32 a)
{
    if (gTcgAi->score == 0)
        return;
    gTcgAi->score = gTcgAi->score > a ? gTcgAi->score - a : 0;
}

void TcgAi_LoadDefendingPokemonColorWRAndPrizeCards(void)
{
    Tcg_SwapTurn();
    gTcgAi->playerColor = Tcg_TranslateColorToWR(Tcg_GetArenaCardColor());
    gTcgAi->playerWeakness = Tcg_GetArenaCardWeakness();
    gTcgAi->playerResistance = Tcg_GetArenaCardResistance();
    gTcgAi->playerPrizeCount = Tcg_CountPrizes();
    Tcg_SwapTurn();
    gTcgAi->opponentPrizeCount = Tcg_CountPrizes();
}

// the AI's chosen attack (wSelectedAttack), with its selections. TRUE: the turn's over.
bool32 TcgAi_TryUseAttack(void)
{
    u32 arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    gTcg->temp = gTcg->selectedAttack;
    gTcg->tempCardIndexFf9f = arena;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(arena, gTcg->selectedAttack);
    if (Tcg_AIMakeDecision(TCG_OPPACTION_BEGIN_ATTACK))
        return TRUE;
    if (!TcgAi_SelectSpecialAttackParameters())
        Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI_SELECTION);
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), gTcg->selectedAttack);
    if (Tcg_AIMakeDecision(TCG_OPPACTION_USE_ATTACK))
        return TRUE;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI_SWITCH_DEFENDING_PKMN);
    return Tcg_AIMakeDecision(TCG_OPPACTION_ATTACK_ANIM_AND_DAMAGE);
}

// TRUE: the attached Energy `deckIndex` is useful to the Pokémon wTempCardID (type wTempCardType):
// Double Colorless, its own type, or one its attacks want (Exeggcute, Exeggutor: Psychic)
bool32 TcgAi_CheckIfEnergyIsUseful(u32 deckIndex)
{
    u32 energy = TurnCardId(deckIndex);
    if (energy == TCG_CARD_DOUBLE_COLORLESS_ENERGY)
        return TRUE;
    if (gTcgAi->tempCardType == TCG_TYPE_ENERGY_DOUBLE_COLORLESS)
        return TRUE;
    // (Psyduck, Golduck, Surfing Pikachu and Eevee aren't in our decks)
    if ((gTcgAi->tempCardId == TCG_CARD_EXEGGCUTE || gTcgAi->tempCardId == TCG_CARD_EXEGGUTOR)
     && energy == TCG_CARD_PSYCHIC_ENERGY)
        return TRUE;
    return Tcg_Card(energy)->type == gTcgAi->tempCardType;
}

u32 TcgAi_PickRandomBenchPokemon(void)
{
    return Tcg_Random(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) - 1) + 1;
}

// the AI plays every Basic in its hand at the start of the duel
void TcgAi_PlayInitialBasicCards(void)
{
    const u8 *c;
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        gTcg->tempCardIndex = *c;
        Tcg_LoadCard1(*c);
        if (gTcg->loadedCard1->type >= TCG_TYPE_ENERGY || gTcg->loadedCard1->stage != TCG_BASIC)
            continue;
        Tcg_PutHandPokemonCardInPlayArea(*c);
    }
}

// TRUE: the turn holder's Pokémon at hTempPlayAreaLocation_ff9d can't use wSelectedAttack (or the AI's
// told to ignore it)
bool32 TcgAi_CheckIfSelectedAttackIsUnusable(void)
{
    if (gTcg->tempPlayAreaLocation == TCG_PLAY_AREA_ARENA)
    {
        if (Tcg_HandleCantAttackSubstatus())
            return TRUE;
        if (Tcg_CheckIfActiveCardParalyzedOrAsleep())
            return TRUE;
        Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), gTcg->selectedAttack);
        if (Tcg_HandleAmnesiaSubstatus())
            return TRUE;
        if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1))
            return TRUE;
    }
    if (TcgAi_CheckEnergyNeededForAttack(NULL))
        return TRUE;
    return Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_IGNORE_THIS_ATTACK_F);
}

// the energy still needed for wLoadedAttack: the game checks each type in turn and keeps the last
// one short (no attack needs two colors); out: the basic and colorless energy needed, and the
// energy card for that color
static bool32 EnergyNeeded(struct TcgAiEnergyNeeded *out)
{
    const struct TcgAttack *a = &gTcg->loadedAttack;
    u32 i, basicAttached, colorless;
    s32 left;
    gTcgAi->tempLoadedAttackEnergyCost = 0;
    gTcgAi->tempLoadedAttackEnergyNeededAmount = 0;
    gTcgAi->tempLoadedAttackEnergyNeededType = 0;
    for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
    {
        // CheckIfEnoughParticularAttachedEnergy
        if (a->cost[i] == 0)
            continue;
        gTcgAi->tempLoadedAttackEnergyCost = a->cost[i];
        if (a->cost[i] > gTcg->attachedEnergies[i])
        {
            gTcgAi->tempLoadedAttackEnergyNeededAmount = a->cost[i] - gTcg->attachedEnergies[i];
            gTcgAi->tempLoadedAttackEnergyNeededType = i;
        }
    }
    colorless = a->cost[TCG_COLORLESS];
    basicAttached = (u8)(gTcgAi->tempLoadedAttackEnergyCost - gTcgAi->tempLoadedAttackEnergyNeededAmount);
    left = (s32)gTcg->totalAttachedEnergies - (s32)basicAttached - (s32)colorless;
    if (left >= 0)
    {
        if (gTcgAi->tempLoadedAttackEnergyNeededAmount == 0)
        {
            if (out)
            {
                out->basic = colorless;     // (the game's b and c here: not "needed")
                out->colorless = basicAttached;
                out->energyCard = 0;
            }
            return FALSE;
        }
        left = 0;
    }
    if (out)
    {
        out->colorless = (u8)-left;
        out->basic = gTcgAi->tempLoadedAttackEnergyNeededAmount;
        out->energyCard = TcgAi_ConvertColorToEnergyCardID(gTcgAi->tempLoadedAttackEnergyNeededType);
    }
    return TRUE;
}

// TRUE: the turn holder's Pokémon at hTempPlayAreaLocation_ff9d can't use wSelectedAttack for want of
// energy (or it's no attack)
bool32 TcgAi_CheckEnergyNeededForAttack(struct TcgAiEnergyNeeded *out)
{
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), gTcg->selectedAttack);
    if (gTcg->loadedAttack.name == NULL || gTcg->loadedAttack.category == TCG_POKEMON_POWER)
    {
        if (out)
            out->basic = out->colorless = out->energyCard = 0;
        return TRUE;
    }
    Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
    Tcg_HandleEnergyBurn();
    return EnergyNeeded(out);
}

// the same, once an Energy card's been discarded (the one AIPickEnergyCardToDiscard picks)
bool32 TcgAi_CheckEnergyNeededForAttackAfterDiscard(struct TcgAiEnergyNeeded *out)
{
    u32 card;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), gTcg->selectedAttack);
    if (gTcg->loadedAttack.name == NULL || gTcg->loadedAttack.category == TCG_POKEMON_POWER)
    {
        if (out)
            out->basic = out->colorless = out->energyCard = 0;
        return TRUE;
    }
    card = TcgAi_PickEnergyCardToDiscard(gTcg->tempPlayAreaLocation);
    Tcg_LoadCard1(card);
    if (gTcg->loadedCard1Id == TCG_CARD_DOUBLE_COLORLESS_ENERGY)
    {
        gTcg->attachedEnergies[TCG_COLORLESS] -= 2;
        gTcg->totalAttachedEnergies -= 2;
    }
    else
    {
        // (the game's: card ID - 1 as the type, so Grass Energy takes off a Fire, Fire a Grass,
        // Water a Lightning, Lightning a Water: its card IDs aren't in the types' order)
        gTcg->attachedEnergies[(gTcg->loadedCard1Id - TCG_CARD_GRASS_ENERGY) & 7]--;
        gTcg->totalAttachedEnergies--;
    }
    Tcg_HandleEnergyBurn();
    return EnergyNeeded(out);
}

u32 TcgAi_ConvertColorToEnergyCardID(u32 color)
{
    static const u8 sCardIds[] =
    {
        TCG_CARD_FIRE_ENERGY, TCG_CARD_GRASS_ENERGY, TCG_CARD_LIGHTNING_ENERGY, TCG_CARD_WATER_ENERGY,
        TCG_CARD_FIGHTING_ENERGY, TCG_CARD_PSYCHIC_ENERGY, TCG_CARD_DOUBLE_COLORLESS_ENERGY,
    };
    return sCardIds[color < ARRAY_COUNT(sCardIds) ? color : TCG_COLORLESS];
}

// TRUE: the card `deckIndex` can't be played now (an Energy: one's been played; a Basic: no room; an
// Evolution: nothing to evolve; a Trainer: it can't be used)
bool32 TcgAi_CheckIfCardCanBePlayed(u32 deckIndex)
{
    u32 type, e, n;
    gTcg->tempCardIndexFf9f = deckIndex;
    Tcg_LoadCard1(deckIndex);
    type = gTcg->loadedCard1->type;
    if (type == TCG_TYPE_TRAINER)
    {
        if (Tcg_CheckCantUseTrainerDueToEffect())
            return TRUE;
        Tcg_LoadNonPokemonCardEffectCommands();
        return Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1);
    }
    if (type >= TCG_TYPE_ENERGY)
        return gTcg->alreadyPlayedEnergy != 0;
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    if (gTcg->loadedCard1->stage == TCG_BASIC)
        return n >= TCG_MAX_PLAY_AREA_POKEMON;
    if (Tcg_IsPrehistoricPowerActive())
        return TRUE;
    for (e = 0; e < n; e++)
        if (!Tcg_CheckIfCanEvolveInto(e, deckIndex, NULL))
            return FALSE;
    return TRUE;
}

// wDuelTempList: the Energy cards in the turn holder's hand. TRUE: none.
bool32 TcgAi_CreateEnergyCardListFromHand(void)
{
    u32 i, n = 0, count = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    for (i = 0; i < count; i++)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
        if (Tcg_Card(TurnCardId(card))->type & TCG_TYPE_ENERGY)
            gTcg->duelTempList[n++] = card;
    }
    gTcg->duelTempList[n] = 0xFF;
    return n == 0;
}

u32 TcgAi_LookForCardIDInHand(u32 cardId)
{
    u32 i, count = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    for (i = 0; i < count; i++)
        if (TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_HAND + i)) == cardId)
            return *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
    return 0xFF;
}

// (this one makes wDuelTempList the hand)
u32 TcgAi_LookForCardIDInHandList(u32 cardId)
{
    const u8 *c;
    gTcgAi->tempCardIdToLook = cardId;
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        gTcg->tempCardIndex = *c;
        Tcg_LoadCard1(*c);
        if (gTcg->loadedCard1Id == cardId)
            return *c;
    }
    return 0xFF;
}

u32 TcgAi_LookForCardIDInPlayArea(u32 cardId, u32 b)
{
    gTcgAi->tempCardIdToLook = cardId;
    for (; b < TCG_MAX_PLAY_AREA_POKEMON; b++)
    {
        u8 card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + b);
        if (card == 0xFF)
            return 0xFF;
        Tcg_LoadCard1(card);
        if (gTcg->loadedCard1Id == cardId)
            return b;
    }
    return 0xFF;
}

// the Energy `energyId` from the hand onto the Pokémon `pokemonId` in play (or on the bench). TRUE: the
// turn's over.
static bool32 AttachEnergyInHandToCard(u32 energyId, u32 pokemonId, u32 start)
{
    u32 card = TcgAi_LookForCardIDInHandList(energyId);
    if (card == 0xFF)
        return FALSE;
    gTcg->tempPlayAreaLocationFfa1 = TcgAi_LookForCardIDInPlayArea(pokemonId, start);
    gTcg->temp = card;
    return Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_ENERGY);
}

bool32 TcgAi_AttachEnergyInHandToCardInPlayArea(u32 energyId, u32 pokemonId)
{
    return AttachEnergyInHandToCard(energyId, pokemonId, TCG_PLAY_AREA_ARENA);
}

bool32 TcgAi_AttachEnergyInHandToCardInBench(u32 energyId, u32 pokemonId)
{
    return AttachEnergyInHandToCard(energyId, pokemonId, TCG_PLAY_AREA_BENCH_1);
}

u32 TcgAi_ConvertHPToDamageCounters(u32 hp)
{
    return (hp & 0xFF) / 10;
}

u32 TcgAi_CalculateBDividedByA(u32 b, u32 a)
{
    return a ? (b & 0xFF) / (a & 0xFF) : 0;
}

// the Energy cards attached (a Double Colorless counts once)
u32 TcgAi_CountNumberOfEnergyCardsAttached(u32 location)
{
    u32 i, n = 0;
    Tcg_GetPlayAreaCardAttachedEnergies(location);
    if (gTcg->totalAttachedEnergies == 0)
        return 0;
    for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
        n += gTcg->attachedEnergies[i];
    return (u8)(n + (gTcg->attachedEnergies[TCG_COLORLESS] >> 1));
}

u32 TcgAi_LookForCardIDInLocation(u32 location, u32 cardId)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == location && TurnCardId(i) == cardId)
            return i;
    return 0xFF;
}

// the Energy cards in the turn holder's hand and attached to their Pokémon (wTempAI)
u32 TcgAi_CountOppEnergyCardsInHandAndAttached(void)
{
    u32 e, n;
    gTcgAi->tempAI = 0;
    if (!TcgAi_CreateEnergyCardListFromHand())
        gTcgAi->tempAI = Tcg_CountCardsInDuelTempList();
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (e = 0; e < n; e++)
        gTcgAi->tempAI += TcgAi_CountNumberOfEnergyCardsAttached(e);
    return gTcgAi->tempAI;
}

u32 TcgAi_RemoveCardIDInList(u8 *list, u32 cardId)
{
    u8 *c;
    for (c = list; *c != 0xFF; c++)
    {
        if (TurnCardId(*c) == cardId)
        {
            u8 card = *c;
            gTcg->tempCardIndex = card;
            do
                c[0] = c[1];
            while (*c++ != 0xFF);
            return card;
        }
    }
    return 0xFF;
}

// the first card of `list` (card IDs) in the hand list, played. FALSE: none of them is there.
static bool32 PlayPokemonCardInOrder(const u8 *list)
{
    for (; *list; list++)
    {
        u32 card = TcgAi_RemoveCardIDInList(gTcg->duelTempList, *list);
        if (card != 0xFF)
        {
            Tcg_PutHandPokemonCardInPlayArea(card);
            return TRUE;
        }
    }
    return FALSE;
}

// a boss deck's starting play area: the Active from its arena list, then Benched from its bench list
// (until the 4th place in play, as the game: "a maximum of 3" it says). TRUE: there was no Active to play.
bool32 TcgAi_TrySetUpBossStartingPlayArea(void)
{
    if (gTcgAi->cardListArenaPriority == NULL)
        return TRUE;
    Tcg_CreateHandCardList();
    if (!PlayPokemonCardInOrder(gTcgAi->cardListArenaPriority))
        return TRUE;
    while (gTcgAi->cardListBenchPriority && PlayPokemonCardInOrder(gTcgAi->cardListBenchPriority))
        if (gTcg->tempPlayAreaLocation >= 3)
            break;
    return FALSE;
}

// TRUE: the defender isn't Mr. Mime, or the Pokémon at `location` can damage it
bool32 TcgAi_CheckDamageToMrMime(u32 location)
{
    u32 defending;
    Tcg_SwapTurn();
    defending = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    if (defending != TCG_CARD_MR_MIME)
        return TRUE;
    return TcgAi_CheckIfCanDamageDefendingPokemon(location);
}

bool32 TcgAi_CheckIfActiveCardCanKnockOut(void)
{
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    if (!TcgAi_CheckIfAnyAttackKnocksOutDefendingCard())
        return FALSE;
    return !TcgAi_CheckIfSelectedAttackIsUnusable();
}

// TRUE: the Active Pokémon has an attack it can use that isn't RESIDUAL
bool32 TcgAi_CanArenaCardUseNonResidualAttack(void)
{
    u32 attack;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    for (attack = 0; attack < 2; attack++)
    {
        gTcg->selectedAttack = attack;
        if (!TcgAi_CheckIfSelectedAttackIsUnusable() && !(gTcg->loadedAttack.category & TCG_RESIDUAL))
            return TRUE;
    }
    return FALSE;
}

// the energy one attack still needs, looked for in the hand: one basic, that card; one colorless, any
// Energy (wDuelTempList); two colorless, a Double Colorless. LOOK_FOUND, LOOK_NOT_FOUND, or
// LOOK_NEITHER (it needs something else: not decided)
enum { LOOK_NEITHER, LOOK_FOUND, LOOK_NOT_FOUND };
static u32 LookForEnergyNeeded(void)
{
    struct TcgAiEnergyNeeded need;
    TcgAi_CheckEnergyNeededForAttack(&need);
    switch ((u8)(need.basic + need.colorless))
    {
    case 1:
        if (need.basic)
            return TcgAi_LookForCardIDInHandList(need.energyCard) != 0xFF ? LOOK_FOUND : LOOK_NOT_FOUND;
        return TcgAi_CreateEnergyCardListFromHand() ? LOOK_NOT_FOUND : LOOK_FOUND;
    case 2:
        if (need.colorless == 2)
            return TcgAi_LookForCardIDInHandList(TCG_CARD_DOUBLE_COLORLESS_ENERGY) != 0xFF ? LOOK_FOUND : LOOK_NOT_FOUND;
        break;
    }
    return LOOK_NEITHER;
}

// (both attacks: the first that needs one energy, or two colorless, decides)
bool32 TcgAi_LookForEnergyNeededInHand(void)
{
    u32 result;
    gTcg->selectedAttack = 0;
    result = LookForEnergyNeeded();
    if (result == LOOK_NEITHER)
    {
        gTcg->selectedAttack = 1;
        result = LookForEnergyNeeded();
    }
    return result == LOOK_FOUND;
}

bool32 TcgAi_LookForEnergyNeededForAttackInHand(void)
{
    return LookForEnergyNeeded() == LOOK_FOUND;
}

// the hand list (wDuelTempList) sorted so the cards of wAICardListPlayFromHandPriority come first, in
// its order
void TcgAi_SortTempHandByIDList(void)
{
    const u8 *list = gTcgAi->cardListPlayFromHandPriority;
    u32 c = 0;
    if (list == NULL)
        return;
    for (; *list; list++)
    {
        u8 *h;
        for (h = &gTcg->duelTempList[c]; *h != 0xFF; h++)
        {
            gTcg->tempCardIndex = *h;
            if (TurnCardId(*h) == *list)
            {
                u8 t = gTcg->duelTempList[c];
                gTcg->duelTempList[c] = *h;
                *h = t;
                c++;
            }
        }
    }
}

// TRUE: an Energy card in wDuelTempList of a type in `flags` (the FIRE_F.. masks; COLORLESS_F: Double
// Colorless)
bool32 TcgAi_CheckEnergyFlagsNeededInList(u32 flags)
{
    static const struct { u8 card, flag; } sFlags[] =
    {
        {TCG_CARD_FIRE_ENERGY, TCG_FIRE_F}, {TCG_CARD_GRASS_ENERGY, TCG_GRASS_F},
        {TCG_CARD_LIGHTNING_ENERGY, TCG_LIGHTNING_F}, {TCG_CARD_WATER_ENERGY, TCG_WATER_F},
        {TCG_CARD_FIGHTING_ENERGY, TCG_FIGHTING_F}, {TCG_CARD_PSYCHIC_ENERGY, TCG_PSYCHIC_F},
        {TCG_CARD_DOUBLE_COLORLESS_ENERGY, TCG_COLORLESS_F},
    };
    const u8 *c;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        u32 id = TurnCardId(*c), i;
        for (i = 0; i < ARRAY_COUNT(sFlags); i++)
            if (sFlags[i].card == id && (sFlags[i].flag & flags))
                return TRUE;
    }
    return FALSE;
}

// both attacks' energy costs as type flags (any Colorless: all of them)
u32 TcgAi_GetAttacksEnergyCostBits(u32 deckIndex)
{
    u32 bits = 0, a, i;
    Tcg_LoadCard2(deckIndex);
    for (a = 0; a < 2; a++)
    {
        const u8 *cost = gTcg->loadedCard2->attacks[a].cost;
        for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
            if (cost[i])
                bits |= 1 << i;
        if (cost[TCG_COLORLESS])
            bits = 0xFF;
    }
    return bits;
}

// (as the game: the card is put in the Active spot for a moment, to check what evolves it)
static u32 FindEvolution(u32 deckIndex, bool32 (*where)(u32 i), const u8 *list)
{
    u8 *arena = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), saved = *arena;
    u32 found = 0xFF, i;
    *arena = deckIndex;
    if (list)
    {
        for (; *list != 0xFF; list++)
            if (!Tcg_CheckIfCanEvolveInto(TCG_PLAY_AREA_ARENA, *list, NULL))
            {
                found = *list;
                break;
            }
    }
    else
    {
        for (i = 0; i < TCG_DECK_SIZE; i++)
            if (where(i) && !Tcg_CheckIfCanEvolveInto(TCG_PLAY_AREA_ARENA, i, NULL))
            {
                found = i;
                break;
            }
    }
    *arena = saved;
    return found;
}

u32 TcgAi_CheckForEvolutionInList(u32 deckIndex)
{
    return FindEvolution(deckIndex, NULL, gTcg->duelTempList);
}

static bool32 InDeck(u32 i)
{
    return *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == TCG_CARD_LOCATION_DECK;
}

static bool32 InDeckOrHand(u32 i)
{
    u8 loc = *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i);
    return loc == TCG_CARD_LOCATION_DECK || loc == TCG_CARD_LOCATION_HAND;
}

// (the game stops at deck index $3c, DUELVARS_PRIZE_CARDS: all 60 of them)
u32 TcgAi_CheckForEvolutionInDeck(u32 deckIndex)
{
    return FindEvolution(deckIndex, InDeck, NULL);
}

u32 TcgAi_CheckCardEvolutionInHandOrDeck(u32 deckIndex)
{
    return FindEvolution(deckIndex, InDeckOrHand, NULL);
}

// TRUE: the Active Pokémon has more than half its HP, can't evolve any further (from the hand or
// deck), and can use its second attack
bool32 TcgAi_CheckIfArenaCardIsFullyPowered(void)
{
    u32 arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    Tcg_LoadCard1(arena);
    if (HalfHP(gTcg->loadedCard1->hp) >= *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP))
        return FALSE;
    if ((gTcg->loadedCard1->aiInfo & TCG_HAS_EVOLUTION) && TcgAi_CheckCardEvolutionInHandOrDeck(arena) != 0xFF)
        return FALSE;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    gTcg->selectedAttack = 1;
    return !TcgAi_CheckIfSelectedAttackIsUnusable();
}

// the benched Pokémon that are "set up": more than half their HP, done evolving, and able to use their
// second attack (the game's: whether or not they have one)
u32 TcgAi_CountNumberOfSetUpBenchPokemon(void)
{
    u8 loc = gTcg->tempPlayAreaLocation, attack = gTcg->selectedAttack;
    u32 c, n = 0;
    const u8 *bench = Tcg_TurnVar(TCG_DUELVARS_BENCH);
    for (c = 1; bench[c - 1] != 0xFF && c < TCG_MAX_PLAY_AREA_POKEMON; c++)
    {
        u32 card = bench[c - 1];
        Tcg_LoadCard1(card);
        // (rrca: half the HP, with its low bit as the top bit)
        if (HalfHP(gTcg->loadedCard1->hp) >= *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + c))
            continue;
        if ((gTcg->loadedCard1->aiInfo & TCG_HAS_EVOLUTION) && TcgAi_CheckCardEvolutionInHandOrDeck(card) != 0xFF)
            continue;
        gTcg->tempPlayAreaLocation = c;
        gTcg->selectedAttack = 1;
        if (!TcgAi_CheckIfSelectedAttackIsUnusable())
            n++;
    }
    gTcg->selectedAttack = attack;
    gTcg->tempPlayAreaLocation = loc;
    return n;
}

// the AI's picks for certain attacks (Exeggutor's Teleport, Electrode's Energy Spike; Mew's Devolution
// Beam and Mewtwo's Energy Absorption aren't in our decks): hTemp_ffa0, hTempPlayAreaLocation_ffa1.
// TRUE: made.
bool32 TcgAi_SelectSpecialAttackParameters(void)
{
    u8 attack = gTcg->selectedAttack;
    bool32 made = FALSE;
    switch (TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD)))
    {
    case TCG_CARD_EXEGGUTOR:
        if (gTcg->selectedAttack == 0)
        {
            u32 loc = TcgAi_DecideBenchPokemonToSwitchTo();
            if (loc != 0xFF)
            {
                gTcg->temp = loc;
                made = TRUE;
            }
        }
        break;
    case TCG_CARD_ELECTRODE_LV35:
        if (gTcg->selectedAttack != 0)
        {
            gTcg->temp = TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, TCG_CARD_LIGHTNING_ENERGY);
            if (gTcg->temp != 0xFF && TcgAi_ProcessButDontPlayEnergy_SkipEvolution())
            {
                gTcg->tempPlayAreaLocationFfa1 = gTcg->tempPlayAreaLocation;
                made = TRUE;
            }
        }
        break;
    }
    gTcg->selectedAttack = attack;
    return made;
}

// TRUE: the Pokémon at hTempPlayAreaLocation_ff9d has no more energy than wSelectedAttack needs (or not
// enough; or it's no attack). *surplus: the energy over.
bool32 TcgAi_CheckIfNoSurplusEnergyForAttack(u32 *surplus)
{
    const struct TcgAttack *a;
    u32 i;
    s32 left;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation), gTcg->selectedAttack);
    a = &gTcg->loadedAttack;
    if (a->name == NULL || a->category == TCG_POKEMON_POWER)
        return TRUE;
    Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
    Tcg_HandleEnergyBurn();
    gTcgAi->tempLoadedAttackEnergyCost = 0;
    gTcgAi->tempLoadedAttackEnergyNeededAmount = 0;
    gTcgAi->tempLoadedAttackEnergyNeededType = 0;
    for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
    {
        // CalculateParticularAttachedEnergyNeeded
        if (a->cost[i] == 0)
            continue;
        gTcgAi->tempLoadedAttackEnergyCost = a->cost[i];
        if (a->cost[i] > gTcg->attachedEnergies[i])
            gTcgAi->tempLoadedAttackEnergyNeededAmount = (u8)(gTcg->attachedEnergies[i] - a->cost[i]);
    }
    left = (s32)gTcg->totalAttachedEnergies - gTcgAi->tempLoadedAttackEnergyCost - a->cost[TCG_COLORLESS];
    if (surplus)
        *surplus = (u8)left;
    if (left < 0)
        return TRUE;
    return left == 0;
}

// TRUE: one of the attacks of the turn holder's Pokémon at `location` does the defender some damage
bool32 TcgAi_CheckIfCanDamageDefendingPokemon(u32 location)
{
    u32 attack;
    gTcg->tempPlayAreaLocation = location;
    for (attack = 0; attack < 2; attack++)
    {
        gTcg->selectedAttack = attack;
        if (TcgAi_CheckIfSelectedAttackIsUnusable())
            continue;
        TcgAi_EstimateDamage_VersusDefendingCard(attack);
        if ((u8)gTcg->damage)
            return TRUE;
    }
    return FALSE;
}

// TRUE: an attack of the defender's that it can use brings the Pokémon at hTempPlayAreaLocation_ff9d
// to 0 HP or below
static bool32 DefendingAttackKnocksOut(u32 attack)
{
    u8 loc = gTcg->tempPlayAreaLocation;
    bool32 unusable;
    gTcg->selectedAttack = attack;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    Tcg_SwapTurn();
    unusable = TcgAi_CheckIfSelectedAttackIsUnusable();
    Tcg_SwapTurn();
    gTcg->tempPlayAreaLocation = loc;
    if (unusable)
        return FALSE;
    TcgAi_EstimateDamage_FromDefendingPokemon(gTcg->selectedAttack);
    return *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->tempPlayAreaLocation) <= (u8)gTcg->damage;
}

// TRUE: the defender can knock out the Pokémon at hTempPlayAreaLocation_ff9d (wAIFirstAttackDamage,
// wAISecondAttackDamage: each attack's damage, if it does); *largestDamage: the larger
bool32 TcgAi_CheckIfDefendingPokemonCanKnockOut(u32 *largestDamage)
{
    u32 a;
    gTcgAi->firstAttackDamage = gTcgAi->secondAttackDamage = 0;
    if (DefendingAttackKnocksOut(0))
        gTcgAi->firstAttackDamage = gTcg->damage;
    if (DefendingAttackKnocksOut(1))
        gTcgAi->secondAttackDamage = gTcg->damage;
    else if (gTcgAi->firstAttackDamage == 0)
        return FALSE;
    a = gTcgAi->secondAttackDamage >= gTcgAi->firstAttackDamage ? gTcgAi->secondAttackDamage : gTcgAi->firstAttackDamage;
    if (largestDamage)
        *largestDamage = a;
    return TRUE;
}

// TRUE: the AI's deck is a Grand Master's, a Club Master's or Ronald's
bool32 TcgAi_CheckIfOpponentHasBossDeckID(void)
{
    return gTcgAi->deckId >= TCG_DECKID_LEGENDARY_MOLTRES && gTcgAi->deckId < TCG_DECKID_MUSCLES_FOR_BRAINS;
}

// TRUE: not a boss duel (and the player hasn't the legendary cards: never, here)
bool32 TcgAi_CheckIfNotABossDeckID(void)
{
    return !TcgAi_CheckIfOpponentHasBossDeckID();
}

// TRUE (sometimes): don't play the Trainer card or Pokémon Power this time (25%; boss decks never)
bool32 TcgAi_ChooseRandomlyNotToDoAction(void)
{
    if (!TcgAi_CheckIfNotABossDeckID())
        return FALSE;
    if (gTcgAi->deckId == TCG_DECKID_MUSCLES_FOR_BRAINS)
        return Tcg_Random(4) < 2;
    return Tcg_Random(4) < 1;
}

// TRUE: a benched `cardId` has more than half its HP and can use its second attack
bool32 TcgAi_CheckForBenchIDAtHalfHPAndCanUseSecondAttack(u32 cardId)
{
    u8 loc = gTcg->tempPlayAreaLocation, attack = gTcg->selectedAttack;
    u32 c;
    bool32 found = FALSE;
    gTcgAi->samePokemonCardId = cardId;
    for (c = 1; c < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + c) != 0xFF; c++)
    {
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + c));
        if (HalfHP(gTcg->loadedCard1->hp) >= *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + c))
            continue;
        if (gTcg->loadedCard1Id != gTcgAi->samePokemonCardId)
            continue;
        gTcg->tempPlayAreaLocation = c;
        gTcg->selectedAttack = 1;
        if (TcgAi_CheckIfSelectedAttackIsUnusable())
            continue;
        found = TRUE;
        break;
    }
    gTcg->selectedAttack = attack;
    gTcg->tempPlayAreaLocation = loc;
    return found;
}

void TcgAi_RaiseAIScoreToAllMatchingIDsInBench(u32 cardId)
{
    u32 e;
    for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) != 0xFF; e++)
        if (TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e)) == cardId)
            gTcgAi->playAreaEnergyAIScore[e] += 5;
}

static void SameScore(u32 e)
{
    u32 counters = TcgAi_ConvertHPToDamageCounters(Tcg_GetCardDamageAndMaxHP(e, NULL));
    u32 energy = TcgAi_CountNumberOfEnergyCardsAttached(e);
    gTcgAi->samePokemonEnergyScore[e] = (u8)(energy * 2 + 0x80 - counters);
    gTcgAi->samePokemonEnergyScoreHandled[e] = 1;
}

// repeated Pokémon on the bench: the one with the least damage and most energy gets a point more for
// energy, the others a point less
void TcgAi_HandleAIEnergyScoringForRepeatedBenchPokemon(void)
{
    u32 e, f;
    memset(gTcgAi->samePokemonEnergyScoreHandled, 0, sizeof(gTcgAi->samePokemonEnergyScoreHandled));
    for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON; e++)
    {
        u32 n, b, best, bestScore;
        u8 card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e);
        memset(gTcgAi->samePokemonEnergyScore, 0, sizeof(gTcgAi->samePokemonEnergyScore));
        if (card == 0xFF)
            return;
        if (gTcgAi->samePokemonEnergyScoreHandled[e])
            continue;
        gTcgAi->samePokemonCardId = TurnCardId(card);
        SameScore(e);
        for (f = e + 1; f < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + f) != 0xFF; f++)
            if (TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + f)) == gTcgAi->samePokemonCardId)
                SameScore(f);
        for (n = 0, b = 0; b < TCG_MAX_PLAY_AREA_POKEMON; b++)
            if (gTcgAi->samePokemonEnergyScore[b])
                n++;
        if (n < 2)
            continue;
        // the highest (the first of those tied, from the bench's end)
        for (best = 0, bestScore = 0, b = TCG_PLAY_AREA_BENCH_1 + 4; b >= 1; b--)
        {
            if (gTcgAi->samePokemonEnergyScore[b] >= bestScore)
            {
                bestScore = gTcgAi->samePokemonEnergyScore[b];
                best = b;
            }
        }
        for (b = 0; b < TCG_MAX_PLAY_AREA_POKEMON; b++)
        {
            if (b == best)
                gTcgAi->playAreaEnergyAIScore[b]++;
            else if (gTcgAi->samePokemonEnergyScore[b])
                gTcgAi->playAreaEnergyAIScore[b]--;
        }
    }
}

// ---- init.asm -------------------------------------------------------------------------------

void TcgAi_InitAIDuelVars(void)
{
    gTcgAi->peekedPrizes = 0;
    gTcgAi->barrierFlagCounter = 0;
    gTcgAi->cardListAvoidPrize = gTcgAi->cardListArenaPriority = gTcgAi->cardListBenchPriority = NULL;
    gTcgAi->cardListPlayFromHandPriority = gTcgAi->cardListRetreatBonus = gTcgAi->cardListEnergyBonus = NULL;
    gTcgAi->retreatScore = 0;
    gTcgAi->pokedexCounter = 5;
    gTcgAi->peekedPrizes = 0xFF;
}

// (the game's Mewtwo Lv53 Barrier counter: no Mewtwo here, so it stays 0)
void TcgAi_InitAITurnVars(void)
{
    gTcgAi->pokedexCounter++;
    gTcgAi->previousAIFlags = 0;
    gTcgAi->triedAttack = 0;
    gTcgAi->retreatedThisTurn = 0;
    gTcgAi->barrierFlagCounter = 0;
}
