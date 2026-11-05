// Pokémon Canvas: the Trading Card Game duel's AI opponent (Grandmaster Rod), to be ported from the
// game's engine/duel/ai/. So far: stand-ins for the choices the rules code needs.
#include "global.h"
#include "tcg.h"

// (AIDoAction_KOSwitch, until the AI's ported: the first benched Pokémon with HP)
u32 __attribute__((weak)) TcgAi_KOSwitch(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 1; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return i;
    return TCG_PLAY_AREA_BENCH_1;
}
