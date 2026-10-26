// Tests for the Gym Trainer hack's custom logic (src/gym_hack.c).
// Run with: make check GAME_VERSION=FIRERED TESTS="GymHack"

#include "global.h"
#include "test/test.h"
#include "battle_main.h"
#include "event_data.h"
#include "gym_hack.h"
#include "gauntlet.h"
#include "tournament.h"
#include "mastery_hall.h"
#include "archive.h"
#include "leader_academy.h"
#include "team_builder.h"
#include "dex_forms.h"
#include "event_mons.h"
#include "party_menu.h"
#include "item.h"
#define ACADEMY_LECTURE_COUNT_FOR_TEST 8
#include "gym_challenge.h"
#include "constants/event_objects.h"
#include "constants/pokedex.h"
#include "exotic_safari.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "pokemon.h"
#include "string_util.h"
#include "battle_util.h"
#include "field_player_avatar.h"
#include "item.h"
#include "item_use.h"
#include "constants/field_move.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "data.h"
#include "battle_setup.h"
#include "test/overworld_script.h"
#include "constants/gym_hack.h"
#include "constants/opponents.h"
#include "constants/rival_teams.h"
#include "constants/species.h"
#include "constants/vars.h"
#include "overworld.h"
#include "region_map.h"
#include "wild_encounter.h"

// Menu labels from Oak's lab script, in menu order.
extern const u8 GymHack_Text_TypeFairy[], GymHack_Text_TypeBug[], GymHack_Text_TypeDark[],
    GymHack_Text_TypeDragon[], GymHack_Text_TypeElectric[], GymHack_Text_TypeFighting[],
    GymHack_Text_TypeFire[], GymHack_Text_TypeFlying[], GymHack_Text_TypeGhost[], GymHack_Text_TypeGrass[],
    GymHack_Text_TypeGround[], GymHack_Text_TypeIce[], GymHack_Text_TypeNormal[],
    GymHack_Text_TypePoison[], GymHack_Text_TypePsychic[], GymHack_Text_TypeRock[],
    GymHack_Text_TypeSteel[], GymHack_Text_TypeWater[];

static const u8 *const sMenuLabels[GYM_HACK_STARTER_COUNT] =
{
    GymHack_Text_TypePsychic, GymHack_Text_TypeFire, GymHack_Text_TypeNormal, GymHack_Text_TypeWater, GymHack_Text_TypeGrass, GymHack_Text_TypeElectric, GymHack_Text_TypeRock, GymHack_Text_TypePoison, GymHack_Text_TypeBug, GymHack_Text_TypeSteel, GymHack_Text_TypeIce, GymHack_Text_TypeGround, GymHack_Text_TypeFlying, GymHack_Text_TypeDragon, GymHack_Text_TypeFighting, GymHack_Text_TypeDark, GymHack_Text_TypeGhost, GymHack_Text_TypeFairy,
};

static u32 GymHackTest_ChooseStarter(u32 menuIndex)
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

TEST("GymHack: every type has exactly one starter")
{
    u32 i, j;
    u32 seen[NUMBER_OF_MON_TYPES] = {0};

    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        seen[gSpeciesInfo[GymHackTest_ChooseStarter(i)].types[0]]++;

    for (j = 0; j < NUMBER_OF_MON_TYPES; j++)
    {
        if (j == TYPE_NONE || j == TYPE_MYSTERY || j == TYPE_STELLAR)
            EXPECT_EQ(seen[j], 0);
        else
            EXPECT_EQ(seen[j], 1);
    }
}

TEST("GymHack: menu labels name the species they give")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        EXPECT(StringEndsWith(sMenuLabels[i], GetSpeciesName(GymHackTest_ChooseStarter(i))));
}

TEST("GymHack: menu labels name the starter's primary type")
{
    u32 i;
    u8 prefix[32];
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 species = GymHackTest_ChooseStarter(i);
        u8 *end = StringCopy(prefix, gTypesInfo[gSpeciesInfo[species].types[0]].name);
        // Label format is "<Type> - <Species>"
        EXPECT_EQ(StringCompareN(sMenuLabels[i], prefix, end - prefix), 0);
    }
}

TEST("GymHack: confirming locks in the starter's primary type")
{
    VarSet(VAR_PLAYER_TYPE, TYPE_NONE);
    GymHackTest_ChooseStarter(17); // Clefairy: Fairy in modern typing
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_FAIRY);

    GymHackTest_ChooseStarter(0); // Ralts: Psychic/Fairy, primary Psychic
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_PSYCHIC);
}

TEST("GymHack: rival's type is never the player's and always beats it")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 playerType = gSpeciesInfo[GymHackTest_ChooseStarter(i)].types[0];
        u32 rivalType = GymHack_GetRivalTypeFor(playerType);
        EXPECT_NE(rivalType, playerType);
        EXPECT_GT(GetTypeModifier(rivalType, playerType), UQ_4_12(1.0));
    }
}

TEST("GymHack: confirming sets the rival's type and starter")
{
    GymHackTest_ChooseStarter(1); // Charmander (Fire)
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
        GymHackTest_ChooseStarter(i);
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

// ---- Team rules ----

static void SetPlayerType(u32 type)
{
    VarSet(VAR_PLAYER_TYPE, type);
}

TEST("GymHack: a full team of the player's type is legal")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 10;
        givemon SPECIES_VULPIX, 10;
        givemon SPECIES_GROWLITHE, 10;
        givemon SPECIES_PONYTA, 10;
        givemon SPECIES_MAGBY, 10;
        givemon SPECIES_NUMEL, 10;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);
}

TEST("GymHack: a Pokemon of another primary type breaks the rules")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 10;
        givemon SPECIES_SQUIRTLE, 10;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_WRONG_TYPE);
    EXPECT_EQ(StringCompare(gStringVar1, GetSpeciesName(SPECIES_SQUIRTLE)), 0);
    EXPECT_EQ(StringCompare(gStringVar2, gTypesInfo[TYPE_WATER].name), 0);
    EXPECT_EQ(StringCompare(gStringVar3, gTypesInfo[TYPE_FIRE].name), 0);
}

TEST("GymHack: only the primary type counts")
{
    // Gyarados is Water/Flying: fine for Water, not for Flying-adjacent teams
    SetPlayerType(TYPE_WATER);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_GYARADOS, 30;);
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);

    // Swablu (Normal/Flying) isn't a Dragon yet
    SetPlayerType(TYPE_DRAGON);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_DRATINI, 10; givemon SPECIES_SWABLU, 10;);
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_WRONG_TYPE);
}

TEST("GymHack: two of the same species break the rules")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 10;
        givemon SPECIES_VULPIX, 10;
        givemon SPECIES_CHARMANDER, 12;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_DUPLICATE);
    EXPECT_EQ(StringCompare(gStringVar1, GetSpeciesName(SPECIES_CHARMANDER)), 0);
}

TEST("GymHack: different stages of one family are allowed")
{
    SetPlayerType(TYPE_GHOST);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_GASTLY, 10;
        givemon SPECIES_HAUNTER, 30;
        givemon SPECIES_GENGAR, 40;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);
}

TEST("GymHack: eggs are ignored")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 10;
        giveegg SPECIES_SQUIRTLE;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);
}

TEST("GymHack: no rules before a type is chosen")
{
    SetPlayerType(TYPE_NONE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_SQUIRTLE, 5; givemon SPECIES_SQUIRTLE, 5;);
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);
}

TEST("GymHack: rules apply to gym leaders and the rival, not other trainers")
{
    EXPECT(GymHack_IsRuleBattleTrainer(TRAINER_LEADER_BROCK));
    EXPECT(GymHack_IsRuleBattleTrainer(TRAINER_LEADER_GIOVANNI));
    EXPECT(GymHack_IsRuleBattleTrainer(TRAINER_RIVAL_CERULEAN_SQUIRTLE));
    EXPECT(GymHack_IsRuleBattleTrainer(TRAINER_RIVAL_ROUTE22_LATE_CHARMANDER));
    // The League is checked at its door instead
    EXPECT(!GymHack_IsRuleBattleTrainer(TRAINER_ELITE_FOUR_LORELEI));
    EXPECT(!GymHack_IsRuleBattleTrainer(TRAINER_CHAMPION_FIRST_SQUIRTLE));
    EXPECT(!GymHack_IsRuleBattleTrainer(TRAINER_NONE));
}

TEST("GymHack: a gym leader battle with an illegal team is replaced")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_SQUIRTLE, 10;);
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_LEADER_BROCK;
    TRAINER_BATTLE_PARAM.opponentB = TRAINER_NONE;
    EXPECT(GymHack_GetBlockedBattleScript() != NULL);
    EXPECT_EQ(gSpecialVar_Result, GYM_HACK_TEAM_WRONG_TYPE);

    // A legal team battles normally, and so does an ordinary trainer
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_CHARMANDER, 10;);
    EXPECT(GymHack_GetBlockedBattleScript() == NULL);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_SQUIRTLE, 10;);
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_ELITE_FOUR_LORELEI;
    EXPECT(GymHack_GetBlockedBattleScript() == NULL);
}

// ---- Evolution toggle ----

TEST("GymHack: a Pokemon with evolution turned off doesn't evolve by level")
{
    bool8 locked = TRUE;
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_CHARMANDER, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_CHARMELEON);
    SetMonData(&mon, MON_DATA_EVOLUTION_LOCKED, &locked);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_TRADE, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    locked = FALSE;
    SetMonData(&mon, MON_DATA_EVOLUTION_LOCKED, &locked);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_CHARMELEON);
}

TEST("GymHack: evolution stones still work with evolution turned off")
{
    bool8 locked = TRUE;
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_GROWLITHE, 20, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&mon, MON_DATA_EVOLUTION_LOCKED, &locked);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_FIRE_STONE, NULL, NULL, CHECK_EVO), SPECIES_ARCANINE);
}

TEST("GymHack: the evolution setting is kept in the PC")
{
    bool8 locked = TRUE;
    struct Pokemon mon;
    struct BoxPokemon *box;
    CreateMon(&mon, SPECIES_GASTLY, 10, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&mon, MON_DATA_EVOLUTION_LOCKED, &locked);
    box = &mon.box;
    EXPECT_EQ(GetBoxMonData(box, MON_DATA_EVOLUTION_LOCKED), TRUE);
}

TEST("GymHack: only Pokemon that can evolve get the evolution option")
{
    EXPECT(GymHack_SpeciesCanEvolve(SPECIES_GASTLY));
    EXPECT(!GymHack_SpeciesCanEvolve(SPECIES_TAUROS));
}

// ---- HM key items ----

TEST("GymHack: each HM key item matches its field move")
{
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_CUT), ITEM_HEDGE_CLIPPERS);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_FLASH), ITEM_FLASHLIGHT);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_SURF), ITEM_BOOGIE_BOARD);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_FLY), ITEM_HOT_AIR_BALLOON);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_STRENGTH), ITEM_CAN_OF_SPINACH);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_ROCK_SMASH), ITEM_SLEDGEHAMMER);
    EXPECT_EQ(GymHack_GetFieldMoveItem(FIELD_MOVE_WATERFALL), ITEM_ROCKET_BOOTS);
    EXPECT_EQ(GetItemPocket(ITEM_HEDGE_CLIPPERS), POCKET_KEY_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_CAN_OF_SPINACH), POCKET_KEY_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_ROCKET_BOOTS), POCKET_KEY_ITEMS);
}

TEST("GymHack: a tree can be cut with the Hedge Clippers instead of a Pokemon")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_CHARMANDER, 10;);
    RUN_OVERWORLD_SCRIPT(checkfieldmove FIELD_MOVE_CUT;);
    EXPECT_EQ(gSpecialVar_Result, PARTY_SIZE);

    RUN_OVERWORLD_SCRIPT(additem ITEM_HEDGE_CLIPPERS; checkfieldmove FIELD_MOVE_CUT;);
    EXPECT_EQ(gSpecialVar_Result, FIELD_MOVE_USER_ITEM);

    // A Pokemon that knows Cut is still used first
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_BULBASAUR, 10, move1=MOVE_CUT; checkfieldmove FIELD_MOVE_CUT;);
    EXPECT_EQ(gSpecialVar_Result, 1);
}

TEST("GymHack: the Boogie Board lets the player surf")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_CHARMANDER, 10;);
    EXPECT(!GymHack_HasFieldMoveItem(FIELD_MOVE_SURF) || PartyHasMonWithSurf());
    RUN_OVERWORLD_SCRIPT(additem ITEM_BOOGIE_BOARD;);
    EXPECT(PartyHasMonWithSurf());
}

// ---- Evolutions without trading ----

static u32 EvolveByLevel(u32 species, u32 level)
{
    struct Pokemon mon;
    CreateMon(&mon, species, level, 0, OTID_STRUCT_PLAYER_ID);
    return GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO);
}

static u32 EvolveWithItem(u32 species, u32 item)
{
    struct Pokemon mon;
    CreateMon(&mon, species, 30, 0, OTID_STRUCT_PLAYER_ID);
    return GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, item, NULL, NULL, CHECK_EVO);
}

TEST("GymHack: trade evolutions happen at level 36 instead")
{
    EXPECT_EQ(EvolveByLevel(SPECIES_KADABRA, 35), SPECIES_NONE);
    EXPECT_EQ(EvolveByLevel(SPECIES_KADABRA, 36), SPECIES_ALAKAZAM);
    EXPECT_EQ(EvolveByLevel(SPECIES_MACHOKE, 36), SPECIES_MACHAMP);
    EXPECT_EQ(EvolveByLevel(SPECIES_GRAVELER, 36), SPECIES_GOLEM);
    EXPECT_EQ(EvolveByLevel(SPECIES_HAUNTER, 36), SPECIES_GENGAR);
}

TEST("GymHack: trade items work like evolution stones")
{
    EXPECT_EQ(EvolveWithItem(SPECIES_POLIWHIRL, ITEM_KINGS_ROCK), SPECIES_POLITOED);
    EXPECT_EQ(EvolveWithItem(SPECIES_SLOWPOKE, ITEM_KINGS_ROCK), SPECIES_SLOWKING);
    EXPECT_EQ(EvolveWithItem(SPECIES_ONIX, ITEM_METAL_COAT), SPECIES_STEELIX);
    EXPECT_EQ(EvolveWithItem(SPECIES_SCYTHER, ITEM_METAL_COAT), SPECIES_SCIZOR);
    EXPECT_EQ(EvolveWithItem(SPECIES_SEADRA, ITEM_DRAGON_SCALE), SPECIES_KINGDRA);
    EXPECT_EQ(EvolveWithItem(SPECIES_PORYGON, ITEM_UPGRADE), SPECIES_PORYGON2);
    EXPECT_EQ(EvolveWithItem(SPECIES_CLAMPERL, ITEM_DEEP_SEA_TOOTH), SPECIES_HUNTAIL);
    EXPECT_EQ(EvolveWithItem(SPECIES_CLAMPERL, ITEM_DEEP_SEA_SCALE), SPECIES_GOREBYSS);
    EXPECT_EQ(EvolveWithItem(SPECIES_FEEBAS, ITEM_PRISM_SCALE), SPECIES_MILOTIC);
    EXPECT(GetItemFieldFunc(ITEM_KINGS_ROCK) == ItemUseOutOfBattle_EvolutionStone);
}

TEST("GymHack: Sun Shard and Moon Shard evolve Eevee")
{
    EXPECT_EQ(EvolveWithItem(SPECIES_EEVEE, ITEM_SUN_SHARD), SPECIES_ESPEON);
    EXPECT_EQ(EvolveWithItem(SPECIES_EEVEE, ITEM_MOON_SHARD), SPECIES_UMBREON);
    EXPECT(GetItemFieldFunc(ITEM_SUN_SHARD) == ItemUseOutOfBattle_EvolutionStone);
}

extern const u16 CeladonCity_DepartmentStore_4F_Items[];

TEST("GymHack: every evolution item at Celadon can be bought")
{
    u32 i;
    for (i = 0; CeladonCity_DepartmentStore_4F_Items[i] != ITEM_NONE; i++)
        EXPECT_GT(GetItemPrice(CeladonCity_DepartmentStore_4F_Items[i]), 0);
    EXPECT_GE(i, 15);
}

static const u8 sFinderTypes[] = {
    TYPE_NORMAL, TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL,
    TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY,
};

static u32 CountFinderAreas(u32 type, u16 *out)
{
    u32 r, total = 0;
    VarSet(VAR_PLAYER_TYPE, type);
    for (r = REGION_MAP_KANTO; r <= REGION_MAP_SEVII67; r++)
        total += GymHack_GetTypeFinderMapSecs(out, 64, r);
    return total;
}

TEST("GymHack: type finder shows nothing before a type is chosen")
{
    u16 out[64];
    EXPECT_EQ(CountFinderAreas(TYPE_NONE, out), 0);
}

TEST("GymHack: type finder finds an area for every selectable type")
{
    u16 out[64];
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sFinderTypes); i++)
        EXPECT_GT(CountFinderAreas(sFinderTypes[i], out), 0);
}

