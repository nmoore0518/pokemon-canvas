// Tests for the Gym Trainer hack's custom logic (src/gym_hack.c).
// Run with: make check GAME_VERSION=FIRERED TESTS="GymHack"

#include "global.h"
#include "test/test.h"
#include "battle_main.h"
#include "event_data.h"
#include "gym_hack.h"
#include "pokemon.h"
#include "string_util.h"
#include "battle_util.h"
#include "data.h"
#include "constants/opponents.h"
#include "constants/rival_teams.h"
#include "constants/species.h"
#include "constants/vars.h"

// Menu labels from Oak's lab script, in menu order.
extern const u8 GymHack_Text_TypeFairy[], GymHack_Text_TypeBug[], GymHack_Text_TypeDark[],
    GymHack_Text_TypeDragon[], GymHack_Text_TypeElectric[], GymHack_Text_TypeFighting[],
    GymHack_Text_TypeFire[], GymHack_Text_TypeGhost[], GymHack_Text_TypeGrass[],
    GymHack_Text_TypeGround[], GymHack_Text_TypeIce[], GymHack_Text_TypeNormal[],
    GymHack_Text_TypePoison[], GymHack_Text_TypePsychic[], GymHack_Text_TypeRock[],
    GymHack_Text_TypeSteel[], GymHack_Text_TypeWater[];

static const u8 *const sMenuLabels[GYM_HACK_STARTER_COUNT] =
{
    GymHack_Text_TypeFairy, GymHack_Text_TypeBug, GymHack_Text_TypeDark,
    GymHack_Text_TypeDragon, GymHack_Text_TypeElectric, GymHack_Text_TypeFighting,
    GymHack_Text_TypeFire, GymHack_Text_TypeGhost, GymHack_Text_TypeGrass,
    GymHack_Text_TypeGround, GymHack_Text_TypeIce, GymHack_Text_TypeNormal,
    GymHack_Text_TypePoison, GymHack_Text_TypePsychic, GymHack_Text_TypeRock,
    GymHack_Text_TypeSteel, GymHack_Text_TypeWater,
};

static u32 ChooseStarter(u32 menuIndex)
{
    gSpecialVar_Result = menuIndex;
    GymHack_PrepareStarterChoice();
    return VarGet(VAR_TEMP_2);
}

static bool32 StringEndsWith(const u8 *str, const u8 *suffix)
{
    u32 n = StringLength(str), m = StringLength(suffix);
    return n >= m && StringCompare(str + n - m, suffix) == 0;
}

TEST("GymHack: every non-Flying type has exactly one starter")
{
    u32 i, j;
    u32 seen[NUMBER_OF_MON_TYPES] = {0};

    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        seen[gSpeciesInfo[ChooseStarter(i)].types[0]]++;

    for (j = 0; j < NUMBER_OF_MON_TYPES; j++)
    {
        if (j == TYPE_NONE || j == TYPE_FLYING || j == TYPE_MYSTERY || j == TYPE_STELLAR)
            EXPECT_EQ(seen[j], 0);
        else
            EXPECT_EQ(seen[j], 1);
    }
}

TEST("GymHack: menu labels name the species they give")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        EXPECT(StringEndsWith(sMenuLabels[i], GetSpeciesName(ChooseStarter(i))));
}

TEST("GymHack: menu labels name the starter's primary type")
{
    u32 i;
    u8 prefix[32];
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 species = ChooseStarter(i);
        u8 *end = StringCopy(prefix, gTypesInfo[gSpeciesInfo[species].types[0]].name);
        // Label format is "<Type> - <Species>"
        EXPECT_EQ(StringCompareN(sMenuLabels[i], prefix, end - prefix), 0);
    }
}

TEST("GymHack: confirming locks in the starter's primary type")
{
    VarSet(VAR_PLAYER_TYPE, TYPE_NONE);
    ChooseStarter(0); // Clefairy: Fairy in modern typing
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_FAIRY);

    ChooseStarter(13); // Ralts: Psychic/Fairy, primary Psychic
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_PSYCHIC);
}

TEST("GymHack: rival's type is never the player's and always beats it")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 playerType = gSpeciesInfo[ChooseStarter(i)].types[0];
        u32 rivalType = GymHack_GetRivalTypeFor(playerType);
        EXPECT_NE(rivalType, playerType);
        EXPECT_GT(GetTypeModifier(rivalType, playerType), UQ_4_12(1.0));
    }
}

