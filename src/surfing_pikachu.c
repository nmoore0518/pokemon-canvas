// Pokémon Canvas: Surfing Pikachu, as in Yellow (where a Pikachu learned Surf in Pokémon Stadium's
// Master Ball Cup). The Stadium trainer in Route 21's hidden cave (data/maps/Route21_StadiumCave_Frlg)
// gives one to a trainer who beats him with a Pikachu in the party, and a shiny one to a trainer who
// beats him with nothing but Pikachu: one of each, ever. With one in the party, the beach house on
// Route 19 opens Pikachu's Beach (src/surfing_minigame.c).
#include "global.h"
#include "surfing_pikachu.h"
#include "event_data.h"
#include "malloc.h"
#include "move.h"
#include "pokemon.h"
#include "random.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/pokedex.h"
#include "constants/species.h"

bool32 SurfingPikachu_IsPikachu(u32 species)
{
    return species != SPECIES_NONE && species < NUM_SPECIES
        && gSpeciesInfo[species].natDexNum == NATIONAL_DEX_PIKACHU;
}

// What the party is: no Pikachu, some, or nothing but (eggs don't count either way)
u32 SurfingPikachu_PartyKind(void)
{
    u32 i, pikachu = 0, others = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        if (species == SPECIES_NONE || GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            continue;
        if (SurfingPikachu_IsPikachu(species))
            pikachu++;
        else
            others++;
    }
    if (pikachu == 0)
        return STADIUM_PARTY_NO_PIKACHU;
    return others ? STADIUM_PARTY_SOME_PIKACHU : STADIUM_PARTY_ALL_PIKACHU;
}

bool32 SurfingPikachu_InParty(void)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == SPECIES_PIKACHU_SURFING
         && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            return TRUE;
    }
    return FALSE;
}

// ---- specials ----

// After a win (VAR_RESULT = STADIUM_PARTY_*): what he owes now is marked (FLAG_STADIUM_OWES_*):
// nothing but Pikachu earns the shiny one, else any Pikachu the plain one (and with nothing but
// Pikachu and the shiny one already given, the plain one)
void SurfingPikachu_CheckParty(void)
{
    u32 kind = SurfingPikachu_PartyKind();
    gSpecialVar_Result = kind;
    if (kind == STADIUM_PARTY_ALL_PIKACHU && !FlagGet(FLAG_STADIUM_GAVE_SHINY))
        FlagSet(FLAG_STADIUM_OWES_SHINY);
    else if (kind != STADIUM_PARTY_NO_PIKACHU && !FlagGet(FLAG_STADIUM_GAVE_SURFING))
        FlagSet(FLAG_STADIUM_OWES_SURFING);
}

// VAR_RESULT: 0 nothing owed, 1 the Surfing Pikachu, 2 the shiny one (the shiny one first)
void SurfingPikachu_Owed(void)
{
    if (FlagGet(FLAG_STADIUM_OWES_SHINY))
        gSpecialVar_Result = 2;
    else if (FlagGet(FLAG_STADIUM_OWES_SURFING))
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 0;
}

// VAR_RESULT: whether he's given both (nothing left to earn)
void SurfingPikachu_AllGiven(void)
{
    gSpecialVar_Result = FlagGet(FLAG_STADIUM_GAVE_SURFING) && FlagGet(FLAG_STADIUM_GAVE_SHINY);
}

// Gives what's owed (the shiny one first): VAR_RESULT = MON_GIVEN_TO_PARTY / _PC / MON_CANT_GIVE
// (then it stays owed), VAR_0x8004 = whether it was the shiny one
void SurfingPikachu_Give(void)
{
    static const u16 sMoves[MAX_MON_MOVES] = {MOVE_SURF, MOVE_THUNDERBOLT, MOVE_QUICK_ATTACK, MOVE_AGILITY};
    struct Pokemon *mon;
    bool32 shiny = FlagGet(FLAG_STADIUM_OWES_SHINY);
    u32 i, value;

    if (!shiny && !FlagGet(FLAG_STADIUM_OWES_SURFING))
    {
        gSpecialVar_Result = MON_CANT_GIVE;
        return;
    }
    mon = AllocZeroed(sizeof(struct Pokemon));
    CreateMon(mon, SPECIES_PIKACHU_SURFING, SURFING_PIKACHU_LEVEL, Random32(), OTID_STRUCT_PLAYER_ID);
    SetMonData(mon, MON_DATA_IS_SHINY, &shiny);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        value = sMoves[i];
        SetMonData(mon, MON_DATA_MOVE1 + i, &value);
        value = GetMovePP(sMoves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &value);
    }
    CalculateMonStats(mon);
    gSpecialVar_Result = GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
    gSpecialVar_0x8004 = shiny;
    if (gSpecialVar_Result == MON_CANT_GIVE)
        return;
    if (shiny)
    {
        FlagClear(FLAG_STADIUM_OWES_SHINY);
        FlagSet(FLAG_STADIUM_GAVE_SHINY);
    }
    else
    {
        FlagClear(FLAG_STADIUM_OWES_SURFING);
        FlagSet(FLAG_STADIUM_GAVE_SURFING);
    }
}

// VAR_RESULT: a Surfing Pikachu in the party (Pikachu's Beach)
void SurfingPikachu_CheckInParty(void)
{
    gSpecialVar_Result = SurfingPikachu_InParty();
}
