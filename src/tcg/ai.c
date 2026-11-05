// Pokémon Canvas: the Trading Card Game duel's AI opponent (Grandmaster Rod), to be ported from the
// game's engine/duel/ai/. So far: a plain stand-in (weak: the real AI replaces it), enough for the
// duel to run start to finish: it puts its Basics down, evolves, attaches an Energy to its Active
// Pokémon, and uses the strongest attack it can. It plays no Trainers and never retreats.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"

static bool32 IsBasic(u32 deckIndex)
{
    const struct TcgCard *c = Tcg_Card(Tcg_GetCardIdFromDeckIndex(deckIndex));
    return c->type < TCG_TYPE_ENERGY && c->stage == TCG_BASIC;
}

// (AIDoAction_KOSwitch: the first benched Pokémon with HP)
u32 __attribute__((weak)) TcgAi_KOSwitch(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 1; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return i;
    return TCG_PLAY_AREA_BENCH_1;
}

// (AIDoAction_ForcedSwitch: the first benched Pokémon with HP)
u32 __attribute__((weak)) TcgAi_ForcedSwitch(void)
{
    return TcgAi_KOSwitch();
}

// (AIDoAction_StartDuel: the first Basic in the hand Active, the rest on the bench)
void __attribute__((weak)) TcgAi_StartDuel(void)
{
    u32 i;
    for (i = 0; i < *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
        if (IsBasic(card) && *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < TCG_MAX_PLAY_AREA_POKEMON)
            Tcg_PutHandPokemonCardInPlayArea(card);
        else
            i++;
    }
}

// (AIDoAction_Turn)
void __attribute__((weak)) TcgAi_DoTurn(void)
{
    u32 i, attack, best = 0xFF, bestDamage = 0;
    // Basics to the bench, evolutions
    for (i = 0; i < *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + i), n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), e;
        const struct TcgCard *c = Tcg_Card(Tcg_GetCardIdFromDeckIndex(card));
        if (c->type < TCG_TYPE_ENERGY && c->stage == TCG_BASIC && n < TCG_MAX_PLAY_AREA_POKEMON)
        {
            gTcg->temp = card;
            if (Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_BASIC_PKMN))
                return;
            continue;
        }
        if (c->type < TCG_TYPE_ENERGY && c->stage != TCG_BASIC)
        {
            for (e = 0; e < n; e++)
                if (!Tcg_CheckIfCanEvolveInto(e, card, NULL))
                    break;
            if (e < n)
            {
                gTcg->temp = card;
                gTcg->tempPlayAreaLocationFfa1 = e;
                if (Tcg_AIMakeDecision(TCG_OPPACTION_EVOLVE_PKMN))
                    return;
                continue;
            }
        }
        i++;
    }
    // an Energy to the Active Pokémon
    for (i = 0; i < *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND); i++)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
        if (Tcg_Card(Tcg_GetCardIdFromDeckIndex(card))->type & (1 << TCG_TYPE_ENERGY_F))
        {
            gTcg->temp = card;
            gTcg->tempPlayAreaLocationFfa1 = TCG_PLAY_AREA_ARENA;
            if (Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_ENERGY))
                return;
            break;
        }
    }
    // the strongest attack it can use
    if (!Tcg_HandleCantAttackSubstatus() && !Tcg_CheckIfActiveCardParalyzedOrAsleep())
    {
        u32 arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
        for (attack = 0; attack < 2; attack++)
        {
            Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
            Tcg_HandleEnergyBurn();
            if (Tcg_CheckIfEnoughEnergiesForGivenAttack(arena, attack))
                continue;
            gTcg->selectedAttack = attack;
            if (Tcg_HandleAmnesiaSubstatus())
                continue;
            if (best == 0xFF || gTcg->loadedCard1->attacks[attack].damage >= bestDamage)
            {
                best = attack;
                bestDamage = gTcg->loadedCard1->attacks[attack].damage;
            }
        }
        if (best != 0xFF)
        {
            gTcg->tempCardIndexFf9f = arena;
            gTcg->temp = best;
            if (Tcg_AIMakeDecision(TCG_OPPACTION_BEGIN_ATTACK))
                return;
            Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI_SELECTION);
            if (Tcg_AIMakeDecision(TCG_OPPACTION_USE_ATTACK))
                return;
            Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AI_SWITCH_DEFENDING_PKMN);
            Tcg_AIMakeDecision(TCG_OPPACTION_ATTACK_ANIM_AND_DAMAGE);
            return;
        }
    }
    Tcg_AIMakeDecision(TCG_OPPACTION_FINISH_NO_ATTACK);
}
