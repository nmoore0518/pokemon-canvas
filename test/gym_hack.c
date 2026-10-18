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
    GymHack_Text_TypeFairy, GymHack_Text_TypeBug, GymHack_Text_TypeDark,
    GymHack_Text_TypeDragon, GymHack_Text_TypeElectric, GymHack_Text_TypeFighting,
    GymHack_Text_TypeFire, GymHack_Text_TypeFlying, GymHack_Text_TypeGhost, GymHack_Text_TypeGrass,
    GymHack_Text_TypeGround, GymHack_Text_TypeIce, GymHack_Text_TypeNormal,
    GymHack_Text_TypePoison, GymHack_Text_TypePsychic, GymHack_Text_TypeRock,
    GymHack_Text_TypeSteel, GymHack_Text_TypeWater,
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
    GymHackTest_ChooseStarter(0); // Clefairy: Fairy in modern typing
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_FAIRY);

    GymHackTest_ChooseStarter(14); // Ralts: Psychic/Fairy, primary Psychic
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
    GymHackTest_ChooseStarter(6); // Charmander (Fire)
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
        if (i != GYM_DECOR_BATTLE_STAGE)
            EXPECT_GT(gGymDecor[i].price, 0);
        EXPECT_LE(gGymDecor[i].width, 4);
        EXPECT_LE(gGymDecor[i].height, 5);
        EXPECT_LE(GetStringWidth(FONT_NORMAL, gGymDecor[i].name, 0), 13 * 8); // the list sits at x 15
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
    EXPECT_EQ(GetItemPrice(ITEM_CILANTRO), 15000);
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