TEST("GymHack: type finder marks exactly the areas with that primary type")
{
    u16 out[64];
    u32 i, j, k, r, count;
    for (i = 0; i < ARRAY_COUNT(sFinderTypes); i++)
    {
        VarSet(VAR_PLAYER_TYPE, sFinderTypes[i]);
        for (r = REGION_MAP_KANTO; r <= REGION_MAP_SEVII67; r++)
        {
            count = GymHack_GetTypeFinderMapSecs(out, ARRAY_COUNT(out), r);
            EXPECT_LT(count, ARRAY_COUNT(out));
            // Every marked area is on this map
            for (k = 0; k < count; k++)
                EXPECT_EQ(GetRegionMapType(out[k]), r);
            // Every wild table on this map with the type is marked
            for (j = 0; gWildMonHeaders[j].mapGroup != MAP_GROUP(MAP_UNDEFINED); j++)
            {
                u32 mapSec = Overworld_GetMapHeaderByGroupAndId(gWildMonHeaders[j].mapGroup, gWildMonHeaders[j].mapNum)->regionMapSectionId;
                bool32 marked = FALSE;
                if (mapSec >= MAPSEC_NONE || GetRegionMapType(mapSec) != r)
                    continue;
                for (k = 0; k < count; k++)
                {
                    if (out[k] == mapSec)
                        marked = TRUE;
                }
                if (GymHack_WildHeaderHasType(&gWildMonHeaders[j], sFinderTypes[i]))
                    EXPECT(marked);
            }
        }
    }
}

TEST("GymHack: type finder only counts primary types")
{
    // Route 1's tables hold Normal and Flying types (Pidgey and Spearow are Flying first here)
    u32 j;
    for (j = 0; gWildMonHeaders[j].mapGroup != MAP_GROUP(MAP_UNDEFINED); j++)
    {
        if (gWildMonHeaders[j].mapGroup == MAP_GROUP(MAP_ROUTE1) && gWildMonHeaders[j].mapNum == MAP_NUM(MAP_ROUTE1))
        {
            EXPECT(GymHack_WildHeaderHasType(&gWildMonHeaders[j], TYPE_NORMAL));
            EXPECT(GymHack_WildHeaderHasType(&gWildMonHeaders[j], TYPE_FLYING));
            EXPECT(!GymHack_WildHeaderHasType(&gWildMonHeaders[j], TYPE_FIRE));
        }
    }
}

// ---- Gym decorations ----
#include "gym_decor.h"
#include "gym_candidate.h"
#include "money.h"
#include "text.h"
#include "constants/gym_room.h"

TEST("GymHack: decor: buying takes the money and adds to storage")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 1500);
    gSpecialVar_0x8004 = GYM_DECOR_POTTED_PLANT;
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 1500 - gGymDecor[GYM_DECOR_POTTED_PLANT].price);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_POTTED_PLANT), 1);
    // Not enough left for a second one
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_POTTED_PLANT), 1);
}

TEST("GymHack: decor: leftover secret base bytes are cleared, not read as decorations")
{
    // A new game (or an old save) has secret base data in this space, e.g. 0xFF name bytes
    memset(gSaveBlock1Ptr->secretBases, 0xFF, sizeof(gSaveBlock1Ptr->secretBases));
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_CHAIR_LEFT), 0);
    EXPECT_EQ(GymDecor_NumPlaced(), 0);
    // Real decoration data is kept
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    gSpecialVar_0x8004 = GYM_DECOR_TV;
    GymDecor_TryBuy();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TV), 1);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TV), 1);
}

TEST("GymHack: decor: gym decorations unlock with badges")
{
    FlagClear(FLAG_BADGE02_GET);
    EXPECT(GymDecor_IsUnlocked(GYM_DECOR_POTTED_PLANT));
    EXPECT(!GymDecor_IsUnlocked(GYM_DECOR_POOL_WATER));
    FlagSet(FLAG_BADGE02_GET);
    EXPECT(GymDecor_IsUnlocked(GYM_DECOR_POOL_WATER));
    FlagClear(FLAG_BADGE02_GET);
}

TEST("GymHack: decor: placing uses up storage and blocks overlaps")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    gSpecialVar_0x8004 = GYM_DECOR_TABLE;
    GymDecor_TryBuy();
    GymDecor_TryBuy();
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 2);
    EXPECT(GymDecor_Place(GYM_DECOR_TABLE, 3, 5));
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 1);
    EXPECT_EQ(GymDecor_CountPlaced(GYM_DECOR_TABLE), 1);
    // The table is 2x2: these overlap it
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TABLE, 4, 6));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TABLE, 2, 4));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_TABLE, 5, 5));
    // Putting it away (from any of its blocks) returns it to storage
    EXPECT(GymDecor_RemoveAt(4, 6));
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 2);
    EXPECT_EQ(GymDecor_NumPlaced(), 0);
}

TEST("GymHack: decor: nothing goes on the walls, the entrance or the PC")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    gSpecialVar_0x8004 = GYM_DECOR_POTTED_PLANT;
    GymDecor_TryBuy();
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 5, 0));                         // back wall
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, GYM_PC_FRONT_X, GYM_PC_FRONT_Y - 1)); // entrance area
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, -1, 5));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, GYM_ROOM_WIDTH - 1, GYM_EDIT_Y1));  // 2 tall: sticks out
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, GYM_ROOM_WIDTH - 1, GYM_EDIT_Y1 - 1));
}

TEST("GymHack: decor: every decoration has a name, a price and fits the gym")
{
    u32 i;
    for (i = 0; i < GYM_DECOR_COUNT; i++)
    {
        EXPECT(gGymDecor[i].name != NULL);
        if (i != GYM_DECOR_BATTLE_STAGE && GymCandidate_GetRecruitByDecor(i) == NULL   // recruits join, they aren't sold
         && gGymDecor[i].category != GYM_DECOR_CAT_TROPHIES)                          // (trophies are won)
            EXPECT_GT(gGymDecor[i].price, 0);
        EXPECT_LE(gGymDecor[i].width, 4);
        EXPECT_LE(gGymDecor[i].height, 5);
        EXPECT_LE(GetStringWidth(FONT_NORMAL, gGymDecor[i].name, 0), 13 * 8); // the list sits at x 15
    }
}

// ---- The Gym Leader Convention ----
#include "gym_convention.h"
#include "gym_challenge.h"
#include "constants/gym_convention.h"

TEST("GymHack: convention: beating a whole hall wins its trophy, and every hall the grand one")
{
    u32 i, hall;
    memset(GymChallenge_GetSave()->conventionBeaten, 0xFF, 16);   // (a save from before the convention)
    GymChallenge_GetSave()->conventionMagic = 0;
    EXPECT(!Convention_IsBeaten(0));
    for (hall = 0; hall < CONVENTION_HALL_COUNT; hall++)
        FlagClear(FLAG_CONVENTION_TROPHIES + hall);
    FlagClear(FLAG_CONVENTION_TROPHY_GRAND);
    for (i = 0; i < CONVENTION_TRAINER_COUNT; i++)
    {
        gSpecialVar_0x8004 = i;
        Convention_RecordWin();
        EXPECT(Convention_IsBeaten(i));
        if (i == 0)                         // (no rematch while the rest of the hall is left)
        {
            Convention_SetupTrainer();
            EXPECT_EQ(gSpecialVar_Result, 1);
        }
        if (i == CONVENTION_TRAINER_COUNT - 1)
            EXPECT_EQ(gSpecialVar_Result, CONVENTION_WON_ALL);
    }
    for (hall = 0; hall < CONVENTION_HALL_COUNT; hall++)
        EXPECT(FlagGet(FLAG_CONVENTION_TROPHIES + hall));
    EXPECT(FlagGet(FLAG_CONVENTION_TROPHY_GRAND));
    EXPECT_GT(GymDecor_CountOwned(GYM_DECOR_TROPHY_GRAND), 0);
    gSpecialVar_0x8004 = 0;                 // (the hall is cleared: rematches)
    Convention_SetupTrainer();
    EXPECT_EQ(gSpecialVar_Result, 2);
    gSpecialVar_0x8004 = 0;                 // (beating someone again wins nothing more)
    Convention_RecordWin();
    EXPECT_EQ(gSpecialVar_Result, 0);
}

TEST("GymHack: convention: every trainer has a full team and lines")
{
    u32 i;
    for (i = 0; i < CONVENTION_TRAINER_COUNT; i++)
    {
        gSpecialVar_0x8004 = i;
        Convention_SetupTrainer();
        EXPECT_EQ((u32)GymChallenge_GetTrainer(TRAINER_GYM_CHALLENGER)->partySize, PARTY_SIZE);
        EXPECT(gGymChallengeIntroText[0] != EOS);
        EXPECT(gGymChallengeAfterText[0] != EOS);
    }
}

// ---- Stat training items ----
#include "stat_items.h"
#include "malloc.h"
#include "string_util.h"

#include "script_pokemon_util.h"

#define TEST_MON (&gParties[B_TRAINER_PLAYER][0])

static void GiveTestMon(enum Species species)
{
    ZeroPlayerPartyMons();
    ScriptGiveMon(species, 50, ITEM_NONE);
}

TEST("GymHack: items: Silver Bottle Cap maxes one IV, Gold maxes all")
{
    u32 iv = 0, i, hpBefore;
    GiveTestMon(SPECIES_GROWLITHE);
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(TEST_MON, MON_DATA_HP_IV + i, &iv);
    CalculateMonStats(TEST_MON);
    hpBefore = GetMonData(TEST_MON, MON_DATA_ATK);
    EXPECT(StatItems_MaxIv(TEST_MON, STAT_ATK));
    EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_ATK_IV), 31);
    EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_DEF_IV), 0);
    EXPECT_GT(GetMonData(TEST_MON, MON_DATA_ATK), hpBefore);   // stats are recalculated
    EXPECT(!StatItems_MaxIv(TEST_MON, STAT_ATK));               // already maxed: not used up
    EXPECT(StatItems_MaxAllIvs(TEST_MON));
    for (i = 0; i < NUM_STATS; i++)
        EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_HP_IV + i), 31);
    EXPECT(!StatItems_MaxAllIvs(TEST_MON));
}

TEST("GymHack: items: Ability Capsule offers every different ability")
{
    u8 slots[NUM_ABILITY_SLOTS];
    u32 slot = 0;
    GiveTestMon(SPECIES_GROWLITHE);   // Intimidate / Flash Fire / Justified (hidden)
    SetMonData(TEST_MON, MON_DATA_ABILITY_NUM, &slot);
    EXPECT_EQ(StatItems_GetAbilityChoices(TEST_MON, slots), 3);
    EXPECT(!StatItems_SetAbilitySlot(TEST_MON, 0));              // the one it has
    EXPECT(StatItems_SetAbilitySlot(TEST_MON, 2));
    EXPECT_EQ(GetMonAbility(TEST_MON), ABILITY_JUSTIFIED);
    GiveTestMon(SPECIES_GASTLY);      // only Levitate
    EXPECT_EQ(StatItems_GetAbilityChoices(TEST_MON, slots), 1);
}

TEST("GymHack: items: Cilantro sets the nature used for stats and shown")
{
    GiveTestMon(SPECIES_GROWLITHE);
    StatItems_SetNature(TEST_MON, NATURE_HARDY);
    {
        u32 neutralAtk = GetMonData(TEST_MON, MON_DATA_ATK);
        EXPECT(StatItems_SetNature(TEST_MON, NATURE_ADAMANT));
        EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_HIDDEN_NATURE), NATURE_ADAMANT);
        EXPECT_GT(GetMonData(TEST_MON, MON_DATA_ATK), neutralAtk);
        EXPECT(!StatItems_SetNature(TEST_MON, NATURE_ADAMANT));
    }
}

TEST("GymHack: items: Multivitamin EVs must stay within the limits")
{
    u16 good[NUM_STATS] = {252, 252, 4, 0, 0, 0};
    u16 tooMuchTotal[NUM_STATS] = {252, 252, 8, 0, 0, 0};
    u16 tooMuchOne[NUM_STATS] = {253, 0, 0, 0, 0, 0};
    GiveTestMon(SPECIES_GROWLITHE);
    EXPECT(StatItems_EvsAreValid(good));
    EXPECT(!StatItems_EvsAreValid(tooMuchTotal));
    EXPECT(!StatItems_EvsAreValid(tooMuchOne));
    EXPECT(!StatItems_SetEvs(TEST_MON, tooMuchTotal));
    EXPECT(StatItems_SetEvs(TEST_MON, good));
    EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_ATK_EV), 252);
    EXPECT_EQ(GetMonData(TEST_MON, MON_DATA_DEF_EV), 4);
}

TEST("GymHack: items: Pallet Mart stocks stat items as they unlock")
{
    const u16 *items;
    u32 i;
    bool32 silver = FALSE, gold = FALSE, capsule = FALSE;
    FlagClear(FLAG_BADGE04_GET);
    FlagClear(FLAG_BADGE06_GET);
    FlagClear(FLAG_BADGE07_GET);
    FlagClear(FLAG_SYS_GAME_CLEAR);
    items = StatItems_GetPalletMartItems();
    for (i = 0; items[i] != ITEM_NONE; i++)
        EXPECT(items[i] != ITEM_BOTTLE_CAP && items[i] != ITEM_MULTIVITAMIN);
    FlagSet(FLAG_BADGE04_GET);
    FlagSet(FLAG_BADGE06_GET);
    items = StatItems_GetPalletMartItems();
    for (i = 0; items[i] != ITEM_NONE; i++)
    {
        silver |= items[i] == ITEM_BOTTLE_CAP;
        capsule |= items[i] == ITEM_ABILITY_CAPSULE;
        gold |= items[i] == ITEM_GOLD_BOTTLE_CAP;
    }
    EXPECT(silver && capsule && !gold);
    EXPECT_EQ(GetItemPrice(ITEM_BOTTLE_CAP), 5000);
    EXPECT_EQ(GetItemPrice(ITEM_GOLD_BOTTLE_CAP), 25000);
    EXPECT_EQ(GetItemPrice(ITEM_ABILITY_CAPSULE), 20000);
    EXPECT_EQ(GetItemPrice(ITEM_CILANTRO), 40000);
    EXPECT_LT(GetItemPrice(ITEM_ADAMANT_MINT), GetItemPrice(ITEM_CILANTRO));
    EXPECT_EQ(GetItemPrice(ITEM_MULTIVITAMIN), 30000);
    FlagClear(FLAG_BADGE04_GET);
    FlagClear(FLAG_BADGE06_GET);
}

// Every line of the new dialogue fits in the message box (27 tiles), with long names filled in
extern const u8 GymHack_Text_BrockSilverCap[], GymHack_Text_BrockSilverCapAfter[], GymHack_Text_MistySilverCap[],
    GymHack_Text_MistySilverCapAfter[], GymHack_Text_SurgeSilverCap[], GymHack_Text_SurgeSilverCapAfter[],
    GymHack_Text_ErikaSilverCap[], GymHack_Text_ErikaCilantro[], GymHack_Text_ErikaCilantroAfter[],
    GymHack_Text_KogaSilverCap[], GymHack_Text_KogaAbilityCapsule[], GymHack_Text_KogaAbilityCapsuleAfter[],
    GymHack_Text_SabrinaSilverCap[], GymHack_Text_SabrinaSilverCapAfter[], GymHack_Text_BlaineSilverCap[],
    GymHack_Text_BlaineSilverCapAfter[], GymHack_Text_GiovanniSilverCap[], GymHack_Text_GiovanniSilverCapAfter[],
    GymHack_Text_RivalGoldCap[], GymHack_Text_RivalGoldCapAfter[], GymHack_Text_SilphGiovanniMultivitamin[],
    GymHack_Text_SilphGiovanniMultivitaminAfter[], GymHack_Text_StatItemEgg[], GymHack_Text_StatItemNoEffect[],
    GymHack_Text_WhichStat[], GymHack_Text_UseGoldBottleCap[], GymHack_Text_WhichAbility[], GymHack_Text_WhichNature[],
    GymHack_Text_MultivitaminUsed[], GymHack_Text_MultivitaminUnchanged[], GymHack_Text_GiftSentToPC[];

