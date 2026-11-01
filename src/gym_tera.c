// Pokémon Canvas: Terastallization.
//
// ARCHER gives the player the TERA ORB after the Viridian finale (data/scripts/dad_story.inc). It
// never needs charging (B_TERA_ORB_ALWAYS_CHARGED); one Pokémon a battle, as usual.
//   - The player's Pokémon Terastallize into the gym's type in every one-type battle (the gym
//     leaders, the rival, the Elite Four and Champion, Oak, the gym's challengers: the battles
//     where the League's rule is checked).
//   - Everywhere else a Pokémon uses its own Tera type - its "free" Tera type - but only if
//     it's a type the player has mastered (or the gym's type); otherwise the gym's type again.
//     ARCHER sets a Pokémon's free Tera type to any mastered type. The SKILLS page shows it.
//   - Once the player has the Orb, the important opponents' aces Terastallize too: in a one-type
//     battle into a type that resists the player's gym type, in a free battle (the tournament,
//     Bill, the Gym Leader Convention) into the type their team is built around.
#include "global.h"
#include "gym_tera.h"
#include "berry_field.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "event_data.h"
#include "gym_challenge.h"
#include "gym_hack.h"
#include "gym_switch.h"
#include "item.h"
#include "move.h"
#include "party_menu.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/trainers.h"

EWRAM_DATA static bool8 sOneTypeBattle = FALSE;

static bool32 IsRealType(enum Type type)
{
    return type > TYPE_NONE && type < NUMBER_OF_MON_TYPES && type != TYPE_MYSTERY && type != TYPE_STELLAR;
}

bool32 GymTera_HasOrb(void)
{
    return CheckBagHasItem(ITEM_TERA_ORB, 1);
}

static bool32 IsLeagueTrainer(u32 trainerId)
{
#if IS_FRLG
    switch (trainerId)
    {
    case TRAINER_ELITE_FOUR_LORELEI:
    case TRAINER_ELITE_FOUR_BRUNO:
    case TRAINER_ELITE_FOUR_AGATHA:
    case TRAINER_ELITE_FOUR_LANCE:
    case TRAINER_ELITE_FOUR_LORELEI_2:
    case TRAINER_ELITE_FOUR_BRUNO_2:
    case TRAINER_ELITE_FOUR_AGATHA_2:
    case TRAINER_ELITE_FOUR_LANCE_2:
    case TRAINER_CHAMPION_FIRST_SQUIRTLE:
    case TRAINER_CHAMPION_FIRST_BULBASAUR:
    case TRAINER_CHAMPION_FIRST_CHARMANDER:
    case TRAINER_CHAMPION_REMATCH_SQUIRTLE:
    case TRAINER_CHAMPION_REMATCH_BULBASAUR:
    case TRAINER_CHAMPION_REMATCH_CHARMANDER:
        return TRUE;
    }
#endif
    return FALSE;
}

// The battles that hold the player to the gym's type
bool32 GymTera_IsOneTypeTrainer(u32 trainerId)
{
#if IS_FRLG
    if (trainerId == TRAINER_GYM_CHALLENGER)
        return GymChallenge_IsOneTypeBattle();
#endif
    return GymHack_IsRuleBattleTrainer(trainerId) || IsLeagueTrainer(trainerId);
}

static enum GymTeraMode OpponentMode(u32 trainerId)
{
#if IS_FRLG
    if (trainerId == TRAINER_GYM_CHALLENGER)
        return GymChallenge_TeraMode();
#endif
    if (GymHack_IsRuleBattleTrainer(trainerId) || IsLeagueTrainer(trainerId))
        return GYM_TERA_COUNTER;
    return GYM_TERA_NONE;
}

static bool32 HasDamagingMoveOfType(struct Pokemon *mon, enum Type type)
{
    u32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move != MOVE_NONE && GetMovePower(move) != 0 && GetMoveType(move) == type)
            return TRUE;
    }
    return FALSE;
}

// A type that resists (or is immune to) the player's gym type: one of the ace's own if it can,
// keeping its STAB; else one it has moves of; better still if it hits the gym's type hard
static enum Type CounterType(struct Pokemon *mon, enum Type gym)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Type own1 = GetSpeciesType(species, 0), own2 = GetSpeciesType(species, 1), type, best = own1;
    s32 bestScore = -1;

    if (!IsRealType(gym))
        return own1;
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        uq4_12_t taken;
        s32 score = 0;
        if (!IsRealType(type))
            continue;
        taken = GetTypeModifier(gym, type);
        if (taken >= UQ_4_12(1.0))
            continue;
        if (type == own1 || type == own2)
            score += 8;
        if (HasDamagingMoveOfType(mon, type))
            score += 4;
        if (GetTypeModifier(type, gym) > UQ_4_12(1.0))
            score += 2;
        if (taken == UQ_4_12(0.0))
            score += 1;
        if (score > bestScore)
        {
            bestScore = score;
            best = type;
        }
    }
    return best;
}

// The type the team is built around: the most common primary type (ties: the ace's)
static enum Type TeamType(struct Pokemon *party, u32 ace)
{
    u8 counts[NUMBER_OF_MON_TYPES] = {0};
    enum Type best = GetSpeciesType(GetMonData(&party[ace], MON_DATA_SPECIES), 0), type;
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);
        if (species != SPECIES_NONE)
            counts[GetSpeciesType(species, 0)]++;
    }
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (IsRealType(type) && counts[type] > counts[best])
            best = type;
    }
    return best;
}