TEST("GymHack: confirming sets the rival's type and starter")
{
    ChooseStarter(6); // Charmander (Fire)
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_RIVAL_TYPE), TYPE_WATER);
    EXPECT_EQ(VarGet(VAR_TEMP_3), SPECIES_SQUIRTLE);
}

static const u16 sRivalTrainerIds[][3] =
{
    { TRAINER_RIVAL_OAKS_LAB_SQUIRTLE, TRAINER_RIVAL_OAKS_LAB_BULBASAUR, TRAINER_RIVAL_OAKS_LAB_CHARMANDER },
    { TRAINER_RIVAL_ROUTE22_EARLY_SQUIRTLE, TRAINER_RIVAL_ROUTE22_EARLY_BULBASAUR, TRAINER_RIVAL_ROUTE22_EARLY_CHARMANDER },
    { TRAINER_RIVAL_CERULEAN_SQUIRTLE, TRAINER_RIVAL_CERULEAN_BULBASAUR, TRAINER_RIVAL_CERULEAN_CHARMANDER },
    { TRAINER_RIVAL_SS_ANNE_SQUIRTLE, TRAINER_RIVAL_SS_ANNE_BULBASAUR, TRAINER_RIVAL_SS_ANNE_CHARMANDER },
    { TRAINER_RIVAL_POKEMON_TOWER_SQUIRTLE, TRAINER_RIVAL_POKEMON_TOWER_BULBASAUR, TRAINER_RIVAL_POKEMON_TOWER_CHARMANDER },
    { TRAINER_RIVAL_SILPH_SQUIRTLE, TRAINER_RIVAL_SILPH_BULBASAUR, TRAINER_RIVAL_SILPH_CHARMANDER },
    { TRAINER_RIVAL_ROUTE22_LATE_SQUIRTLE, TRAINER_RIVAL_ROUTE22_LATE_BULBASAUR, TRAINER_RIVAL_ROUTE22_LATE_CHARMANDER },
    { TRAINER_CHAMPION_FIRST_SQUIRTLE, TRAINER_CHAMPION_FIRST_BULBASAUR, TRAINER_CHAMPION_FIRST_CHARMANDER },
    { TRAINER_CHAMPION_REMATCH_SQUIRTLE, TRAINER_CHAMPION_REMATCH_BULBASAUR, TRAINER_CHAMPION_REMATCH_CHARMANDER },
};
static const u8 sRivalPartySizes[] = { 1, 2, 4, 4, 5, 5, 6, 6, 6 };

TEST("GymHack: every rival battle uses his type-specialist team")
{
    u32 i, stage, v, m, n;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 rivalType;
        ChooseStarter(i);
        GymHack_LockInPlayerType();
        rivalType = VarGet(VAR_RIVAL_TYPE);
        for (stage = 0; stage < ARRAY_COUNT(sRivalTrainerIds); stage++)
        {
            for (v = 0; v < 3; v++)
            {
                const struct Trainer *t = GetTrainerStructFromId(sRivalTrainerIds[stage][v]);
                EXPECT(t == GymHack_GetRivalTrainer(sRivalTrainerIds[stage][v]));
                EXPECT(t != NULL);
                EXPECT_EQ((u32)t->partySize, (u32)sRivalPartySizes[stage]);
                for (m = 0; m < t->partySize; m++)
                {
                    u32 species = t->party[m].species;
                    // Two lines are borrowed from a secondary type (see gen_rival_teams.py)
                    bool32 borrowed = (rivalType == TYPE_STEEL && (species == SPECIES_MAGNEMITE || species == SPECIES_MAGNETON))
                                   || (rivalType == TYPE_ICE && species == SPECIES_LAPRAS);
                    if (!borrowed)
                        EXPECT_EQ(gSpeciesInfo[species].types[0], rivalType);
                    for (n = m + 1; n < t->partySize; n++)
                        EXPECT_NE(species, t->party[n].species);
                }
                // His ace (last slot) is the starter of his type
                if (stage == 0)
                    EXPECT_EQ(t->party[0].species, GymHack_GetStarterSpeciesForType(rivalType));
            }
        }
    }
}

TEST("GymHack: other trainers are untouched")
{
    EXPECT(GymHack_GetRivalTrainer(TRAINER_LEADER_BROCK) == NULL);
}