TEST("GymHack: items: gift and item messages fit the message box")
{
    static const u8 *const texts[] = {
        GymHack_Text_BrockSilverCap, GymHack_Text_BrockSilverCapAfter, GymHack_Text_MistySilverCap,
        GymHack_Text_MistySilverCapAfter, GymHack_Text_SurgeSilverCap, GymHack_Text_SurgeSilverCapAfter,
        GymHack_Text_ErikaSilverCap, GymHack_Text_ErikaCilantro, GymHack_Text_ErikaCilantroAfter,
        GymHack_Text_KogaSilverCap, GymHack_Text_KogaAbilityCapsule, GymHack_Text_KogaAbilityCapsuleAfter,
        GymHack_Text_SabrinaSilverCap, GymHack_Text_SabrinaSilverCapAfter, GymHack_Text_BlaineSilverCap,
        GymHack_Text_BlaineSilverCapAfter, GymHack_Text_GiovanniSilverCap, GymHack_Text_GiovanniSilverCapAfter,
        GymHack_Text_RivalGoldCap, GymHack_Text_RivalGoldCapAfter, GymHack_Text_SilphGiovanniMultivitamin,
        GymHack_Text_SilphGiovanniMultivitaminAfter, GymHack_Text_StatItemEgg, GymHack_Text_StatItemNoEffect,
        GymHack_Text_WhichStat, GymHack_Text_UseGoldBottleCap, GymHack_Text_WhichAbility, GymHack_Text_WhichNature,
        GymHack_Text_MultivitaminUsed, GymHack_Text_MultivitaminUnchanged, GymHack_Text_GiftSentToPC,
    };
    u8 *buf = Alloc(1000);
    u32 i;
    StringCopy(gStringVar1, COMPOUND_STRING("CHARMANDER"));
    StringCopy(gStringVar2, COMPOUND_STRING("Silver Bottle Cap"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("text %d: line too wide: %S", i, line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

// ---- Gym challenges ----
#include "gym_challenge.h"
#include "data.h"
#include "constants/opponents.h"
#include "battle_setup.h"

extern u8 gGymChallengeIntroText[], gGymChallengeAfterText[], gGymChallengeWinText[];

static void GivePlayerTeamAt(u32 level1, u32 level2)
{
    ZeroPlayerPartyMons();
    ScriptGiveMon(SPECIES_GROWLITHE, level1, ITEM_NONE);
    ScriptGiveMon(SPECIES_PONYTA, level2, ITEM_NONE);
}

static u32 ChallengerAverageLevel(void)
{
    const struct Trainer *t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
    u32 i, total = 0;
    for (i = 0; i < t->partySize; i++)
        total += t->party[i].lvl;
    return (total + t->partySize / 2) / t->partySize;
}

TEST("GymHack: challenge: random challengers bring 4-6 different strong Pokemon at the player's level")
{
    u32 run, i, j;
    GivePlayerTeamAt(50, 60);
    EXPECT_EQ(GymChallenge_PlayerAverageLevel(), 55);
    for (run = 0; run < 50; run++)
    {
        const struct Trainer *t;
        GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
        t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        EXPECT_GE((u32)t->partySize, 4);
        EXPECT_LE((u32)t->partySize, 6);
        EXPECT_EQ(ChallengerAverageLevel(), 55);
        EXPECT_NE(t->trainerName[0], EOS);
        for (i = 0; i < t->partySize; i++)
        {
            EXPECT_GE(t->party[i].species, 1);
            EXPECT_LE(t->party[i].species, SPECIES_DEOXYS);
            EXPECT_GE(GetSpeciesBaseStatTotal(t->party[i].species), 420);
            for (j = 0; j < i; j++)
                EXPECT_NE(t->party[i].species, t->party[j].species);
        }
    }
}

TEST("GymHack: challenge: famous challengers and townsfolk have full teams at the player's level")
{
    u32 i;
    GivePlayerTeamAt(70, 70);
    for (i = 0; i < GymChallenge_CountFamous(); i++)
    {
        const struct Trainer *t;
        GymChallenge_Generate(GYM_CHALLENGER_FAMOUS, i);
        t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        EXPECT_GE((u32)t->partySize, 4);
        EXPECT_GE(ChallengerAverageLevel(), 68);
        EXPECT_LE(ChallengerAverageLevel(), 72);
        EXPECT_NE(t->trainerName[0], EOS);
    }
    for (i = 0; i < GymChallenge_CountTownsfolk(); i++)
    {
        GymChallenge_Generate(GYM_CHALLENGER_TOWNSFOLK, i);
        EXPECT_GE((u32)GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->partySize, 4);
        EXPECT_EQ(ChallengerAverageLevel(), 70);
    }
    // Early-game Pokémon on a famous team grow up (Brock's Geodude at level 70)
    EXPECT_EQ(GymChallenge_EvolveForLevel(SPECIES_GEODUDE, 70), SPECIES_GOLEM);
    EXPECT_EQ(GymChallenge_EvolveForLevel(SPECIES_GEODUDE, 30), SPECIES_GRAVELER);
    EXPECT_EQ(GymChallenge_EvolveForLevel(SPECIES_GEODUDE, 20), SPECIES_GEODUDE);
}

TEST("GymHack: challenge: the battle builds the generated team")
{
    const struct Trainer *t;
    u32 i;
    GivePlayerTeamAt(40, 40);
    GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
    t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
    ZeroEnemyPartyMons();
    CreateNPCTrainerPartyFromTrainer(gParties[B_TRAINER_OPPONENT_A], t);
    for (i = 0; i < t->partySize; i++)
    {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES), t->party[i].species);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_LEVEL), t->party[i].lvl);
    }
}

TEST("GymHack: challenge: wins build a streak, a loss ends it and goes on the statue")
{
    struct GymChallengeSave *save;
    u32 i;
    memset(gSaveBlock1Ptr->secretBases, 0xFF, sizeof(gSaveBlock1Ptr->secretBases));
    save = GymChallenge_GetSave();
    EXPECT_EQ(save->streak, 0);
    EXPECT_EQ(save->numVictors, 0);
    GivePlayerTeamAt(50, 50);
    SetMoney(&gSaveBlock1Ptr->money, 0);
    for (i = 0; i < 3; i++)
    {
        GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
        GymChallenge_RecordWin();
        EXPECT_EQ(gSpecialVar_Result, TRUE);       // each one beats the best streak
        EXPECT_NE(gSpecialVar_0x8009, ITEM_NONE);
    }
    EXPECT_EQ(save->streak, 3);
    EXPECT_EQ(save->bestStreak, 3);
    EXPECT_GT(GetMoney(&gSaveBlock1Ptr->money), 6000);
    GymChallenge_Generate(GYM_CHALLENGER_FAMOUS, GymChallenge_CountFamous() - 1);   // Oak
    GymChallenge_RecordLoss();
    EXPECT_EQ(save->streak, 0);
    EXPECT_EQ(save->bestStreak, 3);
    EXPECT_EQ(save->wins, 3);
    EXPECT_EQ(save->losses, 1);
    EXPECT_EQ(save->numVictors, 1);
    EXPECT_EQ(save->victors[0].streak, 3);
    EXPECT_EQ(StringCompare(save->victors[0].name, COMPOUND_STRING("OAK")), 0);
    GymChallenge_BufferVictorList();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(StringCompare(gStringVar4, COMPOUND_STRING("{PKMN} PROF. OAK")), 0);
    // The decorations are untouched
    EXPECT_EQ(GymDecor_NumPlaced(), 0);
}

TEST("GymHack: challenge: every challenger line fits its text box")
{
    u32 i, run;
    u8 *buf = Alloc(400);
    GivePlayerTeamAt(50, 50);
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("MMMMMMM"));
    for (run = 0; run < 260; run++)
    {
        const u8 *texts[4];
        if (run < GymChallenge_CountFamous())
            GymChallenge_Generate(GYM_CHALLENGER_FAMOUS, run);
        else if (run < GymChallenge_CountFamous() + GymChallenge_CountTownsfolk())
            GymChallenge_Generate(GYM_CHALLENGER_TOWNSFOLK, run - GymChallenge_CountFamous());
        else
            GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
        texts[0] = gGymChallengeIntroText;
        texts[1] = gGymChallengeDefeatText;   // in battle: a narrower box
        texts[2] = gGymChallengeAfterText;
        texts[3] = gGymChallengeWinText;
        for (i = 0; i < 4; i++)
        {
            u8 *line, *p;
            StringCopy(buf, texts[i]);
            for (line = p = buf; ; p++)
            {
                u8 c = *p;
                if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
                {
                    *p = EOS;
                    if (GetStringWidth(FONT_NORMAL, line, 0) > (i == 1 ? 26 * 8 - 16 : 27 * 8 - 4))
                        DebugPrintf("challenger line too wide: %S", line);
                    EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), i == 1 ? 26 * 8 - 16 : 27 * 8 - 4);
                    if (c == EOS)
                        break;
                    line = p + 1;
                }
            }
        }
    }
    Free(buf);
}

extern const u8 GymChallenge_Text_AideNotYet[], GymChallenge_Text_AideWelcome[], GymChallenge_Text_ReadyToBattle[], GymChallenge_Text_NotNow[], GymChallenge_Text_TeamBreaksRules[], GymChallenge_Text_NoStage[], GymChallenge_Text_StageBlocked[], GymChallenge_Text_CantReachSpot[], GymChallenge_Text_TakeYourPlace[], GymChallenge_Text_AnnounceTrainer[], GymChallenge_Text_AnnounceFamous[], GymChallenge_Text_AnnounceTownsfolk[], GymChallenge_Text_Won[], GymChallenge_Text_NewRecord[], GymChallenge_Text_LeftAGift[], GymChallenge_Text_KeepGoing[], GymChallenge_Text_TakeABreak[], GymChallenge_Text_AwardBadge[], GymChallenge_Text_Lost[], GymChallenge_Text_StatueNoLeader[], GymChallenge_Text_StatueLeader[], GymChallenge_Text_StatueUnbeaten[], GymChallenge_Text_StatueVictors[];

TEST("GymHack: challenge: aide and statue messages fit the message box")
{
    static const u8 *const texts[] = {GymChallenge_Text_AideNotYet, GymChallenge_Text_AideWelcome, GymChallenge_Text_ReadyToBattle, GymChallenge_Text_NotNow, GymChallenge_Text_TeamBreaksRules, GymChallenge_Text_NoStage, GymChallenge_Text_StageBlocked, GymChallenge_Text_CantReachSpot, GymChallenge_Text_TakeYourPlace, GymChallenge_Text_AnnounceTrainer, GymChallenge_Text_AnnounceFamous, GymChallenge_Text_AnnounceTownsfolk, GymChallenge_Text_Won, GymChallenge_Text_NewRecord, GymChallenge_Text_LeftAGift, GymChallenge_Text_KeepGoing, GymChallenge_Text_TakeABreak, GymChallenge_Text_AwardBadge, GymChallenge_Text_Lost, GymChallenge_Text_StatueNoLeader, GymChallenge_Text_StatueLeader, GymChallenge_Text_StatueUnbeaten, GymChallenge_Text_StatueVictors};
    u8 *buf = Alloc(600);
    u32 i;
    StringCopy(gStringVar1, COMPOUND_STRING("65535"));
    StringCopy(gStringVar2, COMPOUND_STRING("65535"));
    StringCopy(gStringVar3, COMPOUND_STRING("COOLTRAINER XAVIER"));
    for (i = 0; i < ARRAY_COUNT(texts) * 2; i++)
    {
        u8 *line, *p;
        // Second pass: the texts that show the badge name in STR_VAR_1
        if (i >= ARRAY_COUNT(texts) && texts[i - ARRAY_COUNT(texts)] != GymChallenge_Text_AwardBadge
                                    && texts[i - ARRAY_COUNT(texts)] != GymChallenge_Text_StatueVictors)
            continue;
        StringCopy(gStringVar1, i < ARRAY_COUNT(texts) ? COMPOUND_STRING("65535") : COMPOUND_STRING("INFESTATION BADGE"));
        StringExpandPlaceholders(buf, texts[i % ARRAY_COUNT(texts)]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("aide line too wide: %S", line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

TEST("GymHack: decor: the battle stage is free, always in storage, and never sold")
{
    s32 x, y;
    GymDecor_ClearAll();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_BATTLE_STAGE), 1);
    EXPECT_EQ(gGymDecor[GYM_DECOR_BATTLE_STAGE].price, 0);
    gSpecialVar_0x8004 = GYM_DECOR_BATTLE_STAGE;
    GymDecor_TryBuy();
    EXPECT_NE(gSpecialVar_Result, 0);
    EXPECT(!GymDecor_FindPlaced(GYM_DECOR_BATTLE_STAGE, &x, &y));
    EXPECT(GymDecor_Place(GYM_DECOR_BATTLE_STAGE, 3, 5));
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_BATTLE_STAGE), 0);   // only one
    EXPECT(GymDecor_FindPlaced(GYM_DECOR_BATTLE_STAGE, &x, &y));
    EXPECT_EQ(x, 3);
    EXPECT_EQ(y, 5);
}

#include "gym_badge.h"

TEST("GymHack: badge: every choosable type (and Flying) has a badge and art")
{
    u32 type;
    static const u8 types[] = {TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND, TYPE_ROCK,
        TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC,
        TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY};
    for (type = 0; type < ARRAY_COUNT(types); type++)
        EXPECT(GymBadge_HasArt(types[type]));
    EXPECT_EQ(StringCompare(GymBadge_GetName(TYPE_PSYCHIC), COMPOUND_STRING("ESPN BADGE")), 0);
    EXPECT_EQ(StringCompare(GymBadge_GetName(TYPE_DRAGON), COMPOUND_STRING("EGG BADGE")), 0);
}

TEST("GymHack: badge: the pedestal is sold once the gym is awarded")
{
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    EXPECT(!GymDecor_IsUnlocked(GYM_DECOR_BADGE_PEDESTAL));
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    EXPECT(GymDecor_IsUnlocked(GYM_DECOR_BADGE_PEDESTAL));
    FlagClear(FLAG_PALLET_GYM_AWARDED);
}

TEST("GymHack: the birds are Flying first, and every type can field six species")
{
    static const u16 birds[] = {SPECIES_PIDGEY, SPECIES_PIDGEOTTO, SPECIES_PIDGEOT, SPECIES_SPEAROW, SPECIES_FEAROW,
        SPECIES_WINGULL, SPECIES_PELIPPER, SPECIES_DODUO, SPECIES_DODRIO, SPECIES_NATU, SPECIES_XATU,
        SPECIES_TAILLOW, SPECIES_SWELLOW, SPECIES_HOOTHOOT, SPECIES_NOCTOWL, SPECIES_AERODACTYL, SPECIES_FARFETCHD};
    u32 count[NUMBER_OF_MON_TYPES] = {0};
    u32 i;
    for (i = 0; i < ARRAY_COUNT(birds); i++)
        EXPECT_EQ(gSpeciesInfo[birds[i]].types[0], TYPE_FLYING);
    EXPECT_EQ(gSpeciesInfo[SPECIES_PIDGEY].types[1], TYPE_NORMAL);
    EXPECT_EQ(gSpeciesInfo[SPECIES_AERODACTYL].types[1], TYPE_ROCK);
    for (i = 1; i <= SPECIES_DEOXYS; i++)
        count[gSpeciesInfo[i].types[0]]++;
    for (i = 0; i < NUMBER_OF_MON_TYPES; i++)
    {
        if (i == TYPE_NONE || i == TYPE_MYSTERY || i == TYPE_STELLAR)
            continue;
        if (count[i] < 6)
            DebugPrintf("type %d has only %d species", i, count[i]);
        EXPECT_GE(count[i], 6);
    }
}

static u32 BestTeamBst(u32 type)
{
    u32 best[6] = {0}, i, j, total = 0;
    for (i = 1; i <= SPECIES_DEOXYS; i++)
    {
        u32 bst = GetSpeciesBaseStatTotal(i);
        if (gSpeciesInfo[i].types[0] != type)
            continue;
        // Johto's and Hoenn's legendaries are only in the Exotic Safari (post-game)
        if ((i >= SPECIES_RAIKOU && i <= SPECIES_SUICUNE) || (i >= SPECIES_LUGIA && i <= SPECIES_CELEBI)
         || (i >= SPECIES_REGIROCK && i <= SPECIES_DEOXYS))
            continue;
        for (j = 0; j < 6; j++)
        {
            if (bst > best[j])
            {
                u32 m;
                for (m = 5; m > j; m--)
                    best[m] = best[m - 1];
                best[j] = bst;
                break;
            }
        }
    }
    for (j = 0; j < 6; j++)
        total += best[j];
    return total;
}

TEST("GymHack: Oak's type menu runs from the strongest best team to the weakest")
{
    u32 i;
    for (i = 0; i + 1 < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 a = BestTeamBst(gSpeciesInfo[GymHackTest_ChooseStarter(i)].types[0]);
        u32 b = BestTeamBst(gSpeciesInfo[GymHackTest_ChooseStarter(i + 1)].types[0]);
        EXPECT_GE(a, b);
    }
    EXPECT_EQ(gSpeciesInfo[GymHackTest_ChooseStarter(GYM_HACK_HARD_STARTERS_FROM)].types[0], TYPE_FIGHTING);
}

extern const u8 PalletTown_ProfessorOaksLab_Text_HardType[], PalletTown_ProfessorOaksLab_Text_OakThreeMonsChooseOne[];