// After a trainer's party is made (src/battle_setup.c): whether the player is held to the gym's
// type, and the opponent's ace's Tera type (everyone else's stays TYPE_MYSTERY: they don't)
void GymTera_SetUpTrainerParty(struct Pokemon *party, u16 trainerId)
{
    enum GymTeraMode mode;
    enum Type type;
    u32 i, ace = PARTY_SIZE, aceLevel = 0;

    sOneTypeBattle = GymTera_IsOneTypeTrainer(TRAINER_BATTLE_PARAM.opponentA)
                  || ((gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && GymTera_IsOneTypeTrainer(TRAINER_BATTLE_PARAM.opponentB));
    mode = OpponentMode(trainerId);
    if (mode == GYM_TERA_NONE || !GymTera_HasOrb())
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (GetMonData(&party[i], MON_DATA_LEVEL) >= aceLevel)
        {
            aceLevel = GetMonData(&party[i], MON_DATA_LEVEL);
            ace = i;
        }
    }
    if (ace == PARTY_SIZE)
        return;
    if (mode == GYM_TERA_COUNTER)
        type = CounterType(&party[ace], VarGet(VAR_PLAYER_TYPE));
    else
        type = TeamType(party, ace);
    SetMonData(&party[ace], MON_DATA_TERA_TYPE, &type);
}

// A Pokémon's free Tera type: its own, if that's the gym's type or a mastered one; else the gym's
enum Type GymTera_FreeType(struct Pokemon *mon)
{
    enum Type gym = VarGet(VAR_PLAYER_TYPE), type = GetMonData(mon, MON_DATA_TERA_TYPE);

    if (!IsRealType(gym))
        return type;
    if (IsRealType(type) && (type == gym || FlagGet(FLAG_GYM_TYPE_MASTERED + type)))
        return type;
    return gym;
}

// What one of the player's Pokémon Terastallizes into in this battle (src/battle_terastal.c)
enum Type GymTera_PlayerBattleType(struct Pokemon *mon)
{
    enum Type gym = VarGet(VAR_PLAYER_TYPE);

    if (sOneTypeBattle && (gBattleTypeFlags & BATTLE_TYPE_TRAINER) && IsRealType(gym))
        return gym;
    return GymTera_FreeType(mon);
}

// ---------------------------------------------------------------------------
// ARCHER's Tera type changes (data/scripts/dad_story.inc)
// ---------------------------------------------------------------------------

// VAR_RESULT = how many types the player has mastered
void GymTera_CountMasteredTypes(void)
{
    enum Type type;
    u32 n = 0;
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (IsRealType(type) && FlagGet(FLAG_GYM_TYPE_MASTERED + type))
            n++;
    }
    gSpecialVar_Result = n;
}

// VAR_0x8004 = a party slot: STR_VAR_1 = its nickname, STR_VAR_2 = its free Tera type
void GymTera_BufferMonTera(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    GetMonNickname(mon, gStringVar1);
    GymSwitch_BufferTypeName(gStringVar2, GymTera_FreeType(mon));
}

// VAR_0x8005 = a type: VAR_RESULT = whether it can be a free Tera type (the gym's or a mastered one)
void GymTera_IsTypeAllowed(void)
{
    enum Type gym = VarGet(VAR_PLAYER_TYPE), type = gSpecialVar_0x8005;
    gSpecialVar_Result = IsRealType(type) && (type == gym || FlagGet(FLAG_GYM_TYPE_MASTERED + type));
}

// VAR_0x8004 = the party slot, VAR_0x8005 = the type: VAR_RESULT = 0 if it's that type already,
// 1 if the player has TERA_BERRY_PRICE of the type's berry, 2 if not; STR_VAR_3 = the berries
// ("3 OCCA BERRIES"), STR_VAR_2 = the type
void GymTera_CheckPrice(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    enum Type type = gSpecialVar_0x8005;
    enum Item berry = GetTeraBerry(type);
    u8 *end;

    GymSwitch_BufferTypeName(gStringVar2, type);
    end = ConvertIntToDecimalStringN(gStringVar3, TERA_BERRY_PRICE, STR_CONV_MODE_LEFT_ALIGN, 1);
    *end++ = CHAR_SPACE;
    CopyItemNameHandlePlural(berry, end, TERA_BERRY_PRICE);
    if (GymTera_FreeType(mon) == type)
        gSpecialVar_Result = 0;
    else if (berry != ITEM_NONE && CheckBagHasItem(berry, TERA_BERRY_PRICE))
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2;
}

// VAR_0x8004 = the party slot, VAR_0x8005 = the type: takes the berries and sets its Tera type
// (STR_VAR_1, STR_VAR_2 as above)
void GymTera_SetType(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    enum Type type = gSpecialVar_0x8005;
    enum Item berry = GetTeraBerry(type);

    if (IsRealType(type) && berry != ITEM_NONE && RemoveBagItem(berry, TERA_BERRY_PRICE))
        SetMonData(mon, MON_DATA_TERA_TYPE, &type);
    GymTera_BufferMonTera();
}