TEST("GymHack: Oak's type-choice lines fit the message box")
{
    static const u8 *const texts[] = {PalletTown_ProfessorOaksLab_Text_HardType, PalletTown_ProfessorOaksLab_Text_OakThreeMonsChooseOne};
    u8 *buf = Alloc(1200);
    u32 i;
    StringCopy(gStringVar2, COMPOUND_STRING("Fighting"));
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("MMMMMMM"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("oak line too wide: %S", line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

extern const u8 GymFinale_Text_OakComeToTheGym[], GymFinale_Text_OakHereWeAre[], GymFinale_Text_OakThinking[], GymFinale_Text_OakWontGiveItUp[], GymFinale_Text_OakYouCameBack[], GymFinale_Text_OakWillYouBattle[], GymFinale_Text_OakThenShowMe[], GymFinale_Text_OakComeBackWhenReady[], GymFinale_Text_OakRulesStillApply[], GymFinale_Text_OakStillHasIt[], GymFinale_Text_OakGivesUp[], GymFinale_Text_YourBadge[], GymFinale_Text_OakMeetMyAide[], GymFinale_Text_AideHello[], GymFinale_Text_OakHowItWorks[], GymFinale_Text_OakWait[], GymFinale_Text_OakMadeIt[], GymFinale_Text_PokemonHealed[], GymFinale_Text_OakThisBattleDecides[], GymFinale_Text_OakCongratulations[], GymFinale_Text_OakToRival[];

TEST("GymHack: finale: Oak's team for the gym is six strong Pokemon above the player's level")
{
    const struct Trainer *t;
    u32 i;
    GivePlayerTeamAt(50, 54);
    GymChallenge_GenerateOakFinale();
    t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
    EXPECT_EQ((u32)t->partySize, 6);
    for (i = 0; i < 6; i++)
    {
        EXPECT_EQ(t->party[i].lvl, 65);          // never below 65
        EXPECT_NE(t->party[i].moves[0], MOVE_NONE);
        EXPECT_NE(t->party[i].heldItem, ITEM_NONE);
    }
    GivePlayerTeamAt(80, 82);
    GymChallenge_GenerateOakFinale();
    EXPECT_EQ(GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->party[0].lvl, 86);
    EXPECT_EQ(GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->trainerClass, TRAINER_CLASS_PKMN_PROF_FRLG);
}

TEST("GymHack: finale: the gym challenges open once the gym is awarded")
{
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagSet(FLAG_SYS_GAME_CLEAR);
    EXPECT(!GymChallenge_IsOpen());
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    EXPECT(GymChallenge_IsOpen());
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_SYS_GAME_CLEAR);
}

TEST("GymHack: finale: every line fits the message box")
{
    static const u8 *const texts[] = {GymFinale_Text_OakComeToTheGym, GymFinale_Text_OakHereWeAre, GymFinale_Text_OakThinking, GymFinale_Text_OakWontGiveItUp, GymFinale_Text_OakYouCameBack, GymFinale_Text_OakWillYouBattle, GymFinale_Text_OakThenShowMe, GymFinale_Text_OakComeBackWhenReady, GymFinale_Text_OakRulesStillApply, GymFinale_Text_OakStillHasIt, GymFinale_Text_OakGivesUp, GymFinale_Text_YourBadge, GymFinale_Text_OakMeetMyAide, GymFinale_Text_AideHello, GymFinale_Text_OakHowItWorks, GymFinale_Text_OakWait, GymFinale_Text_OakMadeIt, GymFinale_Text_PokemonHealed, GymFinale_Text_OakThisBattleDecides, GymFinale_Text_OakCongratulations, GymFinale_Text_OakToRival};
    u8 *buf = Alloc(1600);
    u32 i;
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("MMMMMMM"));
    StringCopy(gStringVar1, COMPOUND_STRING("INFESTATION BADGE"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("finale line too wide: %S", line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

extern const u8 GymAides_Text_PewterCity[], GymAides_Text_CeruleanCity[], GymAides_Text_VermilionCity[], GymAides_Text_CeladonCity[], GymAides_Text_FuchsiaCity[], GymAides_Text_SaffronCity[], GymAides_Text_CinnabarIsland[], GymAides_Text_ViridianCity[], GymAides_Text_PewterCityAfter[], GymAides_Text_CeruleanCityAfter[], GymAides_Text_VermilionCityAfter[], GymAides_Text_CeladonCityAfter[], GymAides_Text_FuchsiaCityAfter[], GymAides_Text_SaffronCityAfter[], GymAides_Text_CinnabarIslandAfter[], GymAides_Text_ViridianCityAfter[];

TEST("GymHack: gym aides: lines fit the message box")
{
    static const u8 *const texts[] = {GymAides_Text_PewterCity, GymAides_Text_CeruleanCity, GymAides_Text_VermilionCity, GymAides_Text_CeladonCity, GymAides_Text_FuchsiaCity, GymAides_Text_SaffronCity, GymAides_Text_CinnabarIsland, GymAides_Text_ViridianCity, GymAides_Text_PewterCityAfter, GymAides_Text_CeruleanCityAfter, GymAides_Text_VermilionCityAfter, GymAides_Text_CeladonCityAfter, GymAides_Text_FuchsiaCityAfter, GymAides_Text_SaffronCityAfter, GymAides_Text_CinnabarIslandAfter, GymAides_Text_ViridianCityAfter};
    u8 *buf = Alloc(1200);
    u32 i;
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("aide line too wide: %S", line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

// ---- Practice trainers ----
extern const u8 GymPractice_Text_Offer[], GymPractice_Text_Go[], GymPractice_Text_Won[], GymPractice_Text_Lost[], GymPractice_Text_NotNow[];

TEST("GymHack: practice trainers bring 3-5 different Pokemon of the player's type, 5 levels below")
{
    static const u8 types[] = {TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND, TYPE_ROCK,
        TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC,
        TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY};
    u32 t, tier, run, i, j;
    GivePlayerTeamAt(50, 60);
    for (t = 0; t < ARRAY_COUNT(types); t++)
    {
        VarSet(VAR_PLAYER_TYPE, types[t]);
        for (tier = 0; tier < 3; tier++)
        {
            for (run = 0; run < 6; run++)
            {
                const struct Trainer *tr;
                GymChallenge_GeneratePracticeTier(tier, run);
                tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
                EXPECT(gGymPracticeBattle);
                EXPECT_GE((u32)tr->partySize, 3);
                EXPECT_LE((u32)tr->partySize, 5);
                EXPECT_EQ(ChallengerAverageLevel(), 50);
                for (i = 0; i < tr->partySize; i++)
                {
                    u32 s = tr->party[i].species;
                    EXPECT_EQ(gSpeciesInfo[s].types[0], types[t]);
                    EXPECT(!gSpeciesInfo[s].isRestrictedLegendary && !gSpeciesInfo[s].isSubLegendary && !gSpeciesInfo[s].isMythical);
                    for (j = 0; j < i; j++)
                        EXPECT_NE(GET_BASE_SPECIES_ID(s), GET_BASE_SPECIES_ID(tr->party[j].species));
                }
            }
        }
    }
    GymChallenge_EndPractice();
    EXPECT(!gGymPracticeBattle);
}

static u32 CountFullyEvolved(u32 type)
{
    u32 s, n = 0;
    for (s = 1; s <= SPECIES_DEOXYS; s++)
    {
        const struct Evolution *evos = GetSpeciesEvolutions(s);
        if (IsSpeciesEnabled(s) && gSpeciesInfo[s].types[0] == type && !gSpeciesInfo[s].isRestrictedLegendary
         && !gSpeciesInfo[s].isSubLegendary && !gSpeciesInfo[s].isMythical && (evos == NULL || evos[0].method == EVOLUTIONS_END))
            n++;
    }
    return n;
}

TEST("GymHack: practice: the expert uses fully evolved Pokemon, earlier forms only when short")
{
    u32 run, i, full;
    GivePlayerTeamAt(40, 40);
    VarSet(VAR_PLAYER_TYPE, TYPE_FIRE);
    for (run = 0; run < 20; run++)
    {
        const struct Trainer *tr;
        GymChallenge_GeneratePracticeTier(2, 0);
        tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        for (i = 0; i < tr->partySize; i++)
        {
            const struct Evolution *evos = GetSpeciesEvolutions(tr->party[i].species);
            EXPECT(evos == NULL || evos[0].method == EVOLUTIONS_END);
        }
    }
    // Fairy has few fully evolved Pokémon in the first three generations: the team still fills up
    VarSet(VAR_PLAYER_TYPE, TYPE_FAIRY);
    full = CountFullyEvolved(TYPE_FAIRY);
    for (run = 0; run < 20; run++)
    {
        const struct Trainer *tr;
        u32 evolved = 0;
        GymChallenge_GeneratePracticeTier(2, 0);
        tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        EXPECT_GE((u32)tr->partySize, 3);
        for (i = 0; i < tr->partySize; i++)
        {
            const struct Evolution *evos = GetSpeciesEvolutions(tr->party[i].species);
            if (evos == NULL || evos[0].method == EVOLUTIONS_END)
                evolved++;
        }
        EXPECT_EQ(evolved, min(full, (u32)tr->partySize));
    }
    GymChallenge_EndPractice();
}

TEST("GymHack: practice: the rookie never uses third forms when there are enough others")
{
    u32 run, i;
    GivePlayerTeamAt(30, 30);
    VarSet(VAR_PLAYER_TYPE, TYPE_WATER);
    for (run = 0; run < 30; run++)
    {
        const struct Trainer *tr;
        GymChallenge_GeneratePracticeTier(0, 0);
        tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        for (i = 0; i < tr->partySize; i++)
        {
            u32 pre = GetSpeciesPreEvolution(tr->party[i].species);
            EXPECT(pre == SPECIES_NONE || GetSpeciesPreEvolution(pre) == SPECIES_NONE);
        }
    }
    GymChallenge_EndPractice();
}

TEST("GymHack: practice trainer lines fit the message box")
{
    static const u8 *const texts[] = {GymPractice_Text_Offer, GymPractice_Text_Go, GymPractice_Text_Won, GymPractice_Text_Lost, GymPractice_Text_NotNow};
    u8 *buf = Alloc(400);
    u32 i;
    StringCopy(gStringVar1, COMPOUND_STRING("MMMMMMM"));
    StringCopy(gStringVar2, COMPOUND_STRING("SKILLED"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        for (line = p = buf; ; p++)
        {
            u8 c = *p;
            if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                *p = EOS;
                if (GetStringWidth(FONT_NORMAL, line, 0) > 27 * 8 - 4)
                    DebugPrintf("practice line too wide: %S", line);
                EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), 27 * 8 - 4);
                if (c == EOS)
                    break;
                line = p + 1;
            }
        }
    }
    Free(buf);
}

TEST("GymHack: type decorations unlock with their battle, and all of them with the gym")
{
    u32 t;
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_BADGE02_GET);
    EXPECT(!GymDecor_IsTypeUnlocked(TYPE_WATER));
    EXPECT(!GymDecor_IsUnlocked(GYM_DECOR_WATER_AQUARIUM));
    FlagSet(FLAG_BADGE02_GET);
    EXPECT(GymDecor_IsTypeUnlocked(TYPE_WATER));
    EXPECT(GymDecor_IsUnlocked(GYM_DECOR_WATER_AQUARIUM));
    EXPECT(!GymDecor_IsUnlocked(GYM_DECOR_DRAGON_GOLD_HOARD));
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t != TYPE_MYSTERY && t != TYPE_STELLAR)
            EXPECT(GymDecor_IsTypeUnlocked(t));
    }
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_BADGE02_GET);
}

TEST("GymHack: every type has at least five decorations")
{
    u32 counts[NUMBER_OF_MON_TYPES] = {0}, i, t;
    for (i = 0; i < GYM_DECOR_COUNT; i++)
    {
        if (gGymDecor[i].category >= GYM_DECOR_CAT_TYPE(0))
            counts[gGymDecor[i].category - GYM_DECOR_CAT_TYPE(0)]++;
    }
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t != TYPE_MYSTERY && t != TYPE_STELLAR)
            EXPECT_GE(counts[t], 5);
    }
}

TEST("GymHack: practice trainers: any number can be bought, 11 placed at once")
{
    u32 i, x = 0, y = 4;
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    for (i = 0; i < 14; i++)
    {
        gSpecialVar_0x8004 = GYM_DECOR_TRAINER_BEGINNER + i % 3;
        GymDecor_TryBuy();
        EXPECT_EQ(gSpecialVar_Result, 0);
    }
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TRAINER_BEGINNER), 5);
    for (i = 0; i < 14; i++)
    {
        bool32 placed = GymDecor_Place(GYM_DECOR_TRAINER_BEGINNER + i % 3, x, y);
        EXPECT_EQ(placed, i < GYM_PRACTICE_SLOTS);
        x += 2;
        if (x > 20)
        {
            x = 0;
            y += 2;
        }
    }
    EXPECT_EQ(GymDecor_CountPlacedTrainers(), GYM_PRACTICE_SLOTS);
    {
        u32 tier, instance;
        s32 px, py;
        EXPECT(GymDecor_GetTrainerSlot(3, &tier, &instance, &px, &py));
        EXPECT_EQ(tier, GYM_DECOR_TRAINER_BEGINNER);
        EXPECT_EQ(instance, 1);
        EXPECT(!GymDecor_GetTrainerSlot(GYM_PRACTICE_SLOTS, &tier, &instance, &px, &py));
    }
    GymDecor_ClearAll();
}

// ---- Gym candidate (src/gym_candidate.c, data/scripts/gym_candidate.inc) ----
#include "gym_candidate.h"

extern const u8 GymCandidate_Text_LaterThatDay[], GymCandidate_Text_AnotherOne[], GymCandidate_Text_ApprovalCount[], GymCandidate_Text_TryAgain[], GymCandidate_Text_NotYet[], GymCandidate_Text_TeamBreaksRules[], GymCandidate_Text_NeedApprovals[], GymCandidate_Text_OakAside1[], GymCandidate_Text_OakAside2[], GymCandidate_Text_OakAside3[], GymCandidate_Text_OakAside4[], GymCandidate_Text_DaisyAside[], GymCandidate_Text_RecruitJoined[], GymCandidate_Text_RecruitHowToPlace[], GymCandidate_Text_RecruitDeclined[], CeruleanCity_Text_RivalPostBattle[], SSAnne_2F_Corridor_Text_RivalIntro[], PokemonTower_2F_Text_RivalPostBattle[], SilphCo_7F_Text_RivalPostBattle[], Route22_Text_EarlyRivalPostBattle[], Route22_Text_LateRivalIntro[], Route22_Text_LateRivalPostBattle[], PalletTown_Gym_Text_OakWaitingForYou[], GymChallenge_Text_AideNotYet[];

static void SetLongPlaceholders(void)
{
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("MMMMMMM"));
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("MMMMMMM"));
    StringCopy(gStringVar1, COMPOUND_STRING("BIRD KEEPER PERRY"));
    StringCopy(gStringVar2, COMPOUND_STRING("ELECTRIC"));
    StringCopy(gStringVar3, COMPOUND_STRING("LT. SURGE"));
}

// Every line of an (expanded) text fits: max width in pixels
static void ExpectLinesFit(const u8 *text, u32 maxWidth, bool32 expand)
{
    u8 *buf = Alloc(1000);
    u8 *line, *p;
    if (expand)
        StringExpandPlaceholders(buf, text);
    else
        StringCopy(buf, text);
    for (line = p = buf; ; p++)
    {
        u8 c = *p;
        if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
        {
            *p = EOS;
            if (GetStringWidth(FONT_NORMAL, line, 0) > maxWidth)
                DebugPrintf("line too wide: %S", line);
            EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), maxWidth);
            if (c == EOS)
                break;
            line = p + 1;
        }
    }
    Free(buf);
}

TEST("GymHack: candidate: all the new story lines fit the message box")
{
    static const u8 *const texts[] = {GymCandidate_Text_LaterThatDay, GymCandidate_Text_AnotherOne, GymCandidate_Text_ApprovalCount, GymCandidate_Text_TryAgain, GymCandidate_Text_NotYet, GymCandidate_Text_TeamBreaksRules, GymCandidate_Text_NeedApprovals, GymCandidate_Text_OakAside1, GymCandidate_Text_OakAside2, GymCandidate_Text_OakAside3, GymCandidate_Text_OakAside4, GymCandidate_Text_DaisyAside, GymCandidate_Text_RecruitJoined, GymCandidate_Text_RecruitHowToPlace, GymCandidate_Text_RecruitDeclined, CeruleanCity_Text_RivalPostBattle, SSAnne_2F_Corridor_Text_RivalIntro, PokemonTower_2F_Text_RivalPostBattle, SilphCo_7F_Text_RivalPostBattle, Route22_Text_EarlyRivalPostBattle, Route22_Text_LateRivalIntro, Route22_Text_LateRivalPostBattle, PalletTown_Gym_Text_OakWaitingForYou, GymChallenge_Text_AideNotYet};
    u32 i, t;
    SetLongPlaceholders();
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
    for (i = 0; i < GYM_CANDIDATE_LEADERS; i++)
    {
        ExpectLinesFit(gGymCandidateLeaders[i].offer, 27 * 8 - 4, TRUE);
        ExpectLinesFit(gGymCandidateLeaders[i].approve, 27 * 8 - 4, TRUE);
        ExpectLinesFit(gGymCandidateLeaders[i].repeatOffer, 27 * 8 - 4, TRUE);
        ExpectLinesFit(gGymCandidateLeaders[i].repeatThanks, 27 * 8 - 4, TRUE);
        ExpectLinesFit(gGymCandidateLeaders[i].ownGym, 27 * 8 - 4, TRUE);
        ExpectLinesFit(gGymCandidateLeaders[i].observe, 27 * 8 - 4, TRUE);
    }
    for (i = 0; i < GYM_CANDIDATE_ELITE_FOUR; i++)
        ExpectLinesFit(GymCandidate_GetEliteFourApproval(i), 27 * 8 - 4, TRUE);
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        const struct GymCandidateMentor *mentor = GymCandidate_GetMentorForType(t);
        const struct GymRecruit *recruit = GymCandidate_GetRecruit(t);
        if (mentor != NULL)
        {
            ExpectLinesFit(mentor->intro, 27 * 8 - 4, TRUE);
            ExpectLinesFit(mentor->first, 27 * 8 - 4, TRUE);
            ExpectLinesFit(mentor->approve, 27 * 8 - 4, TRUE);
        }
        if (recruit != NULL)
            ExpectLinesFit(recruit->pitch, 27 * 8 - 4, TRUE);
    }
}

TEST("GymHack: candidate: every type has a mentor and a trainer who can join")
{
    u32 t;
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        EXPECT(GymCandidate_GetMentorForType(t) != NULL);
        EXPECT(GymCandidate_GetRecruit(t) != NULL);
        EXPECT_EQ(gGymDecor[GymCandidate_GetRecruit(t)->decor].category, GYM_DECOR_CAT_TRAINERS);
        EXPECT_EQ(gGymDecor[GymCandidate_GetRecruit(t)->decor].price, 0);
    }
}

TEST("GymHack: candidate: guest challengers grow with the story, and their lines fit")
{
    u32 leader, run, i;
    GivePlayerTeamAt(10, 10);
    for (leader = 0; leader < GYM_CANDIDATE_LEADERS; leader++)
    {
        for (run = 0; run < 8; run++)
        {
            const struct Trainer *tr;
            gSpecialVar_0x8009 = leader;
            gSpecialVar_0x8007 = 0;
            GymCandidate_GenerateGuest();
            EXPECT_EQ(gGymGuestBattle, 2);           // they pay the stand-in (a doubled Gym Leader prize)
            EXPECT(!gGymPracticeBattle);
            tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
            EXPECT_EQ((u32)tr->partySize, (u32)gGymCandidateLeaders[leader].partySize);
            for (i = 0; i < tr->partySize; i++)
                EXPECT_GE((u32)tr->party[i].lvl, (u32)gGymCandidateLeaders[leader].level - 2);
            ExpectLinesFit(gGymChallengeIntroText, 27 * 8 - 4, FALSE);
            ExpectLinesFit(gGymChallengeAfterText, 27 * 8 - 4, FALSE);
            ExpectLinesFit(gGymChallengeWinText, 27 * 8 - 4, FALSE);
            ExpectLinesFit(gGymChallengeDefeatText, 26 * 8 - 16, FALSE);
        }
    }
    EXPECT_EQ(gGymCandidateLeaders[0].guests, 1);
    EXPECT_GE(gGymCandidateLeaders[6].guests, 2);
    GymChallenge_EndPractice();
}

TEST("GymHack: candidate: approvals count up, and a leader asks until they approve")
{
    u32 i;
    for (i = 0; i < GYM_CANDIDATE_LEADERS; i++)
        FlagClear(gGymCandidateLeaders[i].approvalFlag);
    EXPECT_EQ(GymCandidate_CountApprovals(), 0);
    gSpecialVar_0x8009 = 2;
    GymCandidate_IsApproved();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    GymCandidate_Approve();
    GymCandidate_IsApproved();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT_EQ(GymCandidate_CountApprovals(), 1);
    gSpecialVar_0x8009 = 0;
    gSpecialVar_0x800B = 0;
    GymCandidate_MoreGuests();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    gSpecialVar_0x800B = 1;
    GymCandidate_MoreGuests();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    gSpecialVar_0x8009 = 6;
    GymCandidate_MoreGuests();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    for (i = 0; i < GYM_CANDIDATE_LEADERS; i++)
        FlagClear(gGymCandidateLeaders[i].approvalFlag);
}

TEST("GymHack: candidate: a recruit joins once and can be placed as a trainer")
{
    GymDecor_ClearAll();
    gSpecialVar_0x8004 = TYPE_FAIRY;
    GymCandidate_BufferRecruit();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    GymCandidate_Recruit();
    GymCandidate_BufferRecruit();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT(GymDecor_Place(GymCandidate_GetRecruit(TYPE_FAIRY)->decor, 3, 5));
    EXPECT_EQ(GymDecor_CountPlacedTrainers(), 1);
    GymDecor_ClearAll();
}

TEST("GymHack: candidate: after approval, a shift is one challenger near the player's level")
{
    u32 leader, run, i, total, avg;
    GivePlayerTeamAt(40, 40);
    avg = GymChallenge_PlayerAverageLevel();
    for (leader = 0; leader < GYM_CANDIDATE_LEADERS; leader++)
    {
        gSpecialVar_0x8009 = leader;
        gSpecialVar_0x8007 = 1;
        gSpecialVar_0x800B = 1;
        GymCandidate_MoreGuests();
        EXPECT_EQ(gSpecialVar_Result, FALSE);   // just one, even for Koga and on
        for (run = 0; run < 8; run++)
        {
            const struct Trainer *tr;
            GymCandidate_GenerateGuest();
            EXPECT_EQ(gGymGuestBattle, 1);
            tr = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
            total = 0;
            for (i = 0; i < tr->partySize; i++)
            {
                EXPECT_GE((u32)tr->party[i].lvl + 2, avg - 2);
                EXPECT_LE((u32)tr->party[i].lvl, avg + 5);
                total += tr->party[i].lvl;
            }
            EXPECT_GE(total / tr->partySize, avg - 2);
            EXPECT_LE(total / tr->partySize, avg + 3);
        }
    }
    gSpecialVar_0x8007 = 0;
    GymChallenge_EndPractice();
}

// ---- Move Extractor (src/move_extractor.c) ----
#include "move_extractor.h"
extern const u8 MoveExtractor_Text_WantToJoin[], MoveExtractor_Text_ChangedYourMind[], MoveExtractor_Text_Hmph[], MoveExtractor_Text_Surprised[], MoveExtractor_Text_HowItWorks[], MoveExtractor_Text_KeepStealing[], MoveExtractor_Text_SecondVisit[], MoveExtractor_Text_SecondVisitAfter[], MoveExtractor_Text_ThirdVisit[], MoveExtractor_Text_ThirdVisitAfter[], Route24_Text_JoinTeamRocket[];

TEST("GymHack: Move Extractor: an extracted move keeps its mark through swaps and shifts")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u32 stolen;
    enum Move moves[] = {MOVE_TACKLE, MOVE_GROWL, MOVE_WATER_GUN, MOVE_EMBER};
    u32 i;
    CreateMon(mon, SPECIES_BULBASAUR, 20, 0, OTID_STRUCT_PLAYER_ID);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
    EXPECT(MoveExtractor_MarkLearned(mon, MOVE_WATER_GUN));
    EXPECT(IsStolenMove(mon, MOVE_WATER_GUN));
    EXPECT(!IsStolenMove(mon, MOVE_EMBER));
    EXPECT_EQ(CheckDynamicMoveType(mon, MOVE_WATER_GUN, 0, MON_OUTSIDE_BATTLE), TYPE_GRASS);
    EXPECT_EQ(CheckDynamicMoveType(mon, MOVE_EMBER, 0, MON_OUTSIDE_BATTLE), TYPE_FIRE);
    // Swapping slots carries the mark
    stolen = SwapStolenMoveBits(GetMonData(mon, MON_DATA_STOLEN_MOVES), 2, 0);
    EXPECT_EQ(stolen, 1u);
    // Forgetting the first move shifts the rest up, mark included
    DeleteFirstMoveAndGiveMoveToMon(mon, MOVE_SCRATCH);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE2), MOVE_WATER_GUN);
    EXPECT(IsStolenMove(mon, MOVE_WATER_GUN));
    // Replacing it with another move clears the mark
    SetMonMoveSlot(mon, MOVE_BUBBLE, 1);
    EXPECT_EQ(GetMonData(mon, MON_DATA_STOLEN_MOVES), 0);
    EXPECT_EQ(CheckDynamicMoveType(mon, MOVE_BUBBLE, 0, MON_OUTSIDE_BATTLE), TYPE_WATER);
}

TEST("GymHack: Move Extractor: the grunt's lines fit the message box")
{
    static const u8 *const texts[] = {MoveExtractor_Text_WantToJoin, MoveExtractor_Text_ChangedYourMind, MoveExtractor_Text_Hmph, MoveExtractor_Text_Surprised, MoveExtractor_Text_HowItWorks, MoveExtractor_Text_KeepStealing, MoveExtractor_Text_SecondVisit, MoveExtractor_Text_SecondVisitAfter, MoveExtractor_Text_ThirdVisit, MoveExtractor_Text_ThirdVisitAfter, Route24_Text_JoinTeamRocket};
    u32 i;
    SetLongPlaceholders();
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
}

TEST("GymHack: Move Extractor: the grunt shows up in Saffron after Silph Co., Viridian after Giovanni's approval")
{
    VarSet(VAR_MOVE_EXTRACTOR_STAGE, 1);
    FlagClear(FLAG_GYM_APPROVAL_GIOVANNI);
    gSpecialVar_0x8004 = 3;
    MoveExtractor_VisitDue();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    VarSet(VAR_MOVE_EXTRACTOR_STAGE, 2);
    MoveExtractor_VisitDue();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    FlagSet(FLAG_GYM_APPROVAL_GIOVANNI);
    MoveExtractor_VisitDue();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    VarSet(VAR_MOVE_EXTRACTOR_STAGE, 3);
    MoveExtractor_VisitDue();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    FlagClear(FLAG_GYM_APPROVAL_GIOVANNI);
    VarSet(VAR_MOVE_EXTRACTOR_STAGE, 0);
}

// ---- The mentors' type items (HOLD_EFFECT_TYPE_GUARD/EDGE, src/gym_candidate.c) ----
#include "constants/event_objects.h"
extern const u8 GymCandidate_Text_MentorAtDoor[], GymCandidate_Text_OakAfterMentor[], GymCandidate_Text_SomeoneToSeeYou[], GymCandidate_Text_GuardItemExplain[], GymCandidate_Text_EdgeItemExplain[], PalletTown_ProfessorOaksLab_Text_OakCustomBallIOrdered[], ViridianCity_Mart_Text_TakeThisToProfOak[];

TEST("GymHack: type items: every type has a defensive and an offensive item of its own")
{
    const void *icons[NUMBER_OF_MON_TYPES * 2];
    u32 t, i, j, n = 0;
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        u16 guard = GymCandidate_GetTypeItem(t, FALSE), edge = GymCandidate_GetTypeItem(t, TRUE);
        if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        EXPECT_EQ(GetItemHoldEffect(guard), HOLD_EFFECT_TYPE_GUARD);
        EXPECT_EQ(GetItemHoldEffect(edge), HOLD_EFFECT_TYPE_EDGE);
        EXPECT_EQ(GetItemHoldEffectParam(guard), t);
        EXPECT_EQ(GetItemHoldEffectParam(edge), t);
        EXPECT_EQ(GetItemPrice(guard), 0);
        EXPECT(!GetItemImportance(edge));   // they can be held
        EXPECT(!GetItemImportance(guard));
        icons[n++] = gItemsInfo[guard].iconPic;
        icons[n++] = gItemsInfo[edge].iconPic;
    }
    EXPECT_EQ(n, 36);
    for (i = 0; i < n; i++)
        for (j = 0; j < i; j++)
            EXPECT(icons[i] != icons[j]);
}

TEST("GymHack: mentor: the new lines fit the message box")
{
    u32 who;
    static const u8 *const texts[] = {GymCandidate_Text_MentorAtDoor, GymCandidate_Text_OakAfterMentor, GymCandidate_Text_SomeoneToSeeYou, PalletTown_ProfessorOaksLab_Text_OakCustomBallIOrdered, ViridianCity_Mart_Text_TakeThisToProfOak};
    SetLongPlaceholders();
    for (who = 0; who < ARRAY_COUNT(texts); who++)
        ExpectLinesFit(texts[who], 27 * 8 - 4, TRUE);
    for (who = 0; who < GYM_CANDIDATE_LEADERS + GYM_CANDIDATE_ELITE_FOUR; who++)
    {
        ExpectLinesFit(GymCandidate_GetMentorGiftText(who, FALSE), 27 * 8 - 4, TRUE);
        ExpectLinesFit(GymCandidate_GetMentorGiftText(who, TRUE), 27 * 8 - 4, TRUE);
    }
    CopyItemName(ITEM_GRANITE_SARCOPHAGUS, gStringVar1);
    ExpectLinesFit(GymCandidate_Text_GuardItemExplain, 27 * 8 - 4, TRUE);
    CopyItemName(ITEM_DIAMOND_DRILL_BIT, gStringVar1);
    ExpectLinesFit(GymCandidate_Text_EdgeItemExplain, 27 * 8 - 4, TRUE);
    CopyItemName(ITEM_CORROSIVE_VENOM, gStringVar1);
    ExpectLinesFit(GymCandidate_Text_EdgeItemExplain, 27 * 8 - 4, TRUE);
}

TEST("GymHack: mentor: meets the player once, then gives each item once")
{
    VarSet(VAR_PLAYER_TYPE, TYPE_ICE);
    VarSet(VAR_MENTOR_STAGE, 0);
    GymCandidate_PrepareMentorIntro();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT_EQ(VarGet(VAR_OBJ_GFX_ID_0), OBJ_EVENT_GFX_LORELEI);
    GymCandidate_PrepareMentorIntro();
    EXPECT_EQ(gSpecialVar_Result, FALSE);

    gSpecialVar_0x8009 = 0;     // Brock: nothing
    GymCandidate_PrepareMentorGift();
    EXPECT_EQ(gSpecialVar_Result, MENTOR_GIFT_NONE);
    gSpecialVar_0x8009 = 1;     // Misty: Lorelei visits with the defensive item
    GymCandidate_PrepareMentorGift();
    EXPECT_EQ(gSpecialVar_Result, MENTOR_GIFT_VISIT);
    EXPECT_EQ(gSpecialVar_0x8004, ITEM_COOLER);
    GymCandidate_PrepareMentorGift();
    EXPECT_EQ(gSpecialVar_Result, MENTOR_GIFT_NONE);
    gSpecialVar_0x8009 = 6;     // Blaine: the offensive one
    GymCandidate_PrepareMentorGift();
    EXPECT_EQ(gSpecialVar_Result, MENTOR_GIFT_VISIT);
    EXPECT_EQ(gSpecialVar_0x8004, ITEM_ROCK_SALT);

    VarSet(VAR_PLAYER_TYPE, TYPE_FIRE);     // Blaine hands his over himself
    VarSet(VAR_MENTOR_STAGE, 0);
    GymCandidate_PrepareMentorGift();
    EXPECT_EQ(gSpecialVar_Result, MENTOR_GIFT_LEADER);
    EXPECT_EQ(gSpecialVar_0x8004, ITEM_OXIDIZER);
    GymCandidate_BufferMentorGift();
    EXPECT(StringCompareN(gStringVar4, COMPOUND_STRING("BLAINE: Hah! One more quiz!"), 27) == 0);
}

// ---- Quality of life: the Palm Pilot and the quick heal pad ----
#include "item_use.h"
#include "metatile_behavior.h"
#include "constants/metatile_behaviors.h"
#include "constants/maps.h"
#include "tilesets.h"
extern const u8 Text_BillPalmPilot[], Text_BillPalmPilotHowTo[], Text_HealPadInnovator[], Text_PalmPilotBoot[], Text_PalmPilotNoSignal[];

TEST("GymHack: the Pokémon Center tileset has the heal pad")
{
    u32 attr = ((const u32 *)gTileset_PokemonCenterFrlg.metatileAttributes)[255];
    EXPECT_EQ(attr & 0x1FF, MB_HEAL_PAD);
    EXPECT(MetatileBehavior_IsHealPad(attr & 0x1FF));
}

TEST("GymHack: the Palm Pilot is a key item that works outside the League")
{
    EXPECT_EQ(GetItemPocket(ITEM_PALM_PILOT), POCKET_KEY_ITEMS);
    EXPECT(GetItemImportance(ITEM_PALM_PILOT));
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_VIRIDIAN_CITY);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_VIRIDIAN_CITY);
    EXPECT(PalmPilot_HasSignal());
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_POKEMON_LEAGUE_LANCES_ROOM);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_POKEMON_LEAGUE_LANCES_ROOM);
    EXPECT(!PalmPilot_HasSignal());
}

TEST("GymHack: Palm Pilot and heal pad lines fit the message box")
{
    static const u8 *const texts[] = {Text_BillPalmPilot, Text_BillPalmPilotHowTo, Text_HealPadInnovator, Text_PalmPilotBoot, Text_PalmPilotNoSignal};
    u32 i;
    SetLongPlaceholders();
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
}

#include "field_move.h"
// ---- v32: mentor meeting, gym-first Pokédex lines, balloon, decoration news ----
extern const u8 GymCandidate_Text_OakWelcomesMentor[], GymCandidate_Text_MentorDecorGift[], GymCandidate_Text_ReceivedMentorDecor[], Text_NewGymDecor[], Text_OakMissedBalloon[],
    PalletTown_Gym_Text_OakTakeBalloon[], PalletTown_Gym_Text_OakBalloonHowTo[], PalletTown_Gym_Text_RivalTossYourJunk[],
    PalletTown_ProfessorOaksLab_Text_OakHaveRequestForYouTwo[], PalletTown_ProfessorOaksLab_Text_OakPokedexOnDesk[], PalletTown_ProfessorOaksLab_Text_OakCatchMonsForDataTakeThese[],
    PalletTown_ProfessorOaksLab_Text_OakCompleteMonGuideWasMyDream[], PalletTown_ProfessorOaksLab_Text_RivalLeaveItToMeGramps[], PalletTown_ProfessorOaksLab_Text_RivalTellSisNotToGiveYouMap[],
    PokedexRating_Text_HowIsPokedexComingAlong[], PokedexRating_Text_LessThan10[], PokedexRating_Text_LessThan20[], PokedexRating_Text_LessThan30[], PokedexRating_Text_LessThan40[],
    PokedexRating_Text_LessThan50[], PokedexRating_Text_LessThan60[], PokedexRating_Text_LessThan70[], PokedexRating_Text_LessThan80[], PokedexRating_Text_LessThan90[],
    PokedexRating_Text_LessThan100[], PokedexRating_Text_LessThan110[], PokedexRating_Text_LessThan120[], PokedexRating_Text_LessThan130[], PokedexRating_Text_LessThan140[],
    PokedexRating_Text_LoveSeeingYourPokedex[];

TEST("GymHack: v32 story lines fit the message box")
{
    static const u8 *const texts[] = {GymCandidate_Text_OakWelcomesMentor, GymCandidate_Text_MentorDecorGift, GymCandidate_Text_ReceivedMentorDecor, Text_NewGymDecor, Text_OakMissedBalloon,
        PalletTown_Gym_Text_OakTakeBalloon, PalletTown_Gym_Text_OakBalloonHowTo, PalletTown_Gym_Text_RivalTossYourJunk,
        PalletTown_ProfessorOaksLab_Text_OakHaveRequestForYouTwo, PalletTown_ProfessorOaksLab_Text_OakPokedexOnDesk, PalletTown_ProfessorOaksLab_Text_OakCatchMonsForDataTakeThese,
        PalletTown_ProfessorOaksLab_Text_OakCompleteMonGuideWasMyDream, PalletTown_ProfessorOaksLab_Text_RivalLeaveItToMeGramps, PalletTown_ProfessorOaksLab_Text_RivalTellSisNotToGiveYouMap,
        PokedexRating_Text_HowIsPokedexComingAlong, PokedexRating_Text_LessThan10, PokedexRating_Text_LessThan20, PokedexRating_Text_LessThan30, PokedexRating_Text_LessThan40,
        PokedexRating_Text_LessThan50, PokedexRating_Text_LessThan60, PokedexRating_Text_LessThan70, PokedexRating_Text_LessThan80, PokedexRating_Text_LessThan90,
        PokedexRating_Text_LessThan100, PokedexRating_Text_LessThan110, PokedexRating_Text_LessThan120, PokedexRating_Text_LessThan130, PokedexRating_Text_LessThan140,
        PokedexRating_Text_LoveSeeingYourPokedex};
    u32 i, who, f;
    SetLongPlaceholders();
    StringCopy(gStringVar1, COMPOUND_STRING("LT. SURGE"));   // a mentor's name
    StringCopy(gStringVar2, COMPOUND_STRING("ELECTRIC TESLA COIL"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
    for (who = 0; who < GYM_CANDIDATE_LEADERS + GYM_CANDIDATE_ELITE_FOUR; who++)
        for (f = 0; f < 5; f++)
            ExpectLinesFit(GymCandidate_GetMentorPersonText(who, f), 27 * 8 - 4, TRUE);
}

TEST("GymHack: the mentor's housewarming gift is a decoration of the player's type")
{
    u32 t;
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        VarSet(VAR_PLAYER_TYPE, t);
        GymCandidate_GiveMentorDecor();
        EXPECT_EQ(gSpecialVar_Result, TRUE);
    }
}

TEST("GymHack: Fly needs no badge")
{
    FlagClear(FLAG_BADGE03_GET);
    EXPECT(IsFieldMoveUnlocked(FIELD_MOVE_FLY));
}

// ---- The legendary Pokémon of the player's type (src/gym_legend.c) ----
#include "gym_legend.h"
extern const u8 GymLegend_Text_OakWhatOnEarth[], GymLegend_Text_OakCatchIt[], GymLegend_Text_Challenge[], GymLegend_Text_Fled[], GymLegend_Text_LightsOut[], GymLegend_Text_RivalEscaped[], GymLegend_Text_RivalCaught[];

TEST("GymHack: every type's legendary has that primary type")
{
    u32 t;
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        u16 species = GymLegend_GetSpecies(t);
        if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        EXPECT_NE(species, SPECIES_NONE);
        EXPECT_EQ(GetSpeciesType(species, 0), t);
    }
}

TEST("GymHack: the legendary scene's lines fit the message box")
{
    static const u8 *const texts[] = {GymLegend_Text_OakWhatOnEarth, GymLegend_Text_OakCatchIt, GymLegend_Text_Challenge, GymLegend_Text_Fled, GymLegend_Text_LightsOut, GymLegend_Text_RivalEscaped, GymLegend_Text_RivalCaught};
    u32 i, t;
    SetLongPlaceholders();
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        VarSet(VAR_PLAYER_TYPE, t);
        GymLegend_Setup();
        for (i = 0; i < ARRAY_COUNT(texts); i++)
            ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
    }
}

TEST("GymHack: the legendary is a 1% encounter in Cerulean Cave only after the rival's hint")
{
    VarSet(VAR_PLAYER_TYPE, TYPE_DARK);
    FlagClear(FLAG_GYM_LEGEND_CAUGHT);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_CERULEAN_CAVE_1F);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_CERULEAN_CAVE_1F);
    VarSet(VAR_GYM_LEGEND_STATE, GYM_LEGEND_ESCAPED);
    EXPECT(!GymLegend_IsCaveEncounterActive());
    VarSet(VAR_GYM_LEGEND_STATE, GYM_LEGEND_IN_CAVE);
    EXPECT(GymLegend_IsCaveEncounterActive());
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE4);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE4);
    EXPECT(!GymLegend_IsCaveEncounterActive());
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_CERULEAN_CAVE_B1F);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_CERULEAN_CAVE_B1F);
    FlagSet(FLAG_GYM_LEGEND_CAUGHT);
    EXPECT(!GymLegend_IsCaveEncounterActive());
}

// ---- Changing the gym's type (src/gym_switch.c, data/scripts/gym_switch.inc) ----
#include "gym_switch.h"

extern const u8 GymSwitch_Text_AideGoal[], GymSwitch_Text_AnnounceMentor[], GymSwitch_Text_Mastered[], GymSwitch_Text_OakWillCome[], GymSwitch_Text_WhatNext[], GymSwitch_Text_CalledItADay[], GymSwitch_Text_OakMastered[], GymSwitch_Text_OakTryAnother[], GymSwitch_Text_OakKeepItUp[], GymSwitch_Text_OakOfferStands[], GymSwitch_Text_LabGoal[], GymSwitch_Text_LabOffer[], GymSwitch_Text_LabSwitchBack[], GymSwitch_Text_LabNo[], GymSwitch_Text_LabLegendHint[], GymSwitch_Text_WhichType[], GymSwitch_Text_WhichTypeStars[], GymSwitch_Text_ConfirmNew[], GymSwitch_Text_ConfirmOld[], GymSwitch_Text_NowType[], GymSwitch_Text_OldLegendLeaves[], GymSwitch_Text_NewTypeTips[], GymSwitch_Text_BackToType[], GymSwitch_Text_LabMidStreak[], GymSwitch_Text_LabMidStreakMastered[], GymLegend_Text_OakCatchAnother[], GymLegend_Text_AideWhatOnEarth[];

static void ResetGymTypes(u32 type)
{
    u32 t;
    struct GymChallengeSave *save = GymChallenge_GetSave();
    save->switchMagic = 0;
    save->streak = 0;
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        FlagClear(FLAG_GYM_TYPE_OWNED + t);
        FlagClear(FLAG_GYM_LEGEND_CAUGHT_TYPE + t);
        FlagClear(FLAG_GYM_LEGEND_LOOSE_TYPE + t);
        FlagClear(FLAG_GYM_TYPE_MASTERED + t);
    }
    FlagClear(FLAG_GYM_LEGEND_APPEARED);
    FlagClear(FLAG_GYM_LEGEND_CAUGHT);
    FlagClear(FLAG_GYM_RUN_MASTERED);
    VarSet(VAR_GYM_LEGEND_STATE, GYM_LEGEND_ON_STAGE);
    VarSet(VAR_PLAYER_TYPE, type);
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    GymSwitch_GetGoal();   // (sets the save up)
}

static void SwitchTo(u32 type)
{
    gSpecialVar_Result = type;
    GymSwitch_BufferChoice();
    GymSwitch_SetType();
}

TEST("GymHack: switch: the mastery battle is the 20th win, then the 10th")
{
    u32 t;
    ResetGymTypes(TYPE_ROCK);
    EXPECT(GymSwitch_IsMasteryBattle(20));
    EXPECT(!GymSwitch_IsMasteryBattle(10));
    EXPECT(!GymSwitch_IsMasteryBattle(21));
    GymChallenge_GetSave()->switchesEarned = 1;
    EXPECT(GymSwitch_IsMasteryBattle(10));
    EXPECT(!GymSwitch_IsMasteryBattle(20));
    // Not before the gym is the player's, nor once it has had every type and mastered its own
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    EXPECT(!GymSwitch_IsMasteryBattle(10));
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
        FlagSet(FLAG_GYM_TYPE_OWNED + t);
    EXPECT(GymSwitch_IsMasteryBattle(10));      // (the last type still has its mastery battle)
    FlagSet(FLAG_GYM_TYPE_MASTERED + TYPE_ROCK);
    EXPECT(!GymSwitch_IsMasteryBattle(10));
}

TEST("GymHack: switch: every type mastered (Terapagos) takes all 18 gym types' streaks")
{
    u32 t, n = 0;
    ResetGymTypes(TYPE_ROCK);
    EXPECT(!GymSwitch_AllTypesMastered());
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (t == TYPE_NONE || t == TYPE_MYSTERY || t == TYPE_STELLAR)
            continue;
        FlagSet(FLAG_GYM_TYPE_MASTERED + t);
        n++;
        EXPECT(GymSwitch_AllTypesMastered() == (n == 18));
    }
    EXPECT_EQ(n, 18);
}

TEST("GymHack: switch: the mastery battle is the mentor with their convention team, and earns a type")
{
    const struct Trainer *trainer;
    u32 i, total = 0;
    ResetGymTypes(TYPE_WATER);
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_LAPRAS, 60, 0, OTID_STRUCT_PLAYER_ID);
    GymChallenge_GetSave()->streak = 19;
    GymChallenge_NextChallenger();
    EXPECT_EQ(gSpecialVar_Result, GYM_CHALLENGER_BOSS);
    trainer = GymChallenge_GetTrainer(TRAINER_GYM_CHALLENGER);
    EXPECT_EQ(StringCompare(trainer->trainerName, COMPOUND_STRING("MISTY")), 0);
    EXPECT_GE((u32)trainer->partySize, 4);
    for (i = 0; i < trainer->partySize; i++)
        total += trainer->party[i].lvl;
    EXPECT_GE(total / trainer->partySize, 62);
    GymSwitch_AfterWin();
    EXPECT(gSpecialVar_Result);
    EXPECT(FlagGet(FLAG_GYM_RUN_MASTERED));
    EXPECT_EQ(GymChallenge_GetSave()->switchTickets, 1);
    EXPECT_EQ(GymSwitch_GetGoal(), GYM_SWITCH_NEXT_STREAK);
    // An ordinary win earns nothing
    GymChallenge_GetSave()->streak = 3;
    GymChallenge_NextChallenger();
    GymSwitch_AfterWin();
    EXPECT(!gSpecialVar_Result);
    EXPECT_EQ(GymChallenge_GetSave()->switchTickets, 1);
}

TEST("GymHack: switch: a new type uses the mastery up; old ones are free; each legendary is kept")
{
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_CERULEAN_CAVE_1F);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_CERULEAN_CAVE_1F);
    ResetGymTypes(TYPE_DARK);
    GymChallenge_GetSave()->switchTickets = 1;
    FlagSet(FLAG_GYM_LEGEND_APPEARED);   // Darkrai waits on the stage
    FlagSet(FLAG_GYM_RUN_MASTERED);
    EXPECT(!GymLegend_IsCaveEncounterActive());

    SwitchTo(TYPE_FIRE);
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_FIRE);
    EXPECT(gSpecialVar_Result);                                       // Ho-Oh is coming
    EXPECT_EQ(VarGet(VAR_GYM_LEGEND_STATE), GYM_LEGEND_PENDING);
    EXPECT(gSpecialVar_0x8007);                                       // Darkrai left the stage
    EXPECT_EQ(GymChallenge_GetSave()->switchTickets, 0);
    EXPECT(!FlagGet(FLAG_GYM_RUN_MASTERED));
    EXPECT(GymSwitch_IsOwned(TYPE_DARK));
    EXPECT(GymLegend_IsCaveEncounterActive());                        // ...for Cerulean Cave

    // Back to Dark, free: Darkrai is still in the cave; Ho-Oh never came, so it still will
    gSpecialVar_Result = TYPE_DARK;
    GymSwitch_BufferChoice();
    EXPECT(!gSpecialVar_Result);
    GymSwitch_SetType();
    EXPECT(!gSpecialVar_Result);
    EXPECT_EQ(VarGet(VAR_GYM_LEGEND_STATE), GYM_LEGEND_IN_CAVE);
    EXPECT(FlagGet(FLAG_GYM_LEGEND_APPEARED));
    EXPECT(GymLegend_IsCaveEncounterActive());
    SwitchTo(TYPE_FIRE);
    EXPECT_EQ(VarGet(VAR_GYM_LEGEND_STATE), GYM_LEGEND_PENDING);
    EXPECT(GymLegend_IsCaveEncounterActive());

    // A caught one stays caught
    FlagSet(FLAG_GYM_LEGEND_APPEARED);
    FlagSet(FLAG_GYM_LEGEND_CAUGHT);
    VarSet(VAR_GYM_LEGEND_STATE, GYM_LEGEND_DONE);
    SwitchTo(TYPE_DARK);
    SwitchTo(TYPE_FIRE);
    EXPECT(FlagGet(FLAG_GYM_LEGEND_CAUGHT));
    EXPECT_EQ(VarGet(VAR_GYM_LEGEND_STATE), GYM_LEGEND_DONE);
}

TEST("GymHack: switch: all the new lines fit the message box")
{
    static const u8 *const texts[] = {GymSwitch_Text_AideGoal, GymSwitch_Text_AnnounceMentor, GymSwitch_Text_Mastered, GymSwitch_Text_OakWillCome, GymSwitch_Text_WhatNext, GymSwitch_Text_CalledItADay, GymSwitch_Text_OakMastered, GymSwitch_Text_OakTryAnother, GymSwitch_Text_OakKeepItUp, GymSwitch_Text_OakOfferStands, GymSwitch_Text_LabGoal, GymSwitch_Text_LabOffer, GymSwitch_Text_LabSwitchBack, GymSwitch_Text_LabNo, GymSwitch_Text_LabLegendHint, GymSwitch_Text_WhichType, GymSwitch_Text_WhichTypeStars, GymSwitch_Text_ConfirmNew, GymSwitch_Text_ConfirmOld, GymSwitch_Text_NowType, GymSwitch_Text_OldLegendLeaves, GymSwitch_Text_NewTypeTips, GymSwitch_Text_BackToType, GymSwitch_Text_LabMidStreakMastered, GymLegend_Text_OakCatchAnother, GymLegend_Text_AideWhatOnEarth};
    u32 i, type;
    static const u8 *const numbered[] = {GymSwitch_Text_LabMidStreak, GymSwitch_Text_AideGoal, GymSwitch_Text_CalledItADay, GymSwitch_Text_LabGoal};
    SetLongPlaceholders();
    StringCopy(gStringVar1, COMPOUND_STRING("ELECTRIC"));
    StringCopy(gStringVar3, COMPOUND_STRING("ELECTRIC"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
    StringCopy(gStringVar1, COMPOUND_STRING("9999"));
    for (i = 0; i < ARRAY_COUNT(numbered); i++)
        ExpectLinesFit(numbered[i], 27 * 8 - 4, TRUE);
    // Every type and badge, and every legendary
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (GymHack_GetStarterSpeciesForType(type) == SPECIES_NONE)
            continue;
        gSpecialVar_Result = type;
        GymSwitch_BufferChoice();
        ExpectLinesFit(GymSwitch_Text_NowType, 27 * 8 - 4, TRUE);
        ExpectLinesFit(GymSwitch_Text_ConfirmNew, 27 * 8 - 4, TRUE);
        ExpectLinesFit(GymSwitch_Text_NewTypeTips, 27 * 8 - 4, TRUE);
        StringCopy(gStringVar1, GetSpeciesName(GymLegend_GetSpecies(type)));
        StringCopy(gStringVar3, GetSpeciesName(GymLegend_GetSpecies(type)));
        ExpectLinesFit(GymLegend_Text_AideWhatOnEarth, 27 * 8 - 4, TRUE);
        ExpectLinesFit(GymSwitch_Text_OldLegendLeaves, 27 * 8 - 4, TRUE);
    }
    // The mentors' mastery battle lines
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_LAPRAS, 60, 0, OTID_STRUCT_PLAYER_ID);
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (GymHack_GetStarterSpeciesForType(type) == SPECIES_NONE)
            continue;
        ResetGymTypes(type);
        GymChallenge_GetSave()->switchesEarned = 0;
        GymChallenge_GetSave()->streak = GYM_SWITCH_FIRST_STREAK - 1;
        GymChallenge_NextChallenger();
        EXPECT_EQ(gSpecialVar_Result, GYM_CHALLENGER_BOSS);
        ExpectLinesFit(gGymChallengeIntroText, 27 * 8 - 4, FALSE);
        ExpectLinesFit(gGymChallengeAfterText, 27 * 8 - 4, FALSE);
        ExpectLinesFit(gGymChallengeDefeatText, 27 * 8 - 4, FALSE);
        ExpectLinesFit(gGymChallengeWinText, 27 * 8 - 4, FALSE);
        StringCopy(gStringVar1, GymCandidate_GetMentorName(GymCandidate_GetMentorForType(type)->who));
        ExpectLinesFit(GymSwitch_Text_AnnounceMentor, 27 * 8 - 4, TRUE);
    }
}

// ---- The Exotic Pokémon Safari (data/maps/ExoticSafari_Hall) and the Gym Charm ----
extern const u8 ExoticSafari_Hall_Text_Shut[], ExoticSafari_Hall_Text_NotOpenYet[], ExoticSafari_Hall_Text_Pitch[],
    ExoticSafari_Hall_Text_Offer[], ExoticSafari_Hall_Text_Thanks[], ExoticSafari_Hall_Text_TakeALook[],
    ExoticSafari_Hall_Text_GoOnThrough[], ExoticSafari_Hall_Text_NoThanks[], ExoticSafari_Hall_Text_NoMoney[],
    ExoticSafari_Hall_Text_Enjoy[], ExoticSafari_Hall_Text_Kiosk[], Johto_Text_MentorGymCharm[],
    Johto_Text_MentorGymCharmHow[], Johto_Text_GymCharmByMail[], Johto_Text_GymCharmHowByMail[],
    GymSwitch_Text_LabGymCharm[], GymSwitch_Text_LabGymCharmHow[];

TEST("GymHack: Exotic Safari and Gym Charm lines fit the message box")
{
    static const u8 *const texts[] = {ExoticSafari_Hall_Text_Shut, ExoticSafari_Hall_Text_NotOpenYet, ExoticSafari_Hall_Text_Pitch,
        ExoticSafari_Hall_Text_Offer, ExoticSafari_Hall_Text_Thanks, ExoticSafari_Hall_Text_TakeALook,
        ExoticSafari_Hall_Text_GoOnThrough, ExoticSafari_Hall_Text_NoThanks, ExoticSafari_Hall_Text_NoMoney,
        ExoticSafari_Hall_Text_Enjoy, ExoticSafari_Hall_Text_Kiosk, Johto_Text_MentorGymCharm,
        Johto_Text_MentorGymCharmHow, Johto_Text_GymCharmByMail, Johto_Text_GymCharmHowByMail,
        GymSwitch_Text_LabGymCharm, GymSwitch_Text_LabGymCharmHow};
    u32 i;
    SetLongPlaceholders();
    StringCopy(gStringVar1, COMPOUND_STRING("LT. SURGE"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
}

TEST("GymHack: the Gym Charm makes Shiny Pokémon of the gym's type common, and only them")
{
    u32 i, typeShiny = 0, otherShiny = 0;
    struct Pokemon mon;
    VarSet(VAR_PLAYER_TYPE, TYPE_PSYCHIC);
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    AddBagItem(ITEM_GYM_CHARM, 1);
    for (i = 0; i < 2500; i++)
    {
        CreateMon(&mon, SPECIES_ABRA, 10, Random32(), OTID_STRUCT_PLAYER_ID);
        typeShiny += GetMonData(&mon, MON_DATA_IS_SHINY);
        CreateMon(&mon, SPECIES_RATTATA, 10, Random32(), OTID_STRUCT_PLAYER_ID);
        otherShiny += GetMonData(&mon, MON_DATA_IS_SHINY);
    }
    EXPECT_GE(typeShiny, 2);    // (about 10 expected)
    EXPECT_LE(otherShiny, 5);   // (under 1 expected)
    RemoveBagItem(ITEM_GYM_CHARM, 1);
}

// ---- Every Pokémon obtainable (v67) ----

static u16 EvoWithItem(u16 species, u8 level, u32 personality, u16 item)
{
    struct Pokemon mon;
    CreateMon(&mon, species, level, personality, OTID_STRUCT_PLAYER_ID);
    return GetEvolutionTargetSpecies(&mon, item == ITEM_NONE ? EVO_MODE_NORMAL : EVO_MODE_ITEM_USE, item, NULL, NULL, CHECK_EVO);
}

TEST("GymHack: the cross-generation, trade and time-of-day evolutions all work here")
{
    EXPECT_EQ(EvoWithItem(SPECIES_MAGNETON, 40, 0, ITEM_THUNDER_STONE), SPECIES_MAGNEZONE);
    EXPECT_EQ(EvoWithItem(SPECIES_EEVEE, 20, 0, ITEM_LEAF_STONE), SPECIES_LEAFEON);
    EXPECT_EQ(EvoWithItem(SPECIES_EEVEE, 20, 0, ITEM_ICE_STONE), SPECIES_GLACEON);
    EXPECT_EQ(EvoWithItem(SPECIES_YUNGOOS, 20, 0, ITEM_SUN_SHARD), SPECIES_GUMSHOOS);
    EXPECT_EQ(EvoWithItem(SPECIES_RATTATA_ALOLA, 20, 0, ITEM_MOON_SHARD), SPECIES_RATICATE_ALOLA);
    EXPECT_EQ(EvoWithItem(SPECIES_BOLDORE, 36, 0, ITEM_NONE), SPECIES_GIGALITH);
    EXPECT_EQ(EvoWithItem(SPECIES_GURDURR, 36, 0, ITEM_NONE), SPECIES_CONKELDURR);
    EXPECT_EQ(EvoWithItem(SPECIES_GRAVELER_ALOLA, 36, 0, ITEM_NONE), SPECIES_GOLEM_ALOLA);
    EXPECT_EQ(EvoWithItem(SPECIES_KARRABLAST, 36, 0, ITEM_NONE), SPECIES_ESCAVALIER);
    EXPECT_EQ(EvoWithItem(SPECIES_SHELMET, 36, 0, ITEM_NONE), SPECIES_ACCELGOR);
    EXPECT_EQ(EvoWithItem(SPECIES_PUMPKABOO_SMALL, 36, 0, ITEM_NONE), SPECIES_GOURGEIST_SMALL);
    EXPECT_EQ(EvoWithItem(SPECIES_PHANTUMP, 36, 0, ITEM_NONE), SPECIES_TREVENANT);
    EXPECT_EQ(EvoWithItem(SPECIES_BOLDORE, 35, 0, ITEM_LINKING_CORD), SPECIES_GIGALITH);
}

TEST("GymHack: Alcremie's cream comes from the Milcery's personality")
{
    EXPECT_EQ(EvoWithItem(SPECIES_MILCERY, 30, 0, ITEM_BERRY_SWEET), SPECIES_ALCREMIE_BERRY_VANILLA_CREAM);
    EXPECT_EQ(EvoWithItem(SPECIES_MILCERY, 30, 3 << 8, ITEM_BERRY_SWEET), SPECIES_ALCREMIE_BERRY_MINT_CREAM);
    EXPECT_EQ(EvoWithItem(SPECIES_MILCERY, 30, 8 << 8, ITEM_STRAWBERRY_SWEET), SPECIES_ALCREMIE_STRAWBERRY_RAINBOW_SWIRL);
    EXPECT_EQ(EvoWithItem(SPECIES_MILCERY, 30, 1 << 8, ITEM_STRAWBERRY_SWEET), SPECIES_ALCREMIE_STRAWBERRY_RUBY_CREAM);
}

TEST("GymHack: shared wild slots and random forms")
{
    u32 i, patrat = 0, purrloin = 0, other = 0, bugForms = 0;
    u8 level = 50;
    bool8 seen[20] = {0};
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_EXOTIC_SAFARI_UNOVA_SKYARROW);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_EXOTIC_SAFARI_UNOVA_SKYARROW);
    for (i = 0; i < 400; i++)
    {
        u16 sp = ExoticSafari_WildSpecies(SPECIES_PATRAT, &level);
        if (sp == SPECIES_PATRAT) patrat++;
        else if (sp == SPECIES_PURRLOIN) purrloin++;
        else other++;
        sp = ExoticSafari_WildSpecies(SPECIES_SCATTERBUG_POLAR, &level);
        if (sp == SPECIES_SCATTERBUG_ICY_SNOW) seen[0] = TRUE;
        else if (sp >= SPECIES_SCATTERBUG_POLAR && sp <= SPECIES_SCATTERBUG_JUNGLE) seen[1 + sp - SPECIES_SCATTERBUG_POLAR] = TRUE;
    }
    for (i = 0; i < 20; i++)
        bugForms += seen[i];
    EXPECT_GT(patrat, 100);
    EXPECT_GT(purrloin, 100);
    EXPECT_EQ(other, 0);
    EXPECT_EQ(bugForms, 18);
    EXPECT_EQ(level, 50);
}

TEST("GymHack: wild Zygarde in Kalos once one is caught, its forms gated by the Cells")
{
    u32 i, n10 = 0, n50 = 0, pc = 0;
    u8 level;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_EXOTIC_SAFARI_KALOS_CITY);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_EXOTIC_SAFARI_KALOS_CITY);
    for (i = 0; i < 1000; i++)
    {
        level = 50;
        EXPECT_NE(ExoticSafari_WildSpecies(SPECIES_ESPURR, &level), SPECIES_ZYGARDE_10);
    }
    FlagSet(FLAG_EXOTIC_LEGEND_ZYGARDE);
    for (i = 0; i < 3000; i++)
    {
        u16 sp;
        level = 50;
        sp = ExoticSafari_WildSpecies(SPECIES_ESPURR, &level);
        if (sp == SPECIES_ZYGARDE_10) { n10++; EXPECT_EQ(level, 70); }
        if (sp == SPECIES_ZYGARDE_50) n50++;
        if (sp == SPECIES_ZYGARDE_50_POWER_CONSTRUCT || sp == SPECIES_ZYGARDE_10_POWER_CONSTRUCT) pc++;
    }
    EXPECT_GT(n10, 20);
    EXPECT_EQ(n50, 0);
    EXPECT_EQ(pc, 0);
    for (i = 0; i < 8; i++)
        FlagSet(FLAG_EXOTIC_CELL_FIRST + i);
    for (i = 0; i < 3000; i++)
    {
        u16 sp;
        level = 50;
        sp = ExoticSafari_WildSpecies(SPECIES_ESPURR, &level);
        if (sp == SPECIES_ZYGARDE_50) n50++;
        if (sp == SPECIES_ZYGARDE_50_POWER_CONSTRUCT || sp == SPECIES_ZYGARDE_10_POWER_CONSTRUCT) pc++;
    }
    EXPECT_GT(n50, 10);
    EXPECT_EQ(pc, 0);
    for (i = 0; i < 4; i++)
        FlagSet(FLAG_EXOTIC_CELL_MORE_FIRST + i);
    for (i = 0; i < 3000; i++)
    {
        u16 sp;
        level = 50;
        sp = ExoticSafari_WildSpecies(SPECIES_ESPURR, &level);
        if (sp == SPECIES_ZYGARDE_50_POWER_CONSTRUCT || sp == SPECIES_ZYGARDE_10_POWER_CONSTRUCT) pc++;
    }
    EXPECT_GT(pc, 10);
    FlagClear(FLAG_EXOTIC_LEGEND_ZYGARDE);
}

TEST("GymHack: CLEFABILL is Fairy/Ghost, 800 in all, and not in the Pokedex")
{
    const struct SpeciesInfo *s = &gSpeciesInfo[SPECIES_CLEFABILL];
    u32 type1 = s->types[0], type2 = s->types[1], natDex = s->natDexNum;
    u32 total = s->baseHP + s->baseAttack + s->baseDefense + s->baseSpeed + s->baseSpAttack + s->baseSpDefense;
    EXPECT_EQ(type1, TYPE_FAIRY);
    EXPECT_EQ(type2, TYPE_GHOST);
    EXPECT_EQ(total, 800);
    EXPECT_EQ(natDex, NATIONAL_DEX_NONE);
}

static void GiveTourneyTeam(void)
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARIZARD, 100;
        givemon SPECIES_BLASTOISE, 100;
        givemon SPECIES_VENUSAUR, 100;
    );
}

TEST("GymHack: a tournament draws 127 different trainers and the player, seeded by tier")
{
    u8 (*tree)[TOURNEY_SIZE] = AllocZeroed((TOURNEY_ROUNDS + 1) * TOURNEY_SIZE);
    u32 run, i, j, players;
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    GiveTourneyTeam();
    EXPECT_EQ(Tourney_PoolSize(), 83 + 127);
    for (run = 0; run < 5; run++)
    {
        struct TourneyEntrant top;
        Tourney_Begin();
        Tourney_BuildTree(tree);
        players = 0;
        for (i = 0; i < TOURNEY_SIZE; i++)
        {
            struct TourneyEntrant a, b;
            players += tree[0][i] == TOURNEY_PLAYER;
            EXPECT(tree[0][i] != TOURNEY_PENDING);
            if (tree[0][i] == TOURNEY_PLAYER)
                continue;
            Tourney_GetEntrant(tree[0][i], &a);
            for (j = i + 1; j < TOURNEY_SIZE; j++)
            {
                if (tree[0][j] == TOURNEY_PLAYER)
                    continue;
                Tourney_GetEntrant(tree[0][j], &b);
                EXPECT_NE(StringCompare(a.name, b.name), 0);
            }
        }
        EXPECT_EQ(players, 1);
        // (seed 1 is in the first slot, seed 2 at the top of the bottom half: both from the top tier)
        Tourney_GetEntrant(tree[0][0], &top);
        EXPECT_EQ(top.tier, 0);
        Tourney_GetEntrant(tree[0][TOURNEY_SIZE / 2], &top);
        EXPECT_EQ(top.tier, 0);
        // (only the first round's undecided, until the player plays theirs)
        EXPECT_EQ(tree[1][0], TOURNEY_PENDING);
    }
    Free(tree);
}

TEST("GymHack: once the player's out, the tournament plays out to a champion")
{
    u8 (*tree)[TOURNEY_SIZE] = AllocZeroed((TOURNEY_ROUNDS + 1) * TOURNEY_SIZE);
    u32 r, m, slot;
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    GiveTourneyTeam();
    Tourney_Begin();
    gTourneySave.run.wins = 2;
    Tourney_RecordLoss();
    EXPECT_EQ(gTourneySave.run.status, TOURNEY_LOST);
    Tourney_BuildTree(tree);
    slot = Tourney_PlayerSlot((const u8 (*)[TOURNEY_SIZE])tree);
    EXPECT_EQ(tree[1][slot >> 1], TOURNEY_PLAYER);
    EXPECT_EQ(tree[2][slot >> 2], TOURNEY_PLAYER);
    EXPECT_NE(tree[3][slot >> 3], TOURNEY_PLAYER);
    for (r = 0; r < TOURNEY_ROUNDS; r++)
    {
        for (m = 0; m < (TOURNEY_SIZE >> (r + 1)); m++)
        {
            u32 w = tree[r + 1][m];
            EXPECT(w == tree[r][2 * m] || w == tree[r][2 * m + 1]);
        }
    }
    EXPECT_NE(tree[TOURNEY_ROUNDS][0], TOURNEY_PENDING);
    EXPECT_NE(tree[TOURNEY_ROUNDS][0], TOURNEY_PLAYER);
    Free(tree);
}

TEST("GymHack: the tournament's opponents are at level 100, and the team is locked in")
{
    const struct Trainer *t;
    u32 i;
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    GiveTourneyTeam();
    Tourney_Begin();
    for (i = 0; i < TOURNEY_ROUNDS; i++)
    {
        Tourney_SetupNext();
        t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        EXPECT_GE((u32)t->partySize, 3);
        EXPECT_EQ(t->party[0].lvl, MAX_LEVEL);
        EXPECT_EQ(t->party[t->partySize - 1].lvl, MAX_LEVEL);
        gTourneySave.run.wins++;
    }
    gTourneySave.run.wins = 0;
    Tourney_CheckTeam();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU, 100;);
    Tourney_CheckTeam();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    GiveTourneyTeam();      // (the same species, but not the same Pokémon)
    Tourney_CheckTeam();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
}

TEST("GymHack: winning the tournament: the Hall of Fame, then Bill; in the simulator, a WITHDRAW")
{
    u32 i, n;
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    VarSet(VAR_GAUNTLET_WITHDRAWS, 0);
    GiveTourneyTeam();
    Tourney_Begin();
    for (i = 0; i < TOURNEY_ROUNDS; i++)
    {
        Tourney_SetupNext();
        Tourney_RecordWin();
        EXPECT_EQ(gSpecialVar_Result, i == TOURNEY_ROUNDS - 1);
    }
    EXPECT_EQ(gTourneySave.run.status, TOURNEY_WON);
    EXPECT_EQ(gTourneySave.titles, 1);
    EXPECT_EQ(VarGet(VAR_GAUNTLET_STATE), GAUNTLET_ALL_BEATEN);
    EXPECT_EQ(Tourney_HallCount(), 1);
    EXPECT_EQ(gTourneySave.hall[0].count, 3);
    EXPECT(gTourneySave.hall[0].mon[0].species == SPECIES_CHARIZARD);
    Tourney_BufferChampion();
    EXPECT_EQ(StringCompare(gStringVar1, gSaveBlock2Ptr->playerName), 0);

    // (the simulator: a WITHDRAW for each title; the Hall keeps the first and the latest)
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR);
    for (n = 2; n <= 6; n++)
    {
        Tourney_Begin();
        EXPECT_EQ(gTourneySave.run.sim, TRUE);
        for (i = 0; i < TOURNEY_ROUNDS; i++)
        {
            Tourney_SetupNext();
            Tourney_RecordWin();
        }
        EXPECT_EQ(VarGet(VAR_GAUNTLET_WITHDRAWS), n - 1);
    }
    EXPECT_EQ(VarGet(VAR_GAUNTLET_STATE), GAUNTLET_SIMULATOR);
    EXPECT_EQ(gTourneySave.titles, 6);
    EXPECT_EQ(Tourney_HallCount(), TOURNEY_HALL_RECORDS);
    EXPECT_EQ(gTourneySave.hall[0].title, 1);
    EXPECT_EQ(gTourneySave.hall[TOURNEY_HALL_RECORDS - 1].title, 6);
    EXPECT_EQ(gTourneySave.hall[1].title, 4);
}

TEST("GymHack: the tournament's save is checked, and a forfeit ends the run")
{
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    GiveTourneyTeam();
    Tourney_Begin();
    gTourneySave.titles = 3;
    Tourney_SealSave();
    Tourney_ValidateSave();
    EXPECT_EQ(gTourneySave.titles, 3);
    EXPECT_EQ(gTourneySave.run.status, TOURNEY_ACTIVE);
    Tourney_Forfeit();
    Tourney_GetStatus();
    EXPECT_EQ(gSpecialVar_Result, TOURNEY_FORFEIT);
    gTourneySave.titles = 4;    // (changed after sealing: as if the data were damaged)
    Tourney_ValidateSave();
    EXPECT_EQ(gTourneySave.titles, 0);
    EXPECT_EQ(gTourneySave.run.status, TOURNEY_NONE);
}

TEST("GymHack: Bill's battle music is for Bill alone")
{
    Tourney_ClearSave();
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    GiveTourneyTeam();
    FlagSet(FLAG_GYM_CHALLENGE_BATTLE);
    Gauntlet_SetupBill();
    EXPECT_EQ(Gauntlet_IsBillBattle(), TRUE);
    Tourney_Begin();
    Tourney_SetupNext();
    EXPECT_EQ(Gauntlet_IsBillBattle(), FALSE);
    FlagClear(FLAG_GYM_CHALLENGE_BATTLE);
}

TEST("GymHack: the Bond Phenomenon and AZ wait for a Kalos win in the tournament")
{
    FlagClear(FLAG_TOURNEY_BEAT_KALOS);
    FlagClear(FLAG_TOURNEY_BEAT_AZ);
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE);
    EXPECT_EQ(Tourney_KalosBeaten(), FALSE);
    EXPECT_EQ(Tourney_AzBeaten(), FALSE);
    FlagSet(FLAG_TOURNEY_BEAT_KALOS);
    EXPECT_EQ(Tourney_KalosBeaten(), TRUE);
    EXPECT_EQ(Tourney_AzBeaten(), FALSE);
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN);
    EXPECT_EQ(Tourney_AzBeaten(), TRUE);
    FlagClear(FLAG_TOURNEY_BEAT_KALOS);
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_NONE);
}

TEST("GymHack: beating the real Bill leaves his Palm Pilot and one WITHDRAW")
{
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN);
    VarSet(VAR_GAUNTLET_WITHDRAWS, 0);
    FlagSet(FLAG_GAUNTLET_BILL_REVEALED);
    Gauntlet_BillDefeated();
    EXPECT_EQ(VarGet(VAR_GAUNTLET_STATE), GAUNTLET_SIMULATOR);
    EXPECT_EQ(VarGet(VAR_GAUNTLET_WITHDRAWS), 1);
    EXPECT_EQ(VarGet(VAR_OBJ_GFX_ID_D), OBJ_EVENT_GFX_GAUNTLET_PALM_PILOT);
}

TEST("GymHack: the Type Mastery Hall of Fame keeps a type's longest streak and its team")
{
    MasteryHall_Clear();
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 30, 0, OTID_STRUCT_PRESET(0));
    CalculatePlayerPartyCount();
    MasteryHall_RecordWin(TYPE_FIRE, 20);
    EXPECT_EQ(gMasteryHall.records[TYPE_FIRE].streak, 20);
    EXPECT_EQ(gMasteryHall.records[TYPE_FIRE].count, 1);
    EXPECT_EQ((u32)gMasteryHall.records[TYPE_FIRE].mon[0].species, SPECIES_CHARMANDER);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_VULPIX, 30, 0, OTID_STRUCT_PRESET(0));
    MasteryHall_RecordWin(TYPE_FIRE, 15);     // (a shorter streak: the record stays)
    EXPECT_EQ((u32)gMasteryHall.records[TYPE_FIRE].mon[0].species, SPECIES_CHARMANDER);
    MasteryHall_RecordWin(TYPE_FIRE, 21);     // (longer: the team now replaces it)
    EXPECT_EQ(gMasteryHall.records[TYPE_FIRE].streak, 21);
    EXPECT_EQ((u32)gMasteryHall.records[TYPE_FIRE].mon[0].species, SPECIES_VULPIX);
    EXPECT_EQ(MasteryHall_Count(), 1);
}

TEST("GymHack: BILL'S ARCHIVE keeps one of each, and only what has a slot")
{
    Archive_Clear();
    EXPECT(Archive_IsSlot(SPECIES_PIKACHU));
    EXPECT(Archive_IsSlot(SPECIES_VULPIX_ALOLA));
    EXPECT(!Archive_IsSlot(SPECIES_CLEFABILL));
    EXPECT(!Archive_IsSlot(SPECIES_CHARIZARD_MEGA_X));
    EXPECT(!Archive_Has(SPECIES_MEWTWO));
    Archive_Store(SPECIES_MEWTWO, TRUE);
    EXPECT(Archive_Has(SPECIES_MEWTWO));
    EXPECT(!Archive_Has(SPECIES_MEW));
}

TEST("GymHack: the Leader Academy's rentals put the player's party back")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_PIKACHU, 12, 0, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    Academy_RentalBegin();
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 0);
    for (u32 i = 0; i < 3; i++)
    {
        gSpecialVar_0x8004 = i;
        Academy_TakeRental();
    }
    EXPECT_EQ(gSpecialVar_Result, 3);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 3);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_LEVEL), 100);
    EXPECT_NE(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU);
    Academy_RentalEnd();
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 12);
}

TEST("GymHack: the Leader Academy's candidates and Battle Points")
{
    const struct Trainer *t;
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_PIKACHU, 30, 0, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gSpecialVar_0x8004 = 6;     // (Briony: expert)
    Academy_SetupCandidate();
    EXPECT_EQ(gSpecialVar_0x8005, 4);
    t = GymChallenge_GetTrainer(TRAINER_GYM_CHALLENGER);
    u32 size = t->partySize;
    EXPECT_GE(size, 3);
    EXPECT_EQ(t->party[0].lvl, 33);
    EXPECT_EQ(t->trainerClass, TRAINER_CLASS_CANDIDATE);

    gSaveBlock2Ptr->frontier.battlePoints = 10;
    gSpecialVar_0x8006 = 12;
    Academy_SpendBP();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    EXPECT_EQ(gSaveBlock2Ptr->frontier.battlePoints, 10);
    gSpecialVar_0x8006 = 6;
    Academy_SpendBP();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT_EQ(gSaveBlock2Ptr->frontier.battlePoints, 4);
    gSpecialVar_0x8004 = 3;
    Academy_AddBP();
    EXPECT_EQ(gSaveBlock2Ptr->frontier.battlePoints, 7);
}

TEST("GymHack: the TEAM BUILDER's saved teams start empty")
{
    gExtraSave.teams.magic = 0;
    gExtraSave.teams.teams[3].species[2] = SPECIES_PIKACHU;
    TeamBuilder_Validate();
    EXPECT_EQ(gExtraSave.teams.teams[3].species[2], SPECIES_NONE);
    gExtraSave.teams.teams[3].species[2] = SPECIES_PIKACHU;
    TeamBuilder_Validate();
    EXPECT_EQ(gExtraSave.teams.teams[3].species[2], SPECIES_PIKACHU);
}

// Primrose's Mega trials: for every stone, three Pokémon at 55-60, her Mega last holding its stone,
// none of them with a type or move that hits the player's Pokémon (or its Mega) super effectively
TEST("GymHack: Mega trial teams are fair to the Pokemon taking the trial")
{
    u32 species, checked = 0;
    for (species = 1; species < NUM_SPECIES; species++)
    {
        const struct FormChange *fc;
        if (!IsSpeciesEnabled(species))
            continue;
        fc = GetSpeciesFormChanges(species);
        if (gSpeciesInfo[species].isMegaEvolution || fc == NULL)
            continue;
        for (; fc->method != FORM_CHANGE_TERMINATOR; fc++)
        {
            const struct TrainerMon *team;
            u32 k, m, mega = fc->targetSpecies, stone = fc->param1;
            if (fc->method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM && fc->method != FORM_CHANGE_BATTLE_PRIMAL_REVERSION)
                continue;
            team = Academy_MegaTrialTeam(species, stone);
            checked++;
            EXPECT_EQ(team[2].lvl, 60);
            EXPECT_NE(team[2].heldItem, ITEM_NONE);
            for (k = 0; k < 3; k++)
            {
                u32 lvl = team[k].lvl;
                EXPECT_GE(lvl, 55);
                EXPECT_LE(lvl, 60);
                EXPECT_NE(team[k].species, species);
                for (m = 0; m < MAX_MON_MOVES; m++)
                {
                    u32 move = team[k].moves[m], type;
                    uq4_12_t a, b;
                    if (move == MOVE_NONE || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS)
                        continue;
                    type = GetMoveType(move);
                    a = GetTypeModifier(type, GetSpeciesType(mega, 0));
                    if (GetSpeciesType(mega, 1) != GetSpeciesType(mega, 0))
                        a = uq4_12_multiply(a, GetTypeModifier(type, GetSpeciesType(mega, 1)));
                    b = GetTypeModifier(type, GetSpeciesType(species, 0));
                    if (GetSpeciesType(species, 1) != GetSpeciesType(species, 0))
                        b = uq4_12_multiply(b, GetTypeModifier(type, GetSpeciesType(species, 1)));
                    EXPECT_LE(a, UQ_4_12(1.0));
                    EXPECT_LE(b, UQ_4_12(1.0));
                }
            }
            EXPECT(Academy_MegaTrialFair(team[0].species, species, mega));
            EXPECT(Academy_MegaTrialFair(team[1].species, species, mega));
        }
    }
    EXPECT_GE(checked, 90);
}

TEST("GymHack: Rayquaza Mega Evolves with the Rayquazite, not Dragon Ascent")
{
    const struct FormChange *fc = GetSpeciesFormChanges(SPECIES_RAYQUAZA);
    bool32 stone = FALSE;
    for (; fc != NULL && fc->method != FORM_CHANGE_TERMINATOR; fc++)
    {
        EXPECT(fc->method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_MOVE);
        if (fc->method == FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM && fc->param1 == ITEM_RAYQUAZITE)
            stone = TRUE;
    }
    EXPECT(stone);
    EXPECT_EQ(GetItemHoldEffect(ITEM_RAYQUAZITE), HOLD_EFFECT_MEGA_STONE);
    EXPECT_EQ(Academy_MegaTrialTeam(SPECIES_KYOGRE, ITEM_BLUE_ORB)[2].lvl, 60);
    EXPECT_EQ(Academy_MegaTrialTeam(SPECIES_GROUDON, ITEM_RED_ORB)[2].lvl, 60);
}

static u32 FormIndex(u32 species)
{
    u32 i;
    for (i = 0; i < DEX_FORM_COUNT; i++)
        if (gDexFormSpecies[i] == species)
            return i;
    return 0;
}

TEST("GymHack: the team builder only offers Pokemon the player can get by now")
{
    FlagClear(FLAG_SYS_GAME_CLEAR);
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_GYM_TYPE_OWNED + TYPE_DRAGON);
    FlagClear(FLAG_GYM_LEGEND_LOOSE_TYPE + TYPE_DRAGON);
    FlagClear(FLAG_GYM_LEGEND_CAUGHT_TYPE + TYPE_DRAGON);
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_NONE);
    VarSet(VAR_PLAYER_TYPE, TYPE_FIRE);
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_CHARMANDER)));
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_DRAGONITE)));
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_SPRIGATITO)));     // (the Exotic Safari)
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_RAYQUAZA)));       // (the Dragon gym's legendary)
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_ARCEUS_NORMAL)));
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_PIKACHU_COSPLAY))); // (the costume maker: post-game)
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_PIKACHU_ORIGINAL)));  // (the caps: main game)
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_PIKACHU_STARTER)));
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    FlagSet(FLAG_SYS_GAME_CLEAR);
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_SPRIGATITO)));
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_RAYQUAZA)));
    VarSet(VAR_PLAYER_TYPE, TYPE_DRAGON);
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_RAYQUAZA)));
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR);
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_PIKACHU_COSPLAY)));
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_NONE);
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_SYS_GAME_CLEAR);
}

TEST("GymHack: every species and form can be had somewhere")
{
    u32 i;
    for (i = 0; i < DEX_FORM_COUNT; i++)
        EXPECT_NE(gDexFormAvail[i] & AVAIL_TIER_MASK, AVAIL_NEVER);
}

TEST("GymHack: the Let's Go partners wait for their type to be mastered")
{
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    FlagClear(FLAG_GYM_TYPE_MASTERED + TYPE_ELECTRIC);
    FlagClear(FLAG_GYM_TYPE_MASTERED + TYPE_NORMAL);
    FlagClear(FLAG_EVENT_PARTNER_PIKACHU);
    FlagClear(FLAG_EVENT_PARTNER_EEVEE);
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_NONE);
    EventMons_PartnerReady();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT(!TeamBuilder_IsAvailable(FormIndex(SPECIES_EEVEE_STARTER)));
    FlagSet(FLAG_GYM_TYPE_MASTERED + TYPE_NORMAL);
    EventMons_PartnerReady();
    EXPECT_EQ(gSpecialVar_Result, 2);
    EXPECT(TeamBuilder_IsAvailable(FormIndex(SPECIES_EEVEE_STARTER)));
    FlagSet(FLAG_GYM_TYPE_MASTERED + TYPE_ELECTRIC);
    EventMons_PartnerReady();
    EXPECT_EQ(gSpecialVar_Result, 1);
    FlagClear(FLAG_GYM_TYPE_MASTERED + TYPE_ELECTRIC);
    FlagClear(FLAG_GYM_TYPE_MASTERED + TYPE_NORMAL);
    FlagClear(FLAG_PALLET_GYM_AWARDED);
}

TEST("GymHack: Cosplay Pikachu changes costume and brings the costume's move")
{
    u32 i;
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU_COSPLAY, 50;);
    for (i = 0; i < ACADEMY_LECTURE_COUNT_FOR_TEST; i++)
        FlagSet(FLAG_ACADEMY_LECTURE_FIRST + i);
    gSpecialVar_0x8005 = 4;     // (Ph.D.)
    EventMons_CostumeUnlocked();
    EXPECT(gSpecialVar_Result);
    gSpecialVar_0x8004 = 0;
    EventMons_SetCostume();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU_PHD);
    EXPECT(MonKnowsMove(&gParties[B_TRAINER_PLAYER][0], MOVE_ELECTRIC_TERRAIN));
    gSpecialVar_0x8005 = 0;     // (back to no costume: Electric Terrain goes, Thunder Shock comes)
    EventMons_SetCostume();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU_COSPLAY);
    EXPECT(!MonKnowsMove(&gParties[B_TRAINER_PLAYER][0], MOVE_ELECTRIC_TERRAIN));
    for (i = 0; i < ACADEMY_LECTURE_COUNT_FOR_TEST; i++)
        FlagClear(FLAG_ACADEMY_LECTURE_FIRST + i);
}

TEST("GymHack: the caps need the Cap Case, and each is traded once")
{
    VarSet(VAR_EVENT_CAPS, 0);
    RemoveBagItem(ITEM_CAP_CASE, 1);
    gSpecialVar_0x8004 = 3;
    EventMons_CapFound();
    EXPECT_EQ(gSpecialVar_Result, 0);
    AddBagItem(ITEM_CAP_CASE, 1);
    EventMons_CapFound();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EventMons_CapFound();
    EXPECT_EQ(gSpecialVar_Result, 2);
    EventMons_CapReward();
    EXPECT_EQ(gSpecialVar_Result, SPECIES_PIKACHU_UNOVA);
    EventMons_CapRewardGiven();
    EventMons_CapReward();
    EXPECT_EQ(gSpecialVar_Result, SPECIES_NONE);
    VarSet(VAR_EVENT_CAPS, 0x7F00);     // (all seven traded: the Partner Cap)
    EventMons_CapReward();
    EXPECT_EQ(gSpecialVar_Result, SPECIES_PIKACHU_PARTNER);
    VarSet(VAR_EVENT_CAPS, 0);
    RemoveBagItem(ITEM_CAP_CASE, 1);
}

TEST("GymHack: a Battle Bond Froakie's line ends in Greninja's Battle Bond form")
{
    struct Pokemon mon;
    bool32 bond = TRUE;
    CreateRandomMon(&mon, SPECIES_FROGADIER, 40);
    EXPECT_EQ(EventMons_EvolutionTarget(&mon, SPECIES_GRENINJA), SPECIES_GRENINJA);
    SetMonData(&mon, MON_DATA_BATTLE_BOND, &bond);
    EXPECT_EQ(EventMons_EvolutionTarget(&mon, SPECIES_GRENINJA), SPECIES_GRENINJA_BATTLE_BOND);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_GRENINJA_BATTLE_BOND);
}

TEST("GymHack: Eternamax is the pitch's alone")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_ETERNATUS_ETERNAMAX, 70;);
    EventMons_OnMapLoad();      // (the tests' map isn't the pitch)
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_ETERNATUS);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MAX_HP));
}
