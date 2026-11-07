// Tests for the Gym Trainer hack's custom logic (src/gym_hack.c).
// Run with: make check GAME_VERSION=FIRERED TESTS="GymHack"

#include "global.h"
#include "test/test.h"
#include "battle_main.h"
#include "event_data.h"
#include "gym_hack.h"
#include "tower_graves.h"
#include "dad_story.h"
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
#include "item_upgrades.h"
#include "load_save.h"
#include "incense.h"
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
#include "painter.h"
#include "constants/painter.h"
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

TEST("GymHack: MissingNo. (Bird/Normal, typeless first) is on no gym's team")
{
    u32 type;
    EXPECT_EQ(gSpeciesInfo[SPECIES_MISSINGNO].types[0], TYPE_MYSTERY);
    EXPECT_EQ(gSpeciesInfo[SPECIES_MISSINGNO].types[1], TYPE_NORMAL);
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (type == TYPE_MYSTERY || type == TYPE_STELLAR)
            continue;
        SetPlayerType(type);
        ZeroPlayerPartyMons();
        RUN_OVERWORLD_SCRIPT(givemon SPECIES_MISSINGNO, 30;);
        EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_WRONG_TYPE);
    }
    EXPECT_EQ(StringCompare(gStringVar2, COMPOUND_STRING("Bird")), 0);
}

TEST("GymHack: the Pokemon Tower has ten unevolved graves and DAD's ace")
{
    u32 i, unevolved = 0;
    VarSet(VAR_PLAYER_TYPE, TYPE_FIRE);
    EXPECT_EQ(TowerGraves_Count(), 30);
    for (i = 0; i < TowerGraves_Count(); i++)
        unevolved += GymHack_SpeciesCanEvolve(TowerGraves_GetSpecies(i));
    EXPECT_EQ(unevolved, TOWER_GRAVES_UNEVOLVED);
    for (i = TYPE_NORMAL; i < NUMBER_OF_MON_TYPES; i++)
        EXPECT(!GymHack_SpeciesCanEvolve(DadStory_GetAce(i)));
}

TEST("GymHack: the tenth unevolved grave brings the ghost girl, once")
{
    u32 i, calls = 0;
    VarSet(VAR_TOWER_GRAVES, 0);
    FlagClear(FLAG_GOT_EVIOLATE);
    for (i = 0; i < TowerGraves_Count(); i++)
    {
        gSpecialVar_0x8004 = i;
        TowerGraves_Read();
        if (gSpecialVar_Result && calls++ == 0)
            EXPECT_EQ(i, 28);   // (FERN's ODDISH: the last of the ten, in order)
    }
    EXPECT_EQ(calls, 2);        // (28, then 29: until the Eviolate is taken, every grave calls her)
    FlagSet(FLAG_GOT_EVIOLATE);
    gSpecialVar_0x8004 = 0;
    TowerGraves_Read();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    FlagClear(FLAG_GOT_EVIOLATE);
    VarSet(VAR_TOWER_GRAVES, 0);
}

TEST("GymHack: two Pokemon holding the same item break the rules")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 10, item=ITEM_EVIOLITE;
        givemon SPECIES_VULPIX, 10, item=ITEM_LEFTOVERS;
        givemon SPECIES_GROWLITHE, 10;
        givemon SPECIES_PONYTA, 10;
    );
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_OK);   // (different items, and no item, are fine)
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_MAGBY, 10, item=ITEM_EVIOLITE;);
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_DUPLICATE_ITEM);
    EXPECT_EQ(StringCompare(gStringVar1, GetItemName(ITEM_EVIOLITE)), 0);
    EXPECT_EQ(GymHack_CheckTeamRules(TEAM_RULE_ITEM), GYM_HACK_TEAM_DUPLICATE_ITEM);
}

TEST("GymHack: the tournament's clauses: any type, but no two alike and no two items alike")
{
    SetPlayerType(TYPE_FIRE);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_SQUIRTLE, 10; givemon SPECIES_BULBASAUR, 10, item=ITEM_EVIOLITE;);
    EXPECT_EQ(GymHack_CheckTeamRules(TEAM_RULES_OPEN), GYM_HACK_TEAM_OK);
    EXPECT_EQ(GymHack_CheckTeam(), GYM_HACK_TEAM_WRONG_TYPE);
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU, 10, item=ITEM_EVIOLITE;);
    EXPECT_EQ(GymHack_CheckTeamRules(TEAM_RULES_OPEN), GYM_HACK_TEAM_DUPLICATE_ITEM);
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_SQUIRTLE, 10; givemon SPECIES_WARTORTLE, 20; givemon SPECIES_SQUIRTLE, 12;);
    EXPECT_EQ(GymHack_CheckTeamRules(TEAM_RULES_OPEN), GYM_HACK_TEAM_DUPLICATE);
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
    EXPECT_EQ(GetItemPocket(ITEM_HEDGE_CLIPPERS), POCKET_TOOLS);
    EXPECT_EQ(GetItemPocket(ITEM_CAN_OF_SPINACH), POCKET_TOOLS);
    EXPECT_EQ(GetItemPocket(ITEM_ROCKET_BOOTS), POCKET_TOOLS);
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
#include "gym_room.h"
#include "pokedex.h"
#include "fieldmap.h"

TEST("GymHack: decor: buying takes the money and adds to storage")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 1500);
    gSpecialVar_0x8004 = GYM_DECOR_POTTED_PLANT;
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 1500 - gGymDecor[GYM_DECOR_POTTED_PLANT].price);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_POTTED_PLANT), 1);
    // Not enough left for a second one
    gSpecialVar_0x8009 = 0;
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
    gSpecialVar_0x8009 = 0;
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
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 2);
    EXPECT(GymDecor_Place(GYM_DECOR_TABLE, 0, 3, 5));
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 1);
    EXPECT_EQ(GymDecor_CountPlaced(GYM_DECOR_TABLE), 1);
    // The table is 2x2: these overlap it
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TABLE, 0, 4, 6));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TABLE, 0, 2, 4));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_TABLE, 0, 5, 5));
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
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, 5, 0));                         // back wall
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, GYM_PC_FRONT_X, GYM_PC_FRONT_Y - 1)); // entrance area
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, -1, 5));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, GYM_EDIT_X1, GYM_EDIT_Y1));  // 2 tall: sticks out
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, GYM_EDIT_X1, GYM_EDIT_Y1 - 1));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, 0, 5));                      // the side walls
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, GYM_ROOM_WIDTH - 1, 5));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, 1, 5));                       // right up against them
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, GYM_ROOM_WIDTH - 2, 5));
    EXPECT_EQ(GYM_EDIT_Y1, GYM_ROOM_HEIGHT - 2);                                      // the bottom row
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, 2, GYM_EDIT_Y1 - 1));
    EXPECT(!GymRoom_IsPaintable(0, 10));
    EXPECT(GymRoom_IsPaintable(2, GYM_ROOM_HEIGHT - 2));
}

TEST("GymHack: decor: buying several at once")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, gGymDecor[GYM_DECOR_GYM_WALL].price * 20 + 5);
    gSpecialVar_0x8004 = GYM_DECOR_GYM_WALL;
    gSpecialVar_0x8009 = 21;                                    // (one too many)
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_GYM_WALL), 0);
    gSpecialVar_0x8009 = 20;
    GymDecor_BufferDecorInfo();
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_GYM_WALL), 20);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 5);
    // Storage holds 255 of each
    GymDecor_SetOwned(GYM_DECOR_GYM_WALL, 250);
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    gSpecialVar_0x8009 = 6;
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 2);
    gSpecialVar_0x8009 = 5;
    GymDecor_TryBuy();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_GYM_WALL), 255);
    gSpecialVar_0x8009 = 0;
}

TEST("GymHack: decor: selling what's in storage, for half price; putting everything away")
{
    u32 id, facing;
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 0);
    GymDecor_SetOwned(GYM_DECOR_TABLE, 3);
    EXPECT(GymDecor_Place(GYM_DECOR_TABLE, 0, 3, 5));
    gSpecialVar_0x8004 = GYM_DECOR_TABLE;
    gSpecialVar_0x8009 = 3;                                     // (only 2 aren't placed)
    GymDecor_Sell();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TABLE), 3);
    gSpecialVar_0x8009 = 2;
    GymDecor_Sell();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TABLE), 1);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), gGymDecor[GYM_DECOR_TABLE].price);
    // Picking a piece up says which it was and how it faced
    GymDecor_SetOwned(GYM_DECOR_NORMAL_COUCH, 1);
    EXPECT(GymDecor_Place(GYM_DECOR_NORMAL_COUCH, 2, 10, 5));
    EXPECT(GymDecor_TakeAt(11, 5, &id, &facing));
    EXPECT_EQ(id, GYM_DECOR_NORMAL_COUCH);
    EXPECT_EQ(facing, 2);
    // Everything back in storage
    GymDecor_PutAllAway();
    EXPECT_EQ(GymDecor_NumPlaced(), 0);
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TABLE), 1);
    // The free and the bought-once can't be sold
    gSpecialVar_0x8004 = GYM_DECOR_BATTLE_STAGE;
    gSpecialVar_0x8009 = 1;
    GymDecor_Sell();
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_BATTLE_STAGE), 1);
    gSpecialVar_0x8009 = 0;
}

TEST("GymHack: decor: a turned piece takes up its turned shape")
{
    s32 x = 3, y = 5;
    GymDecor_ClearAll();
    GymDecor_SetOwned(GYM_DECOR_NORMAL_COUCH, 2);
    EXPECT_EQ(gGymDecor[GYM_DECOR_NORMAL_COUCH].numFacings, 4);
    EXPECT_EQ(gGymDecor[GYM_DECOR_NORMAL_COUCH].facings[0].width, 3);
    EXPECT_EQ(gGymDecor[GYM_DECOR_NORMAL_COUCH].facings[1].width, 2);
    EXPECT_EQ(gGymDecor[GYM_DECOR_NORMAL_COUCH].facings[1].height, 3);
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_NORMAL_COUCH, 4, x, y));        // (only 4 ways round)
    EXPECT(GymDecor_Place(GYM_DECOR_NORMAL_COUCH, 1, x, y));
    EXPECT_EQ(GYM_PLACEMENT_FACING(&GymDecor_GetSaveData()->placed[0]), 1);
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_NORMAL_COUCH, 0, x + 1, y + 2)); // (its third row)
    EXPECT(GymDecor_CanPlace(GYM_DECOR_NORMAL_COUCH, 0, x + 2, y));      // (it's 2 wide now)
    EXPECT(GymDecor_RemoveAt(x + 1, y + 2));
    EXPECT_EQ(GymDecor_NumPlaced(), 0);
    // Round things don't turn
    EXPECT_EQ(gGymDecor[GYM_DECOR_POTTED_PLANT].numFacings, 1);
}

TEST("GymHack: decor: tall pieces stand right against a wall, their tops over its face")
{
    GymDecor_ClearAll();
    GymDecor_SetOwned(GYM_DECOR_GYM_WALL, 5);
    GymDecor_SetOwned(GYM_DECOR_TRASH_CAN, 5);
    GymDecor_SetOwned(GYM_DECOR_POTTED_PLANT, 5);
    GymDecor_SetOwned(GYM_DECOR_BOOKSHELF, 2);
    GymDecor_SetOwned(GYM_DECOR_GREEN_RUG, 1);
    GymDecor_SetOwned(GYM_DECOR_POOL_WATER, 1);
    EXPECT(GymDecor_Place(GYM_DECOR_GYM_WALL, 0, 5, 5));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_POTTED_PLANT, 0, 5, 6));  // (its top over the wall's face)
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 5, 6));    // (one square tall: it'd be in the wall)
    EXPECT(GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 5, 7));     // (in front of it)
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_GREEN_RUG, 0, 5, 6));    // (nor a rug, flat on the floor,
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_POOL_WATER, 0, 5, 6));   // or a pool)
    EXPECT(GymDecor_Place(GYM_DECOR_POTTED_PLANT, 0, 5, 6));
    // The gym's own back wall: a tall piece's top can overlap it, nothing else
    EXPECT(GymDecor_CanPlace(GYM_DECOR_BOOKSHELF, 0, 3, 1));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 3, 1));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_BOOKSHELF, 0, 3, 0));
    EXPECT(GymDecor_Place(GYM_DECOR_BOOKSHELF, 0, 3, 1));
    // A wall can go up behind a tall piece, but not right behind a short one or a rug
    EXPECT(GymDecor_Place(GYM_DECOR_POTTED_PLANT, 0, 9, 6));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_GYM_WALL, 0, 9, 5));
    EXPECT(GymDecor_Place(GYM_DECOR_TRASH_CAN, 0, 11, 6));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_GYM_WALL, 0, 11, 5));
    EXPECT(GymDecor_Place(GYM_DECOR_GREEN_RUG, 0, 13, 6));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_GYM_WALL, 0, 13, 5));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_GYM_WALL, 0, 13, 4));     // (not right behind it)
    // The wall goes on down, over the piece's square once it's moved
    EXPECT(GymDecor_RemoveAt(5, 7));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_GYM_WALL, 0, 5, 6));
}

TEST("GymHack: decor: the floor round the statues, the PC and the doormat can be painted")
{
    GymDecor_ClearAll();
    EXPECT(GymRoom_IsPaintable(10, GYM_ROOM_HEIGHT - 4));       // (behind the statues)
    EXPECT(GymRoom_IsPaintable(14, GYM_ROOM_HEIGHT - 4));
    EXPECT(GymRoom_IsPaintable(GYM_PC_FRONT_X, GYM_PC_FRONT_Y - 1));
    EXPECT(!GymRoom_IsPaintable(10, GYM_ROOM_HEIGHT - 3));      // (the statues themselves)
    EXPECT(!GymRoom_IsPaintable(12, GYM_ROOM_HEIGHT - 1));      // (below the doormat)
}

static void SetLocation(u16 map)
{
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(map);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(map);
}

TEST("GymHack: decor: the extra rooms share the storage, with their own floors and walls")
{
    struct WarpData saved = gSaveBlock1Ptr->location;
    GymDecor_ClearAll();
    FlagClear(FLAG_GYM_ROOM_BACK);
    GymDecor_SetOwned(GYM_DECOR_TRASH_CAN, 2);
    GymDecor_SetOwned(GYM_DECOR_TABLE, 1);
    GymDecor_SetOwned(GYM_DECOR_BOOKSHELF, 1);
    SetLocation(MAP_PALLET_TOWN_GYM);
    EXPECT_EQ(GymRoom_Current(), GYM_ROOM_MAIN);
    // A doorway's way through is kept clear once it's open
    EXPECT(GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 21, 2));
    EXPECT(GymDecor_CanPlace(GYM_DECOR_BOOKSHELF, 0, 20, 1));   // (over the wall, till the doorway's there)
    FlagSet(FLAG_GYM_ROOM_BACK);
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_BOOKSHELF, 0, 20, 1));
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 21, 2));
    EXPECT(GymDecor_Place(GYM_DECOR_TRASH_CAN, 0, 11, 2));
    // The back room: its own size, floors and walls; the gym's storage
    SetLocation(MAP_PALLET_TOWN_GYM_BACK_ROOM);
    EXPECT_EQ(GymRoom_Current(), GYM_ROOM_BACK);
    EXPECT_EQ(GymRoom_CurrentInfo()->width, GYM_EXTRA_ROOM_WIDTH);
    EXPECT_EQ(GymDecor_CountAvailable(GYM_DECOR_TRASH_CAN), 1);
    EXPECT(GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 11, 2));   // (the gym's is in the gym)
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_TRASH_CAN, 0, 20, 5));  // (outside this room)
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_BATTLE_STAGE, 0, 3, 4)); // (only in the gym)
    EXPECT(GymDecor_Place(GYM_DECOR_TABLE, 0, 3, 4));
    EXPECT_EQ(GymDecor_IdAtMapPos(3 + MAP_OFFSET, 4 + MAP_OFFSET), GYM_DECOR_TABLE);
    GymRoom_SetFloor(8, 8, 2);
    GymRoom_SetWallStyle(3);
    EXPECT_EQ(GymRoom_GetFloor(8, 8), 2);
    GymDecor_PutAllAway();                                       // (this room's, not the gym's)
    EXPECT_EQ(GymDecor_NumPlaced(), 1);
    EXPECT(GymDecor_Place(GYM_DECOR_TABLE, 0, 3, 4));
    SetLocation(MAP_PALLET_TOWN_GYM);
    EXPECT_EQ(GymRoom_GetFloor(8, 8), 0);
    EXPECT_EQ(GymRoom_GetWallStyle(), 0);
    EXPECT_EQ(GymDecor_IdAtMapPos(3 + MAP_OFFSET, 4 + MAP_OFFSET), GYM_DECOR_COUNT);
    EXPECT_EQ(GymDecor_CountPlaced(GYM_DECOR_TABLE), 1);
    SetLocation(MAP_PALLET_TOWN_GYM_LEFT_ROOM);
    EXPECT_EQ(GymRoom_GetFloor(8, 8), 0);
    EXPECT_EQ(GymRoom_CurrentInfo()->pcFrontX, GYM_EXTRA_ROOM_WIDTH - 2);
    FlagClear(FLAG_GYM_ROOM_BACK);
    gSaveBlock1Ptr->location = saved;
}

TEST("GymHack: decor: Oak builds a room for every 100 kinds caught")
{
    u32 i;
    FlagClear(FLAG_GYM_ROOM_BACK);
    FlagClear(FLAG_GYM_ROOM_LEFT);
    FlagClear(FLAG_GYM_ROOM_RIGHT);
    FlagSet(FLAG_PALLET_GYM_OPEN);
    for (i = 1; i <= 99; i++)
        GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
    GymRooms_OakReward();
    EXPECT_EQ(gSpecialVar_Result, 0);
    GetSetPokedexFlag(100, FLAG_SET_CAUGHT);
    GymRooms_OakReward();
    EXPECT_EQ(gSpecialVar_Result, 1);
    gSpecialVar_0x8004 = 1;
    GymRooms_GiveReward();
    EXPECT(FlagGet(FLAG_GYM_ROOM_BACK));
    GymRooms_OakReward();
    EXPECT_EQ(gSpecialVar_Result, 0);
    for (i = 101; i <= 200; i++)
        GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
    GymRooms_OakReward();
    EXPECT_EQ(gSpecialVar_Result, 2);
    FlagClear(FLAG_GYM_ROOM_BACK);
}

TEST("GymHack: decor: floors are painted square by square, and fill an area")
{
    s32 x, y;
    u32 n = 0;
    GymDecor_ClearAll();
    EXPECT_EQ(GymRoom_GetFloor(4, 4), 0);
    EXPECT(!GymRoom_IsPaintable(4, 0));                          // the back wall
    EXPECT(GymRoom_IsPaintable(4, 4));
    GymRoom_SetFloor(4, 4, 3);
    GymRoom_SetFloor(5, 4, 9);
    EXPECT_EQ(GymRoom_GetFloor(4, 4), 3);
    EXPECT_EQ(GymRoom_GetFloor(5, 4), 9);
    EXPECT_EQ(GymRoom_GetFloor(3, 4), 0);
    // Filling the gym floor around them: everything else with a floor
    for (y = 0; y < GYM_ROOM_HEIGHT; y++)
        for (x = 0; x < GYM_ROOM_WIDTH; x++)
            if (GymRoom_IsPaintable(x, y))
                n++;
    EXPECT_EQ(GymRoom_FillFloor(10, 10, 1, NULL), n - 2);
    EXPECT_EQ(GymRoom_GetFloor(4, 4), 3);
    EXPECT_EQ(GymRoom_GetFloor(3, 4), 1);
    EXPECT_EQ(GymRoom_FillFloor(10, 10, 1, NULL), 0);           // (already that floor)
    EXPECT(!GymRoom_IsSolidFloor(1));
    EXPECT(GymRoom_IsSolidFloor(gGymDecor[GYM_DECOR_FLOOR_LAVA].style));
}

TEST("GymHack: decor: floors and wall styles are bought once; a wall style is used for all the walls")
{
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_FLOOR_GYM), 1);      // (the gym's own)
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_WALLS_GYM), 1);
    gSpecialVar_0x8004 = GYM_DECOR_FLOOR_WOOD;
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 0);
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT_EQ(gSpecialVar_Result, 3);
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_FLOOR_WOOD), 1);
    EXPECT(!GymDecor_CanPlace(GYM_DECOR_FLOOR_WOOD, 0, 5, 5));   // (painted, not placed)
    // A wall style not owned can't be used
    gSpecialVar_0x8004 = GYM_DECOR_WALLS_BRICK;
    GymDecor_ApplyWallStyle();
    EXPECT_EQ(GymRoom_GetWallStyle(), 0);
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    GymDecor_ApplyWallStyle();
    EXPECT_EQ(GymRoom_GetWallStyle(), gGymDecor[GYM_DECOR_WALLS_BRICK].style);
}

TEST("GymHack: decor: the decorations' save keeps its size (the gym challenge's records follow it)")
{
    u32 size;
    void *spare = GymDecor_GetSpareSaveSpace(&size);
    EXPECT_EQ(sizeof(struct GymDecorSaveData), 2860);
    EXPECT_EQ((u8 *)spare - (u8 *)gSaveBlock1Ptr->secretBases, 2860);
    EXPECT_EQ(size, sizeof(gSaveBlock1Ptr->secretBases) - 2860);
}

TEST("GymHack: decor: every decoration has a name, a price and fits the gym")
{
    u32 i;
    for (i = 0; i < GYM_DECOR_COUNT; i++)
    {
        EXPECT(gGymDecor[i].name != NULL);
        u32 f;
        if (i != GYM_DECOR_BATTLE_STAGE && GymCandidate_GetRecruitByDecor(i) == NULL   // recruits join, they aren't sold
         && gGymDecor[i].category != GYM_DECOR_CAT_TROPHIES                           // (trophies are won)
         && i != GYM_DECOR_FLOOR_GYM && i != GYM_DECOR_WALLS_GYM                      // (the gym's own)
         && i != GYM_DECOR_DRINK_MACHINE)                                             // (a gift)
            EXPECT_GT(gGymDecor[i].price, 0);
        EXPECT(gGymDecor[i].numFacings >= 1 && gGymDecor[i].numFacings <= 4);
        for (f = 0; f < gGymDecor[i].numFacings; f++)
        {
            EXPECT_LE(gGymDecor[i].facings[f].width, 4);
            EXPECT_LE(gGymDecor[i].facings[f].height, 5);
            if (gGymDecor[i].kind == GYM_DECOR_KIND_PIECE)
                EXPECT(gGymDecor[i].facings[f].looks != NULL);
        }
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

TEST("GymHack: items: Ability Capsule offers the regular abilities, not the hidden one")
{
    u8 slots[NUM_ABILITY_SLOTS];
    u32 slot = 0;
    GiveTestMon(SPECIES_GROWLITHE);   // Intimidate / Flash Fire / Justified (hidden)
    SetMonData(TEST_MON, MON_DATA_ABILITY_NUM, &slot);
    EXPECT_EQ(StatItems_GetAbilityChoices(TEST_MON, slots), 2);
    EXPECT(!StatItems_SetAbilitySlot(TEST_MON, 0));              // the one it has
    EXPECT(!StatItems_SetAbilitySlot(TEST_MON, 2));              // the hidden one: the Ability Patch's
    EXPECT(StatItems_SetAbilitySlot(TEST_MON, 1));
    EXPECT_EQ(GetMonAbility(TEST_MON), ABILITY_FLASH_FIRE);
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
    bool32 silver = FALSE, gold = FALSE, capsule = FALSE, patch = FALSE;
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
        patch |= items[i] == ITEM_ABILITY_PATCH;
        gold |= items[i] == ITEM_GOLD_BOTTLE_CAP;
    }
    EXPECT(silver && capsule && patch && !gold);
    EXPECT_EQ(GetItemPrice(ITEM_BOTTLE_CAP), 50000);
    EXPECT_EQ(GetItemPrice(ITEM_GOLD_BOTTLE_CAP), 150000);
    EXPECT_EQ(GetItemPrice(ITEM_ABILITY_CAPSULE), 100000);
    EXPECT_EQ(GetItemPrice(ITEM_ABILITY_PATCH), 250000);
    EXPECT_EQ(GetItemPrice(ITEM_CILANTRO), 40000);
    EXPECT_LT(GetItemPrice(ITEM_ADAMANT_MINT), GetItemPrice(ITEM_CILANTRO));
    EXPECT_EQ(GetItemPrice(ITEM_MULTIVITAMIN), 100000);
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
            EXPECT(t->party[i].species <= SPECIES_DEOXYS   // (or one of the new eeveelutions)
                || (t->party[i].species >= SPECIES_ODEON && t->party[i].species <= SPECIES_VALYREON));
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
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("BLUE"));    // (the rival's lines name him)
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
    gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
    EXPECT_NE(gSpecialVar_Result, 0);
    EXPECT(!GymDecor_FindPlaced(GYM_DECOR_BATTLE_STAGE, &x, &y));
    EXPECT(GymDecor_Place(GYM_DECOR_BATTLE_STAGE, 0, 3, 5));
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

TEST("GymHack: finale: Oak's team for the gym is six strong Pokemon at the player's level + 1")
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
    EXPECT_EQ(GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->party[0].lvl, 81 + GYM_OAK_FINALE_MARGIN);
    EXPECT_EQ(GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->trainerClass, TRAINER_CLASS_PKMN_PROF_FRLG);
    for (i = 0; i < MAX_TRAINER_ITEMS; i++)     // (no battle items in post-game battles)
        EXPECT_EQ(GetTrainerStructFromId(TRAINER_GYM_CHALLENGER)->items[i], ITEM_NONE);
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
    u32 i, x = 1, y = 4;
    GymDecor_ClearAll();
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    for (i = 0; i < 14; i++)
    {
        gSpecialVar_0x8004 = GYM_DECOR_TRAINER_BEGINNER + i % 3;
        gSpecialVar_0x8009 = 0;
    GymDecor_TryBuy();
        EXPECT_EQ(gSpecialVar_Result, 0);
    }
    EXPECT_EQ(GymDecor_CountOwned(GYM_DECOR_TRAINER_BEGINNER), 5);
    for (i = 0; i < 14; i++)
    {
        bool32 placed = GymDecor_Place(GYM_DECOR_TRAINER_BEGINNER + i % 3, 0, x, y);
        EXPECT_EQ(placed, i < GYM_PRACTICE_SLOTS);
        x += 2;
        if (x > 20)
        {
            x = 1;
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
    EXPECT(GymDecor_Place(GymCandidate_GetRecruit(TYPE_FAIRY)->decor, 0, 3, 5));
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
    EXPECT(IsKeyItemPocket(GetItemPocket(ITEM_PALM_PILOT)));
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

TEST("GymHack: switch: the mastery battle is the 15th win, then the 10th")
{
    u32 t;
    ResetGymTypes(TYPE_ROCK);
    EXPECT(GymSwitch_IsMasteryBattle(15));
    EXPECT(!GymSwitch_IsMasteryBattle(10));
    EXPECT(!GymSwitch_IsMasteryBattle(16));
    GymChallenge_GetSave()->switchesEarned = 1;
    EXPECT(GymSwitch_IsMasteryBattle(10));
    EXPECT(!GymSwitch_IsMasteryBattle(15));
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
    GymChallenge_GetSave()->streak = GYM_SWITCH_FIRST_STREAK - 1;
    GymChallenge_NextChallenger();
    EXPECT_EQ(gSpecialVar_Result, GYM_CHALLENGER_BOSS);
    trainer = GymChallenge_GetTrainer(TRAINER_GYM_CHALLENGER);
    EXPECT_EQ(StringCompare(trainer->trainerName, COMPOUND_STRING("MISTY")), 0);
    for (i = 0; i < MAX_TRAINER_ITEMS; i++)       // (no battle items)
        EXPECT_EQ(trainer->items[i], ITEM_NONE);
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

TEST("GymHack: CLEFFABILL is Fairy/Ghost, 800 in all, and not in the Pokedex")
{
    const struct SpeciesInfo *s = &gSpeciesInfo[SPECIES_CLEFFABILL];
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

TEST("GymHack: winning the tournament: the Hall of Fame, then Bill; in the simulator, Bill again for a WITHDRAW")
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
        // (the title alone isn't the WITHDRAW: Bill's simulation is next)
        EXPECT_EQ(gTourneySave.run.status, TOURNEY_BILL);
        EXPECT_EQ(VarGet(VAR_GAUNTLET_WITHDRAWS), n - 2);
        Tourney_GetStatus();
        EXPECT_EQ(gSpecialVar_Result, TOURNEY_BILL);
        Tourney_SimBillWon();
        EXPECT_EQ(gTourneySave.run.status, TOURNEY_WON);
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
    EXPECT(!Archive_IsSlot(SPECIES_CLEFFABILL));
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

// Primrose's Mega trials: for every stone, three Pokémon at 45-50, her Mega last holding its stone,
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
            EXPECT_EQ(team[2].lvl, 50);
            EXPECT_NE(team[2].heldItem, ITEM_NONE);
            for (k = 0; k < 3; k++)
            {
                u32 lvl = team[k].lvl;
                EXPECT_GE(lvl, 45);
                EXPECT_LE(lvl, 50);
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
    EXPECT_EQ(Academy_MegaTrialTeam(SPECIES_KYOGRE, ITEM_BLUE_ORB)[2].lvl, 50);
    EXPECT_EQ(Academy_MegaTrialTeam(SPECIES_GROUDON, ITEM_RED_ORB)[2].lvl, 50);
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

// ---- The Pokédex count ----
#include "pokedex.h"
#include "menu.h"
#include "string_util.h"

TEST("GymHack: the save menu counts a starter from outside Kanto")
{
    u8 str[16], expected[16];
    ResetPokedex();
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_RALTS), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_RALTS), FLAG_SET_CAUGHT);
    BufferSaveMenuText(SAVE_MENU_CAUGHT, str, 0);
    ConvertIntToDecimalStringN(expected, 1, STR_CONV_MODE_LEFT_ALIGN, 4);
    // (the colour codes come first)
    EXPECT_EQ(StringCompare(str + StringLength(str) - StringLength(expected), expected), 0);
}

TEST("GymHack: items: the Lucky Egg and Amulet Coin level up to 10")
{
    u32 i;
    EXPECT_EQ(ItemUpgrade_GetLevel(ITEM_LUCKY_EGG), 1);      // (a new save: the var is 0)
    EXPECT_EQ(ItemUpgrade_ExpPercent(), 150);
    EXPECT_EQ(ItemUpgrade_MoneyPercent(), 200);
    gSpecialVar_0x8004 = ITEM_LUCKY_EGG;
    for (i = 2; i <= ITEM_UPGRADE_MAX_LEVEL; i++)
    {
        ItemUpgrade_Upgrade();
        EXPECT_EQ(gSpecialVar_Result, i);
    }
    ItemUpgrade_Upgrade();
    EXPECT_EQ(gSpecialVar_Result, 0);                        // (at the top)
    EXPECT_EQ(ItemUpgrade_ExpPercent(), 600);
    EXPECT_EQ(ItemUpgrade_GetLevel(ITEM_AMULET_COIN), 1);    // (each item has its own level)
    ItemUpgrade_SetLevel(ITEM_AMULET_COIN, 10);
    EXPECT_EQ(ItemUpgrade_MoneyPercent(), 650);
    EXPECT(!ItemUpgrade_SetLevel(ITEM_POTION, 3));
    EXPECT(ItemUpgrade_GetDescription(ITEM_POTION) == NULL);
    ItemUpgrade_SetLevel(ITEM_LUCKY_EGG, 1);
    ItemUpgrade_SetLevel(ITEM_AMULET_COIN, 1);
}

TEST("GymHack: items: the Lucky Egg's and Amulet Coin's levels cost clovers and coins")
{
    u32 level, total[2] = {0}, i;
    static const u16 items[] = {ITEM_LUCKY_EGG, ITEM_AMULET_COIN};
    static const u16 tiers[2][3] = {
        {ITEM_LUCKY_CLOVER, ITEM_FOUR_LEAF_CLOVER, ITEM_GOLDEN_CLOVER},
        {ITEM_COPPER_COIN, ITEM_SILVER_COIN, ITEM_GOLD_COIN},
    };
    ClearBag();
    for (i = 0; i < 2; i++)
    {
        ItemUpgrade_SetLevel(items[i], 1);
        gSpecialVar_0x8004 = items[i];
        for (level = 2; level <= ITEM_UPGRADE_MAX_LEVEL; level++)
        {
            u32 item = ItemUpgrade_GetCostItem(items[i], level), count = ItemUpgrade_GetCostCount(items[i], level);
            EXPECT_EQ(item, tiers[i][(level - 2) / 3]);     // (common, rare, very rare)
            EXPECT_GT(count, 0);
            total[i] += count;
            ItemUpgrade_CheckCost();
            EXPECT_EQ(gSpecialVar_Result, ITEM_UPGRADE_SHORT);
            ItemUpgrade_PayAndUpgrade();
            EXPECT_EQ(gSpecialVar_Result, 0);               // (nothing to pay with: nothing changes)
            EXPECT_EQ(ItemUpgrade_GetLevel(items[i]), level - 1);
            AddBagItem(item, count + 1);
            ItemUpgrade_CheckCost();
            EXPECT_EQ(gSpecialVar_Result, ITEM_UPGRADE_CAN_PAY);
            EXPECT_EQ(gSpecialVar_0x8005, item);
            ItemUpgrade_PayAndUpgrade();
            EXPECT_EQ(gSpecialVar_Result, level);
            EXPECT(CheckBagHasItem(item, 1) && !CheckBagHasItem(item, 2));   // (it took `count`)
            RemoveBagItem(item, 1);
        }
        ItemUpgrade_CheckCost();
        EXPECT_EQ(gSpecialVar_Result, ITEM_UPGRADE_MAXED);
        ItemUpgrade_SetLevel(items[i], 1);
    }
    EXPECT_EQ(total[0], 1 + 2 + 3 + 1 + 2 + 3 + 1 + 1 + 2);
    EXPECT_EQ(total[1], total[0]);
    ClearBag();
}

extern const u8 PalletTown_Text_ChanseyStatueLegend[], PalletTown_Text_ChanseyStatueLeaveClovers[],
    PalletTown_Text_ChanseyStatueEggGlows[], PalletTown_Text_ChanseyStatueEggLevel[], PalletTown_Text_ChanseyStatueShort[],
    PalletTown_Text_ChanseyStatueMaxed[], PalletTown_RivalsHouse_Text_DaisyGrandpasCoins[],
    PalletTown_RivalsHouse_Text_DaisySetCoins[], PalletTown_RivalsHouse_Text_DaisyLetMeSee[],
    PalletTown_RivalsHouse_Text_DaisyCoinLevel[], PalletTown_RivalsHouse_Text_DaisyNeedMoreCoins[],
    PalletTown_RivalsHouse_Text_DaisyCoinMaxed[], PalletTown_RivalsHouse_Text_DaisyKeepThem[];

TEST("GymHack: items: the Chansey statue's and Daisy's upgrade lines fit the message box")
{
    static const u8 *const texts[] = {
        PalletTown_Text_ChanseyStatueLegend, PalletTown_Text_ChanseyStatueLeaveClovers,
        PalletTown_Text_ChanseyStatueEggGlows, PalletTown_Text_ChanseyStatueEggLevel, PalletTown_Text_ChanseyStatueShort,
        PalletTown_Text_ChanseyStatueMaxed, PalletTown_RivalsHouse_Text_DaisyGrandpasCoins,
        PalletTown_RivalsHouse_Text_DaisySetCoins, PalletTown_RivalsHouse_Text_DaisyLetMeSee,
        PalletTown_RivalsHouse_Text_DaisyCoinLevel, PalletTown_RivalsHouse_Text_DaisyNeedMoreCoins,
        PalletTown_RivalsHouse_Text_DaisyCoinMaxed, PalletTown_RivalsHouse_Text_DaisyKeepThem,
    };
    u8 *buf = Alloc(1000);
    u32 i;
    // (the longest: "3 Four-Leaf Clovers", level 10, "6.5")
    u8 *s = StringCopy(gStringVar1, COMPOUND_STRING("3 "));
    CopyItemNameHandlePlural(ITEM_FOUR_LEAF_CLOVER, s, 3);
    StringCopy(gStringVar2, COMPOUND_STRING("10"));
    StringCopy(gStringVar3, COMPOUND_STRING("6.5"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
    {
        u8 *line, *p;
        StringExpandPlaceholders(buf, texts[i]);
        if (texts[i] == PalletTown_Text_ChanseyStatueEggLevel || texts[i] == PalletTown_RivalsHouse_Text_DaisyCoinLevel)
        {
            // (these show the level in STR_VAR_1 and the multiplier in STR_VAR_2)
            StringCopy(gStringVar1, COMPOUND_STRING("10"));
            StringCopy(gStringVar2, COMPOUND_STRING("6.5"));
            StringExpandPlaceholders(buf, texts[i]);
            s = StringCopy(gStringVar1, COMPOUND_STRING("3 "));
            CopyItemNameHandlePlural(ITEM_FOUR_LEAF_CLOVER, s, 3);
            StringCopy(gStringVar2, COMPOUND_STRING("10"));
        }
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

TEST("GymHack: challenges: random challengers have at most one legendary")
{
    u32 run, i, teamsWithOne = 0;
    GivePlayerTeamAt(60, 62);
    for (run = 0; run < 60; run++)
    {
        const struct Trainer *t;
        u32 legends = 0;
        GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
        t = GetTrainerStructFromId(TRAINER_GYM_CHALLENGER);
        for (i = 0; i < t->partySize; i++)
        {
            const struct SpeciesInfo *info = &gSpeciesInfo[t->party[i].species];
            legends += info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
        }
        EXPECT_LE(legends, 1);
        teamsWithOne += legends;
        for (i = 0; i < MAX_TRAINER_ITEMS; i++)
            EXPECT_EQ(t->items[i], ITEM_NONE);
    }
    EXPECT_GT(teamsWithOne, 5);       // (about 40% of teams)
    EXPECT_LT(teamsWithOne, 45);
}

TEST("GymHack: bag: every TM is reusable")
{
    EXPECT_EQ(GetItemImportance(ITEM_TM_FOCUS_PUNCH), 1);
    EXPECT_EQ(GetItemImportance(ITEM_TM_THUNDERBOLT), 1);
    EXPECT_EQ(GetItemImportance(ITEM_HM_CUT), 1);
}

static void Encrypt(struct ItemSlot *slots, u32 n)
{
    u32 i;
    for (i = 0; i < n; i++)
        ApplyNewEncryptionKeyToHword(&slots[i].quantity, gSaveBlock2Ptr->encryptionKey);
}

TEST("GymHack: bag: items go into the ten pockets")
{
    EXPECT_EQ(GetItemPocket(ITEM_POTION), POCKET_MEDICINE);
    EXPECT_EQ(GetItemPocket(ITEM_RARE_CANDY), POCKET_MEDICINE);
    EXPECT_EQ(GetItemPocket(ITEM_X_ATTACK), POCKET_BATTLE_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_LEFTOVERS), POCKET_HOLD_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_CHARIZARDITE_X), POCKET_MEGA_STONES);
    EXPECT_EQ(GetItemPocket(ITEM_NUGGET), POCKET_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_FIRE_STONE), POCKET_ITEMS);
    EXPECT_EQ(GetItemPocket(ITEM_ULTRA_BALL), POCKET_POKE_BALLS);
    EXPECT_EQ(GetItemPocket(ITEM_ORAN_BERRY), POCKET_BERRIES);
    EXPECT_EQ(GetItemPocket(ITEM_TM_THUNDERBOLT), POCKET_TM_HM);
    EXPECT_EQ(GetItemPocket(ITEM_BICYCLE), POCKET_TOOLS);
    EXPECT_EQ(GetItemPocket(ITEM_OLD_ROD), POCKET_TOOLS);
    EXPECT_EQ(GetItemPocket(ITEM_SS_TICKET), POCKET_KEY_ITEMS);
}

TEST("GymHack: bag: one of every item in the game fits")
{
    u32 i, n = 0;
    ClearBag();
    for (i = ITEM_NONE + 1; i < ITEMS_COUNT; i++)
    {
        if (GetItemPocket(i) >= POCKETS_COUNT || gItemsInfo[i].name[0] == 0 || CheckBagHasItem(i, 1))
            continue;
        EXPECT(AddBagItem(i, 1));
        n++;
    }
    EXPECT_GT(n, 900);
    ClearBag();
}

TEST("GymHack: bag: a FireRed save's five pockets move into the ten")
{
    struct OldBag *old = (struct OldBag *)&gSaveBlock1Ptr->bag;

    ClearBag();
    memset(old, 0, sizeof(*old));
    gSaveBlock3Ptr->bagLayout = 0;              // (as an older save's)
    old->items[0] = (struct ItemSlot) {ITEM_POTION, 5};
    old->items[1] = (struct ItemSlot) {ITEM_X_ATTACK, 2};
    old->items[2] = (struct ItemSlot) {ITEM_CHARIZARDITE_X, 1};
    old->items[BAG_OLD_ITEMS_COUNT - 1] = (struct ItemSlot) {ITEM_LEFTOVERS, 1};
    old->keyItems[3] = (struct ItemSlot) {ITEM_BICYCLE, 1};
    old->keyItems[4] = (struct ItemSlot) {ITEM_SS_TICKET, 1};
    old->pokeBalls[0] = (struct ItemSlot) {ITEM_ULTRA_BALL, 12};
    old->TMsHMs[0] = (struct ItemSlot) {ITEM_TM_THUNDERBOLT, 1};
    old->berries[0] = (struct ItemSlot) {ITEM_ORAN_BERRY, 3};
    Encrypt((struct ItemSlot *)old, sizeof(*old) / sizeof(struct ItemSlot));
    Bag_UpgradeLayout();
    EXPECT_EQ(gSaveBlock3Ptr->bagLayout, BAG_LAYOUT_POCKETS);
    EXPECT(CheckBagHasItem(ITEM_POTION, 5));
    EXPECT(CheckBagHasItem(ITEM_X_ATTACK, 2));
    EXPECT(CheckBagHasItem(ITEM_CHARIZARDITE_X, 1));
    EXPECT(CheckBagHasItem(ITEM_LEFTOVERS, 1));
    EXPECT(CheckBagHasItem(ITEM_BICYCLE, 1));
    EXPECT(CheckBagHasItem(ITEM_SS_TICKET, 1));
    EXPECT(CheckBagHasItem(ITEM_ULTRA_BALL, 12));
    EXPECT(CheckBagHasItem(ITEM_TM_THUNDERBOLT, 1));
    EXPECT(CheckBagHasItem(ITEM_ORAN_BERRY, 3));
    EXPECT(!CheckBagHasItem(ITEM_POTION, 6));
    EXPECT_EQ(GetBagItemId(POCKET_MEDICINE, 0), ITEM_POTION);
    EXPECT_EQ(GetBagItemId(POCKET_MEGA_STONES, 0), ITEM_CHARIZARDITE_X);
    EXPECT_EQ(GetBagItemId(POCKET_TOOLS, 0), ITEM_BICYCLE);
    Bag_UpgradeLayout();                        // (once only)
    EXPECT(CheckBagHasItem(ITEM_POTION, 5));
    ClearBag();
}

TEST("GymHack: bag: a v99 save's five big pockets move into the ten")
{
    struct V99Bag *sb1 = (struct V99Bag *)&gSaveBlock1Ptr->bag;
    struct ItemSlot *sb3 = gSaveBlock3Ptr->bagTMsHMs;      // (v99: 108 TMs, then 297 items)
    u32 i;

    ClearBag();
    memset(sb1, 0, sizeof(*sb1));
    memset(sb3, 0, (BAG_V99_TMHM_COUNT + BAG_V99_ITEMS_COUNT) * sizeof(struct ItemSlot));
    gSaveBlock3Ptr->bagLayout = BAG_LAYOUT_SPLIT;
    sb1->keyItems[0] = (struct ItemSlot) {ITEM_OLD_ROD, 1};
    sb1->keyItems[BAG_V99_KEYITEMS_COUNT - 1] = (struct ItemSlot) {ITEM_SS_TICKET, 1};
    sb1->pokeBalls[5] = (struct ItemSlot) {ITEM_GREAT_BALL, 7};
    sb1->berries[0] = (struct ItemSlot) {ITEM_SITRUS_BERRY, 2};
    sb3[0] = (struct ItemSlot) {ITEM_TM_ICE_BEAM, 1};
    for (i = 0; i < 40; i++)    // (forty kinds of items)
        sb3[BAG_V99_TMHM_COUNT + i] = (struct ItemSlot) {ITEM_VENUSAURITE + i, 1};
    sb3[BAG_V99_TMHM_COUNT + 296] = (struct ItemSlot) {ITEM_HYPER_POTION, 9};
    Encrypt((struct ItemSlot *)sb1, sizeof(*sb1) / sizeof(struct ItemSlot));
    Encrypt(sb3, BAG_V99_TMHM_COUNT + BAG_V99_ITEMS_COUNT);
    Bag_UpgradeLayout();
    EXPECT_EQ(gSaveBlock3Ptr->bagLayout, BAG_LAYOUT_POCKETS);
    EXPECT(CheckBagHasItem(ITEM_OLD_ROD, 1));
    EXPECT(CheckBagHasItem(ITEM_SS_TICKET, 1));
    EXPECT(CheckBagHasItem(ITEM_GREAT_BALL, 7));
    EXPECT(CheckBagHasItem(ITEM_SITRUS_BERRY, 2));
    EXPECT(CheckBagHasItem(ITEM_TM_ICE_BEAM, 1));
    EXPECT(CheckBagHasItem(ITEM_HYPER_POTION, 9));
    for (i = 0; i < 40; i++)
        EXPECT(CheckBagHasItem(ITEM_VENUSAURITE + i, 1));
    ClearBag();
}

static u32 IncenseChance(const struct WildPokemon *mons, u32 species)
{
    static const u8 sLand[] = {20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1};
    u32 i, total = 0;
    for (i = 0; i < 12; i++)
    {
        if (mons[i].species == species)
            total += Incense_BoostedChance(mons, 0, sLand, 12, i, 100);
    }
    return total;   // (out of 10000)
}

TEST("GymHack: incense: rare Pokemon are 5 times as likely, the common ones share the rest")
{
    // A 40%, B 45%, D 10% and E 5%: D and E (15%) go to 50% and 25%; A and B share 25%
    static const struct WildPokemon mons[12] = {
        {5, 5, SPECIES_PIDGEY}, {5, 5, SPECIES_PIDGEY}, {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_RATTATA},
        {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_EEVEE}, {5, 5, SPECIES_RATTATA},
        {5, 5, SPECIES_ABRA}, {5, 5, SPECIES_ABRA}, {5, 5, SPECIES_ABRA}, {5, 5, SPECIES_ABRA},
    };
    EXPECT_EQ(IncenseChance(mons, SPECIES_ABRA), 5000);
    EXPECT_EQ(IncenseChance(mons, SPECIES_EEVEE), 2500);
    EXPECT_GE(IncenseChance(mons, SPECIES_PIDGEY) + IncenseChance(mons, SPECIES_RATTATA), 2490);
    EXPECT_LE(IncenseChance(mons, SPECIES_PIDGEY) + IncenseChance(mons, SPECIES_RATTATA), 2500);
}

TEST("GymHack: incense: when the rare ones would pass 100%, they share it all")
{
    // A 40%, B 40%; C 10%, D 8%, E 2% (20%: 5 times is 100%) - C 50%, D 40%, E 10%, A and B none
    static const struct WildPokemon mons[12] = {
        {5, 5, SPECIES_PIDGEY}, {5, 5, SPECIES_PIDGEY}, {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_RATTATA},
        {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_RATTATA}, {5, 5, SPECIES_ABRA}, {5, 5, SPECIES_ABRA},
        {5, 5, SPECIES_EEVEE}, {5, 5, SPECIES_EEVEE}, {5, 5, SPECIES_MEW}, {5, 5, SPECIES_MEW},
    };
    EXPECT_EQ(IncenseChance(mons, SPECIES_ABRA), 5000);
    EXPECT_EQ(IncenseChance(mons, SPECIES_EEVEE), 4000);
    EXPECT_EQ(IncenseChance(mons, SPECIES_MEW), 1000);
    EXPECT_EQ(IncenseChance(mons, SPECIES_PIDGEY), 0);
}

TEST("GymHack: incense: lasts the Safari Zone's 600 steps and is sold at 50,000")
{
    VarSet(VAR_INCENSE_STEPS, 0);
    EXPECT(!Incense_IsActive());
    Incense_Start();
    EXPECT_EQ(VarGet(VAR_INCENSE_STEPS), 600);
    EXPECT_EQ(GetItemPrice(ITEM_INCENSE), 50000);
    EXPECT_EQ(GetItemPocket(ITEM_INCENSE), POCKET_ITEMS);
    VarSet(VAR_INCENSE_STEPS, 0);
}

TEST("GymHack: bag: a v100 save's hold items and last items move to their new places")
{
    struct ItemSlot *trees = (struct ItemSlot *)gSaveBlock1Ptr->berryTrees;
    ClearBag();
    gSaveBlock3Ptr->bagLayout = BAG_LAYOUT_POCKETS_V100;
    trees[0] = (struct ItemSlot) {ITEM_LEFTOVERS, 1};
    trees[238] = (struct ItemSlot) {ITEM_LIFE_ORB, 1};
    trees[239] = (struct ItemSlot) {ITEM_NUGGET, 4};       // (v100's items 205-221)
    trees[255] = (struct ItemSlot) {ITEM_PEARL, 2};
    Encrypt(trees, 256);
    Bag_UpgradeLayout();
    EXPECT_EQ(gSaveBlock3Ptr->bagLayout, BAG_LAYOUT_POCKETS);
    EXPECT(CheckBagHasItem(ITEM_LEFTOVERS, 1));
    EXPECT(CheckBagHasItem(ITEM_LIFE_ORB, 1));
    EXPECT(CheckBagHasItem(ITEM_NUGGET, 4));
    EXPECT(CheckBagHasItem(ITEM_PEARL, 2));
    ClearBag();
}

extern const u8 Route8_EventScript_Andrea[];
extern const u8 Route8_EventScript_Julia[];

TEST("GymHack: recruits: a recruit placed in the GYM is gone from their route until put away")
{
    s32 x, y;
    bool32 placed = FALSE;
    GymDecor_ClearAll();
    GymDecor_SetOwned(GYM_DECOR_RECRUIT_NORMAL, 1);
    EXPECT(!GymCandidate_IsPlacedRecruitScript(Route8_EventScript_Andrea));   // (only in the PC)
    for (y = 0; y < 20 && !placed; y++)
        for (x = 0; x < 20 && !placed; x++)
            placed = GymDecor_Place(GYM_DECOR_RECRUIT_NORMAL, 0, x, y);
    EXPECT(placed);
    EXPECT(GymCandidate_IsPlacedRecruitScript(Route8_EventScript_Andrea));
    EXPECT(!GymCandidate_IsPlacedRecruitScript(Route8_EventScript_Julia));   // (the FAIRY recruit)
    EXPECT(!GymCandidate_IsPlacedRecruitScript(NULL));
    GymDecor_PutAllAway();
    EXPECT(!GymCandidate_IsPlacedRecruitScript(Route8_EventScript_Andrea));
    GymDecor_ClearAll();
}

#include "synthetic_abilities.h"

TEST("GymHack: synthetic abilities: every Pokémon has one, and Chansey's is Triage")
{
    u32 i, missing = 0;
    EXPECT_EQ(GetSpeciesSyntheticAbility(SPECIES_CHANSEY), ABILITY_TRIAGE);
    EXPECT_EQ(GetSpeciesSyntheticAbility(SPECIES_BLISSEY), ABILITY_TRIAGE);
    EXPECT_EQ(GetSpeciesSyntheticAbility(SPECIES_PIKACHU_ORIGINAL), ABILITY_TRANSISTOR);   // (a form: its species')
    for (i = 1; i < NUM_SPECIES; i++)
    {
        if (!IsSpeciesEnabled(i) || gSpeciesInfo[i].isMegaEvolution || gSpeciesInfo[i].isGigantamax
         || gSpeciesInfo[i].isTotem || gSpeciesInfo[i].isPrimalReversion || gSpeciesInfo[i].isUltraBurst
         || gSpeciesInfo[i].isTeraForm || gSpeciesInfo[i].natDexNum == 0)
            continue;
        if (GetSpeciesListedSyntheticAbility(i) == ABILITY_NONE)
        {
            DebugPrintf("no synthetic ability: %d", i);
            missing++;
        }
    }
    EXPECT_EQ(missing, 0);
}

#include "basement_machines.h"
#include "item_menu.h"

TEST("GymHack: synthetic ability: slot 3 gives the species' synthetic ability, a blocked one keeps its own")
{
    u32 slot = ABILITY_NUM_SYNTHETIC;
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHANSEY, 30;
        givemon SPECIES_DOUBLADE, 40;
    );
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM, &slot);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_ABILITY_NUM, &slot);
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][0]), ABILITY_TRIAGE);
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][1]), ABILITY_SHARPNESS);
    // Doublade evolves into Aegislash, which the Synthesizer won't take: it keeps Stance Change
    EXPECT_EQ(GetAbilityBySpecies(SPECIES_AEGISLASH_SHIELD, ABILITY_NUM_SYNTHETIC), ABILITY_STANCE_CHANGE);
    EXPECT_EQ(GetSpeciesAbility(SPECIES_AEGISLASH_SHIELD, ABILITY_NUM_SYNTHETIC), ABILITY_STANCE_CHANGE);
    // ...and evolving keeps the slot: Blissey gets Blissey's
    EXPECT_EQ(GetAbilityBySpecies(SPECIES_BLISSEY, ABILITY_NUM_SYNTHETIC), ABILITY_TRIAGE);
}

TEST("GymHack: Synthesizer: Evil Mode sets friendship to 0, Nice Mode raises it; it won't take a flagged one twice")
{
    u32 friendship = 100;
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHANSEY, 30;
        givemon SPECIES_MIMIKYU, 30;
    );
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP, &friendship);
    gSpecialVar_0x8004 = 0;
    FlagClear(FLAG_BASEMENT_NICE_MODE);
    Synthesizer_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_OK);
    Synthesizer_Apply();
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][0]), ABILITY_TRIAGE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP), 0);
    Synthesizer_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_SYNTH_ALREADY);

    FlagSet(FLAG_BASEMENT_NICE_MODE);
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP, &friendship);
    Synthesizer_Apply();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP), 100 + SYNTH_NICE_FRIENDSHIP);
    FlagClear(FLAG_BASEMENT_NICE_MODE);

    gSpecialVar_0x8004 = 1;
    Synthesizer_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_SYNTH_BLOCKED);
}

TEST("GymHack: Type Ball: Metapod's weakest base stat (Attack 20) counts as 30 in a Bug Ball, not in a Fire Ball")
{
    u32 ball, plainAtk, plainDef;
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_METAPOD, 100;
    );
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    plainAtk = GetMonData(mon, MON_DATA_ATK);
    plainDef = GetMonData(mon, MON_DATA_DEF);
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_METAPOD, BALL_TYPE_BUG), 1u << STAT_ATK);
    ball = BALL_TYPE_BUG;
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
    CalculateMonStats(mon);
    EXPECT_GE(GetMonData(mon, MON_DATA_ATK) - plainAtk, 18); // 2 * 10 more base at Lv. 100, give or take its nature
    EXPECT_LE(GetMonData(mon, MON_DATA_ATK) - plainAtk, 22);
    EXPECT_EQ(GetMonData(mon, MON_DATA_DEF), plainDef);
    ball = BALL_TYPE_FIRE;
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
    CalculateMonStats(mon);
    EXPECT_EQ(GetMonData(mon, MON_DATA_ATK), plainAtk);
    // ties: Chansey's Attack and Defense are both 5
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_CHANSEY, BALL_TYPE_NORMAL), (1u << STAT_ATK) | (1u << STAT_DEF));
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_VENUSAUR, BALL_TYPE_POISON), (1u << STAT_HP) | (1u << STAT_SPEED)); // either type counts
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_SHEDINJA, BALL_TYPE_BUG), (1u << STAT_SPATK) | (1u << STAT_SPDEF)); // never its 1 HP
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_METAPOD, BALL_POKE), 0);
}

TEST("GymHack: Ball Maker: Evil Mode takes the Pokemon, Nice Mode takes 5 levels; Lv. 50 and up only")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARIZARD, 60, ITEM_LEFTOVERS;
        givemon SPECIES_PIDGEY, 49;
        givemon SPECIES_BLASTOISE, 55;
    );
    FlagClear(FLAG_BASEMENT_NICE_MODE);
    gSpecialVar_0x8004 = 1;
    BallMaker_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_MAKER_LOW_LEVEL);

    gSpecialVar_0x8004 = 0;
    BallMaker_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_OK);
    EXPECT_EQ(gSpecialVar_0x8005, ITEM_FIRE_BALL);
    u32 leftovers = CountTotalItemQuantityInBag(ITEM_LEFTOVERS);
    BallMaker_Apply();
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIDGEY);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_LEFTOVERS), leftovers + 1);   // its held item came back

    FlagSet(FLAG_BASEMENT_NICE_MODE);
    gSpecialVar_0x8004 = 1;
    BallMaker_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_OK);
    EXPECT_EQ(gSpecialVar_0x8005, ITEM_WATER_BALL);
    BallMaker_Apply();
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_LEVEL), 50);
    FlagClear(FLAG_BASEMENT_NICE_MODE);

    // Evil Mode won't take your last Pokémon
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_CHARIZARD, 60;);
    gSpecialVar_0x8004 = 0;
    BallMaker_CheckMon();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_MAKER_LAST_MON);
}

TEST("GymHack: Ball Changer moves the Pokemon into the new ball and gives back the old one")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_METAPOD, 100;);
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u32 atk = GetMonData(mon, MON_DATA_ATK);
    u32 poke = CountTotalItemQuantityInBag(ITEM_POKE_BALL);
    AddBagItem(ITEM_BUG_BALL, 1);
    gSpecialVar_0x8004 = 0;
    gSpecialVar_ItemId = ITEM_POKE_BALL;
    BallChanger_CheckBall();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_CHANGER_SAME_BALL);
    gSpecialVar_ItemId = ITEM_BUG_BALL;
    BallChanger_CheckBall();
    EXPECT_EQ(gSpecialVar_Result, MACHINE_OK);
    BallChanger_Apply();
    EXPECT_EQ(GetMonData(mon, MON_DATA_POKEBALL), BALL_TYPE_BUG);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_BUG_BALL), 0);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_POKE_BALL), poke + 1);
    EXPECT_GT(GetMonData(mon, MON_DATA_ATK), atk);   // the Bug Ball's boost
}

TEST("GymHack: the Ability Capsule switches a synthetic ability back to a regular one")
{
    u8 slots[NUM_ABILITY_SLOTS];
    u32 slot = ABILITY_NUM_SYNTHETIC;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_BULBASAUR, 10;);
    SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
    EXPECT_GT(StatItems_GetAbilityChoices(mon, slots), 0);
    EXPECT(StatItems_SetAbilitySlot(mon, 0));
    EXPECT_EQ(GetMonData(mon, MON_DATA_ABILITY_NUM), 0);
    EXPECT_EQ(GetMonAbility(mon), GetAbilityBySpecies(SPECIES_BULBASAUR, 0));
}

#include "dad_story.h"
#include "tv.h"
#include "data.h"
#include "constants/dad_story.h"
#include "constants/outfits.h"
#include "outfits.h"

extern const u8 DadStory_Text_MomSendoff[], DadStory_Text_RivalMomHasSomething[], DadStory_Text_MomDadsBall[],
    DadStory_Text_MomMoveExtractor[], DadStory_Text_GruntInsists[], DadStory_Text_GiovanniSilphGift[],
    DadStory_Text_MomSilphBoy[], DadStory_Text_MomSilphGirl[], DadStory_Text_MomWhyWithHim[], DadStory_Text_MomBasement[],
    DadStory_Text_GiovanniTakeThese[], DadStory_Text_ReceivedEarthBadge[], DadStory_Text_GiovanniYourFather[],
    DadStory_Text_GiovanniRematch[], DadStory_Text_GiovanniFinalDefeat[], DadStory_Text_GiovanniYouLost[],
    DadStory_Text_GiovanniHow[], DadStory_Text_MomLastTime[], DadStory_Text_MomDadsChansey[],
    DadStory_Text_ChanseyGlows[], DadStory_Text_ChanseyUnleashes[], DadStory_Text_GiovanniOverwhelmed[], DadStory_Text_GiovanniGone[],
    DadStory_Text_ReceivedDadsChansey[], DadStory_Text_ReceivedDadsNotebook[], DadStory_Text_ChanseySentToPC[],
    DadStory_Text_Note1[], DadStory_Text_Note2[], DadStory_Text_Note3[], DadStory_Text_Note4[], DadStory_Text_Notebook[],
    RocketHideout_B4F_Text_GiovanniIntro[], RocketHideout_B4F_Text_GiovanniPostBattle[], RocketHideout_B4F_Text_Grunt2Intro[],
    RocketHideout_B1F_Text_Grunt1Intro[], RocketHideout_B1F_Text_Grunt5Intro[], CeladonCity_GameCorner_Text_GruntPostBattle[],
    SaffronCity_Text_ImASecurityGuard[], SilphCo_4F_Text_Grunt2Defeat[], SilphCo_11F_Text_GiovanniIntro[],
    SilphCo_11F_Text_GiovanniPostBattle[], PalletTown_Text_OakDontGoOut[], PalletTown_Text_OakGrassUnsafeNeedMon[],
    MoveExtractor_Text_ThirdVisit[], MoveExtractor_Text_ThirdVisitAfter[];

// Each line of a text (after placeholders) no wider than maxWidth; a page no more than maxLines lines
static void ExpectStoryTextFits(const u8 *text, u32 maxWidth, u32 maxLines)
{
    u8 *buf = Alloc(3000);
    u8 *line, *p;
    u32 lines = 0;
    StringExpandPlaceholders(buf, text);
    for (line = p = buf; ; p++)
    {
        u8 c = *p;
        if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR || c == EOS)
        {
            *p = EOS;
            lines++;
            if (GetStringWidth(FONT_NORMAL, line, 0) > maxWidth)
                DebugPrintf("line too wide: %S", line);
            EXPECT_LE(GetStringWidth(FONT_NORMAL, line, 0), maxWidth);
            if (c == CHAR_PROMPT_CLEAR || c == EOS)
            {
                EXPECT_LE(lines, maxLines);
                lines = 0;
            }
            if (c == EOS)
                break;
            line = p + 1;
        }
    }
    Free(buf);
}

TEST("GymHack: Dad story: every line fits the message box, and Dad's notes fit their pages")
{
    static const u8 *const boxes[] = {
        DadStory_Text_MomSendoff, DadStory_Text_RivalMomHasSomething, DadStory_Text_MomDadsBall,
        DadStory_Text_MomMoveExtractor, DadStory_Text_GruntInsists, DadStory_Text_GiovanniSilphGift,
        DadStory_Text_MomSilphBoy, DadStory_Text_MomSilphGirl, DadStory_Text_MomWhyWithHim, DadStory_Text_MomBasement,
        DadStory_Text_GiovanniTakeThese, DadStory_Text_ReceivedEarthBadge, DadStory_Text_GiovanniYourFather,
        DadStory_Text_GiovanniRematch, DadStory_Text_GiovanniFinalDefeat, DadStory_Text_GiovanniYouLost,
        DadStory_Text_GiovanniHow, DadStory_Text_MomLastTime, DadStory_Text_MomDadsChansey,
        DadStory_Text_ChanseyGlows, DadStory_Text_ChanseyUnleashes, DadStory_Text_GiovanniOverwhelmed, DadStory_Text_GiovanniGone,
        DadStory_Text_ReceivedDadsChansey, DadStory_Text_ReceivedDadsNotebook, DadStory_Text_ChanseySentToPC,
        RocketHideout_B4F_Text_GiovanniIntro, RocketHideout_B4F_Text_GiovanniPostBattle, RocketHideout_B4F_Text_Grunt2Intro,
        RocketHideout_B1F_Text_Grunt1Intro, RocketHideout_B1F_Text_Grunt5Intro, CeladonCity_GameCorner_Text_GruntPostBattle,
        SaffronCity_Text_ImASecurityGuard, SilphCo_4F_Text_Grunt2Defeat, SilphCo_11F_Text_GiovanniIntro,
        SilphCo_11F_Text_GiovanniPostBattle, PalletTown_Text_OakDontGoOut, PalletTown_Text_OakGrassUnsafeNeedMon,
        MoveExtractor_Text_ThirdVisit, MoveExtractor_Text_ThirdVisitAfter,
    };
    static const u8 *const pages[] = {
        DadStory_Text_Note1, DadStory_Text_Note2, DadStory_Text_Note3, DadStory_Text_Note4, DadStory_Text_Notebook,
    };
    u32 i;
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gSaveBlock1Ptr->rivalName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gStringVar1, GetSpeciesName(SPECIES_MEOWSCARADA));   // (the longest of Dad's aces)
    for (i = 0; i < ARRAY_COUNT(boxes); i++)
        ExpectStoryTextFits(boxes[i], 27 * 8 - 4, 99);
    for (i = 0; i < ARRAY_COUNT(pages); i++)
        ExpectStoryTextFits(pages[i], 26 * 8 - 12, 7);
}

TEST("GymHack: Dad story: DAD's CHANSEY is a Lv. 50 trade with Triage, perfect IVs and Attack 100, in a Normal Ball")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU, 5;);
    DadStory_GiveChansey();
    EXPECT_EQ(gSpecialVar_Result, MON_GIVEN_TO_PARTY);
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][1];
    u8 ot[PLAYER_NAME_LENGTH + 1];
    u32 i;
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_CHANSEY_DAD);
    EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), DADS_CHANSEY_LEVEL);
    EXPECT_EQ(GetMonAbility(mon), ABILITY_TRIAGE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_POKEBALL), BALL_TYPE_NORMAL);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HELD_ITEM), ITEM_DADS_NOTEBOOK);
    for (i = 0; i < NUM_STATS; i++)
    {
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_IV + i), MAX_PER_STAT_IVS);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV + i), 0);
    }
    GetMonData(mon, MON_DATA_OT_NAME, ot);
    EXPECT_EQ(StringCompare(ot, COMPOUND_STRING("DAD")), 0);
    EXPECT_NE(GetMonData(mon, MON_DATA_OT_ID), GetPlayerIDAsU32());                     // a trade
    EXPECT_EQ(StringCompare(GetSpeciesName(SPECIES_CHANSEY_DAD), GetSpeciesName(SPECIES_CHANSEY)), 0);
    EXPECT_EQ(GetSpeciesBaseStat(SPECIES_CHANSEY_DAD, STAT_ATK), 100);
    EXPECT_EQ(GetSpeciesBaseStat(SPECIES_CHANSEY_DAD, STAT_SPATK), 100);
    EXPECT_GT(GetMonData(mon, MON_DATA_ATK), 100);
    EXPECT_EQ(GetTypeBallStatBoosts(SPECIES_CHANSEY_DAD, BALL_TYPE_NORMAL), 1u << STAT_DEF);   // its lowest now
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_CHANSEY_DAD), NULL);
}

TEST("GymHack: Dad story: GIOVANNI's last team has their synthetic abilities, each in its own Type Ball")
{
    // (tests have their own trainer table: his team as in trainers_frlg.party)
    static const u16 team[] = {SPECIES_PERSIAN, SPECIES_CROBAT, SPECIES_WEEZING, SPECIES_NIDOQUEEN, SPECIES_RHYPERIOR, SPECIES_NIDOKING};
    u32 i;
    ZeroPartyMons(gParties[B_TRAINER_OPPONENT_A]);
    for (i = 0; i < PARTY_SIZE; i++)
        CreateMon(&gParties[B_TRAINER_OPPONENT_A][i], team[i], 50, 0, OTID_STRUCT_RANDOM_NO_SHINY);
    DadStory_SynthesizeParty(gParties[B_TRAINER_OPPONENT_A]);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        enum Species species = GetMonData(mon, MON_DATA_SPECIES);
        EXPECT_NE(species, SPECIES_NONE);
        EXPECT_EQ(GetMonAbility(mon), GetSpeciesSyntheticAbility(species));
        EXPECT_EQ(GetTypeBallType(GetMonData(mon, MON_DATA_POKEBALL)), GetSpeciesType(species, 0));
    }
}

TEST("GymHack: Dad story: Dad's ace is a starter the player's GYM type hits super effectively")
{
    u32 type;
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        enum Species ace;
        if (type == TYPE_MYSTERY || type == TYPE_STELLAR)
            continue;
        ace = DadStory_GetAce(type);
        EXPECT_NE(ace, SPECIES_NONE);
        if (type == TYPE_NORMAL || type == TYPE_STEEL || type == TYPE_DRAGON)
            continue;   // (no starter is weak to these)
        uq4_12_t mod = uq4_12_multiply(GetTypeModifier(type, GetSpeciesType(ace, 0)),
                                       GetSpeciesType(ace, 1) != GetSpeciesType(ace, 0) ? GetTypeModifier(type, GetSpeciesType(ace, 1)) : UQ_4_12(1.0));
        if (mod <= UQ_4_12(1.0))
            DebugPrintf("type %d: %S isn't weak to it", type, GetSpeciesName(ace));
        EXPECT_GT(mod, UQ_4_12(1.0));
    }
}

TEST("GymHack: Dad story: BRUNO mentors GROUND and AGATHA mentors DARK (not GIOVANNI)")
{
    EXPECT_EQ(StringCompare(GymCandidate_GetMentorName(GymCandidate_GetMentorForType(TYPE_GROUND)->who), COMPOUND_STRING("BRUNO")), 0);
    EXPECT_EQ(StringCompare(GymCandidate_GetMentorName(GymCandidate_GetMentorForType(TYPE_DARK)->who), COMPOUND_STRING("AGATHA")), 0);
}

TEST("GymHack: outfits: each type has its gym uniform, and unlocking keeps the others")
{
    u32 t, before = VarGet(VAR_OUTFITS_UNLOCKED), before2 = VarGet(VAR_OUTFITS_UNLOCKED_2);
    for (t = TYPE_NORMAL; t <= TYPE_FAIRY; t++)
    {
        if (t == TYPE_MYSTERY)
            continue;
        EXPECT_NE(Outfit_ForType(t), OUTFIT_CASUAL);
    }
    EXPECT_EQ(Outfit_ForType(TYPE_WATER), OUTFIT_WATER);
    EXPECT_EQ(Outfit_ForType(TYPE_FAIRY), OUTFIT_FAIRY);
    VarSet(VAR_OUTFITS_UNLOCKED, 0);
    VarSet(VAR_OUTFITS_UNLOCKED_2, 0);
    EXPECT(Outfit_IsUnlocked(OUTFIT_CASUAL));
    EXPECT(!Outfit_IsUnlocked(OUTFIT_ROCKET));
    Outfit_Unlock(OUTFIT_ROCKET);
    Outfit_Unlock(OUTFIT_FAIRY);
    EXPECT(Outfit_IsUnlocked(OUTFIT_ROCKET));
    EXPECT(Outfit_IsUnlocked(OUTFIT_FAIRY));
    EXPECT(!Outfit_IsUnlocked(OUTFIT_DARK));
    VarSet(VAR_OUTFITS_UNLOCKED, before);
    VarSet(VAR_OUTFITS_UNLOCKED_2, before2);
}

TEST("GymHack: outfits: changing the gym's type gives every gym uniform (and saves that already changed catch up)")
{
    u32 t, before = VarGet(VAR_OUTFITS_UNLOCKED), before2 = VarGet(VAR_OUTFITS_UNLOCKED_2);
    VarSet(VAR_OUTFITS_UNLOCKED, 0);
    VarSet(VAR_OUTFITS_UNLOCKED_2, 0);
    Outfit_UnlockAllGymUniforms();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    for (t = OUTFIT_NORMAL; t < OUTFIT_COUNT; t++)
        EXPECT(Outfit_IsUnlocked(t));
    EXPECT(!Outfit_IsUnlocked(OUTFIT_ROCKET));
    Outfit_UnlockAllGymUniforms();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    VarSet(VAR_OUTFITS_UNLOCKED, 0);
    VarSet(VAR_OUTFITS_UNLOCKED_2, 0);
    FlagSet(FLAG_GYM_TYPE_OWNED + TYPE_FIRE);
    Outfit_CatchUp();
    EXPECT(Outfit_IsUnlocked(OUTFIT_DRAGON));
    FlagClear(FLAG_GYM_TYPE_OWNED + TYPE_FIRE);
    VarSet(VAR_OUTFITS_UNLOCKED, before);
    VarSet(VAR_OUTFITS_UNLOCKED_2, before2);
}

// Pokémon Canvas: the new eeveelutions (src/data/pokemon/species_info/canvas_families.h)
TEST("GymHack: eeveelutions: Eevee becomes each new one by levelling up holding its type's item")
{
    static const u16 evo[][3] = {
        {ITEM_SILK_SCARF, SPECIES_ODEON, TYPE_NORMAL},          {ITEM_BLACK_BELT, SPECIES_ASSASSEON, TYPE_FIGHTING},
        {ITEM_SHARP_BEAK, SPECIES_HYPEREON, TYPE_FLYING},       {ITEM_POISON_BARB, SPECIES_POISEON, TYPE_POISON},
        {ITEM_SOFT_SAND, SPECIES_SUBTERRANEON, TYPE_GROUND},    {ITEM_HARD_STONE, SPECIES_OBSIDEON, TYPE_ROCK},
        {ITEM_SILVER_POWDER, SPECIES_LENNEON, TYPE_BUG},        {ITEM_SPELL_TAG, SPECIES_APPARITEON, TYPE_GHOST},
        {ITEM_METAL_COAT, SPECIES_AUTOMATEON, TYPE_STEEL},      {ITEM_DRAGON_FANG, SPECIES_VALYREON, TYPE_DRAGON},
    };
    struct Pokemon mon;
    u32 i, item;
    for (i = 0; i < ARRAY_COUNT(evo); i++)
    {
        const struct SpeciesInfo *info = &gSpeciesInfo[evo[i][1]];
        CreateMon(&mon, SPECIES_EEVEE, 20, 0, OTID_STRUCT_PLAYER_ID);
        item = evo[i][0];
        SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
        EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), evo[i][1]);
        EXPECT_EQ((u32)info->types[0], (u32)evo[i][2]);
        EXPECT_EQ((u32)(info->baseHP + info->baseAttack + info->baseDefense + info->baseSpAttack + info->baseSpDefense + info->baseSpeed), 525);
        EXPECT((u32)info->natDexNum != NATIONAL_DEX_NONE);
        EXPECT(info->levelUpLearnset[0].move != MOVE_NONE);
        EXPECT(info->teachableLearnset[0] != MOVE_UNAVAILABLE);
        EXPECT(GetSpeciesSyntheticAbility(evo[i][1]) != ABILITY_NONE);
    }
    // (holding nothing it stays an Eevee)
    CreateMon(&mon, SPECIES_EEVEE, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_NONE);
}

TEST("GymHack: eeveelutions: the Pokédex keeps seen and caught for every new number")
{
    u32 n;
    for (n = NATIONAL_DEX_ODEON; n <= NATIONAL_DEX_VALYREON; n++)
    {
        EXPECT(!GetSetPokedexFlag(n, FLAG_GET_SEEN));
        GetSetPokedexFlag(n, FLAG_SET_SEEN);
        EXPECT(GetSetPokedexFlag(n, FLAG_GET_SEEN));
        EXPECT(!GetSetPokedexFlag(n, FLAG_GET_CAUGHT));
        GetSetPokedexFlag(n, FLAG_SET_CAUGHT);
        EXPECT(GetSetPokedexFlag(n, FLAG_GET_CAUGHT));
        EXPECT(!GetSetPokedexFlag(n + 1 <= NATIONAL_DEX_VALYREON ? n + 1 : NATIONAL_DEX_ODEON, FLAG_GET_CAUGHT) || n == NATIONAL_DEX_VALYREON);
    }
    EXPECT_EQ(SpeciesToNationalPokedexNum(SPECIES_VALYREON), NATIONAL_DEX_VALYREON);
}

#include "penny.h"
TEST("GymHack: Penny: every type has an Eevee evolution, and the item really evolves it")
{
    u32 type;
    struct Pokemon mon;
    for (type = TYPE_NORMAL; type < NUMBER_OF_MON_TYPES; type++)
    {
        u32 item = Penny_GetItem(type), species = Penny_GetSpecies(type);
        if (type == TYPE_MYSTERY || type == TYPE_STELLAR)
            continue;
        EXPECT_EQ((u32)gSpeciesInfo[species].types[0], type);
        CreateMon(&mon, SPECIES_EEVEE, 30, 0, OTID_STRUCT_PLAYER_ID);
        if (type == TYPE_FAIRY)
        {
            EXPECT_EQ(item, ITEM_NONE);
            continue;
        }
        if (GetItemPocket(item) == POCKET_HOLD_ITEMS)
        {
            SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
            EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), species);
        }
        else
        {
            EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, item, NULL, NULL, CHECK_EVO), species);
        }
    }
}

extern const u8 ViridianCity_PennysHouse_Text_Intro[], ViridianCity_PennysHouse_Text_StillLooking[],
    ViridianCity_PennysHouse_Text_YouFoundOne[], ViridianCity_PennysHouse_Text_ForThisType[],
    ViridianCity_PennysHouse_Text_HowHold[], ViridianCity_PennysHouse_Text_HowUse[], ViridianCity_PennysHouse_Text_HowFairy[],
    ViridianCity_PennysHouse_Text_BagFull[], ViridianCity_PennysHouse_Text_ComeBackLater[], ViridianCity_PennysHouse_Text_NotYet[],
    ViridianCity_PennysHouse_Text_BringAnother[], ViridianCity_PennysHouse_Text_AnotherEevee[],
    ViridianCity_PennysHouse_Text_Already[], ViridianCity_PennysHouse_Text_Plush[], ViridianCity_PennysHouse_Text_PlushNew[];

TEST("GymHack: Penny: her lines fit the message box")
{
    static const u8 *const texts[] = {
        ViridianCity_PennysHouse_Text_Intro, ViridianCity_PennysHouse_Text_StillLooking, ViridianCity_PennysHouse_Text_YouFoundOne,
        ViridianCity_PennysHouse_Text_ForThisType, ViridianCity_PennysHouse_Text_HowHold, ViridianCity_PennysHouse_Text_HowUse,
        ViridianCity_PennysHouse_Text_HowFairy, ViridianCity_PennysHouse_Text_BagFull, ViridianCity_PennysHouse_Text_ComeBackLater,
        ViridianCity_PennysHouse_Text_NotYet, ViridianCity_PennysHouse_Text_BringAnother, ViridianCity_PennysHouse_Text_AnotherEevee,
        ViridianCity_PennysHouse_Text_Already, ViridianCity_PennysHouse_Text_Plush, ViridianCity_PennysHouse_Text_PlushNew,
    };
    u8 *buf = Alloc(1000);
    u32 i;
    StringCopy(gStringVar1, COMPOUND_STRING("Fighting"));
    StringCopy(gStringVar2, COMPOUND_STRING("Silver Powder"));
    StringCopy(gStringVar3, COMPOUND_STRING("Subterraneon"));
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

#include "battle.h"
// Wild holders (src/pokemon.c SetWildMonHeldItem): a clover or an old coin (item_upgrades.c), else
// the type's Tera berry (berry_field.c), at about their rates
static void CountWildHeldItems(u32 species, u32 n, u16 *items, u32 *counts, u32 numItems)
{
    u32 i, k;
    u16 none = ITEM_NONE;
    ZeroEnemyPartyMons();
    CreateMon(&gParties[B_TRAINER_OPPONENT_A][0], species, 20, 0, OTID_STRUCT_RANDOM_NO_SHINY);
    for (i = 0; i < n; i++)
    {
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM, &none);
        SetWildMonHeldItem();
        for (k = 0; k < numItems; k++)
            if (GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM) == items[k])
                counts[k]++;
    }
}

TEST("GymHack: wild Pokemon hold clovers, coins and Tera berries at about their rates")
{
    u16 meowth[] = {ITEM_COPPER_COIN, ITEM_CHILAN_BERRY};
    u16 persian[] = {ITEM_SILVER_COIN, ITEM_GOLD_COIN};
    u16 chansey[] = {ITEM_FOUR_LEAF_CLOVER, ITEM_GOLDEN_CLOVER};
    u16 oddish[] = {ITEM_LUCKY_CLOVER};
    u32 c[2];
    gBattleTypeFlags = 0;
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_RATTATA, 20, 0, OTID_STRUCT_RANDOM_NO_SHINY);   // (no Compound Eyes)
    c[0] = c[1] = 0;
    CountWildHeldItems(SPECIES_MEOWTH, 4000, meowth, c, 2);          // 5%, then 5% of the rest
    EXPECT_GE(c[0], 120); EXPECT_LE(c[0], 290);
    EXPECT_GE(c[1], 110); EXPECT_LE(c[1], 280);
    c[0] = c[1] = 0;
    CountWildHeldItems(SPECIES_PERSIAN, 4000, persian, c, 2);        // 5% silver, 1% gold
    EXPECT_GE(c[0], 120); EXPECT_LE(c[0], 290);
    EXPECT_GE(c[1], 15); EXPECT_LE(c[1], 75);
    c[0] = c[1] = 0;
    CountWildHeldItems(SPECIES_CHANSEY, 4000, chansey, c, 2);        // 5% four-leaf, 1% golden
    EXPECT_GE(c[0], 120); EXPECT_LE(c[0], 290);
    EXPECT_GE(c[1], 15); EXPECT_LE(c[1], 75);
    c[0] = 0;
    CountWildHeldItems(SPECIES_ODDISH, 4000, oddish, c, 1);          // 5% of what the lab's Strong Acid
    EXPECT_GE(c[0], 60); EXPECT_LE(c[0], 200);                         //   and Apple Juice leave (about 3%)
}

// Every item an evolution is used with works from the bag: the party menu (ExecuteTableBasedItemEffect,
// CheckIfItemIsTMHMOrEvolutionStone) needs the item's EvoItem effect. (The Sun and Moon Shards had none.)
TEST("GymHack: every evolution item can be used from the bag")
{
    u32 species, i;
    for (species = 1; species < NUM_SPECIES; species++)
    {
        const struct Evolution *evos;
        if (!IsSpeciesEnabled(species) || (evos = GetSpeciesEvolutions(species)) == NULL)
            continue;
        for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            if (evos[i].method != EVO_ITEM || !IsSpeciesEnabled(evos[i].targetSpecies))
                continue;
            if (CheckIfItemIsTMHMOrEvolutionStone(evos[i].param) != ITEM_IS_EVOLUTION_STONE)
                DebugPrintf("species %d: item %d has no EvoItem effect", species, evos[i].param);
            EXPECT_EQ(CheckIfItemIsTMHMOrEvolutionStone(evos[i].param), ITEM_IS_EVOLUTION_STONE);
        }
    }
}

TEST("GymHack: the Sun and Moon Shards work on Eevee from the bag")
{
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_EEVEE, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(CheckIfItemIsTMHMOrEvolutionStone(ITEM_SUN_SHARD), ITEM_IS_EVOLUTION_STONE);
    EXPECT_EQ(GetEvolutionTargetSpecies(&gParties[B_TRAINER_PLAYER][0], EVO_MODE_ITEM_CHECK, ITEM_SUN_SHARD, NULL, NULL, CHECK_EVO), SPECIES_ESPEON);
    EXPECT_EQ(CheckIfItemIsTMHMOrEvolutionStone(ITEM_MOON_SHARD), ITEM_IS_EVOLUTION_STONE);
    EXPECT_EQ(GetEvolutionTargetSpecies(&gParties[B_TRAINER_PLAYER][0], EVO_MODE_ITEM_CHECK, ITEM_MOON_SHARD, NULL, NULL, CHECK_EVO), SPECIES_UMBREON);
}

// The hack's own vars and flags never share a number (VAR_ACADEMY_STATE and the outfits did: the
// Mega Bracelet came early). Each list: every custom one, by its FireRed number.
TEST("GymHack: the hack's vars don't share numbers")
{
    static const u16 vars[] =
    {
        VAR_PLAYER_TYPE, VAR_GAUNTLET_STATE, VAR_GAUNTLET_NEXT, VAR_GAUNTLET_WITHDRAWS, VAR_ACADEMY_STATE,
        VAR_RIVAL_TYPE, VAR_MAP_SCENE_PALLET_TOWN_GYM, VAR_GYM_FINALE, VAR_MOVE_EXTRACTOR_STAGE, VAR_MENTOR_STAGE,
        VAR_GYM_DECOR_SEEN, VAR_GYM_LEGEND_STATE, VAR_JOHTO_STATE, VAR_EVENT_CAPS, VAR_EVENT_VIVILLON_LO,
        VAR_EVENT_VIVILLON_HI, VAR_LUCKY_EGG_LEVEL, VAR_AMULET_COIN_LEVEL, VAR_CHAMPION_LOSSES, VAR_INCENSE_STEPS,
        VAR_OUTFITS_UNLOCKED, VAR_OUTFITS_UNLOCKED_2, VAR_PLAYER_OUTFIT, VAR_DAD_STORY, VAR_SURFING_HIGH_SCORE,
        VAR_PAINTER_SKETCHES, VAR_PAINTING_DEST, VAR_PAINTING_MODE, VAR_PAINTING_RESULT, VAR_PAINTING_RETURN,
        VAR_PAINTER_SKETCH, VAR_PAINTER_SKETCH_MOVE, VAR_PAINTER_MON_LO, VAR_PAINTER_MON_HI, VAR_PAINTING_SEED,
    };
    u32 i, j;
    for (i = 0; i < ARRAY_COUNT(vars); i++)
        for (j = 0; j < i; j++)
            EXPECT_NE(vars[i], vars[j]);
}

TEST("GymHack: an old save's outfits move out of the Academy's var (no early Mega Bracelet)")
{
    // (v135: Dad's uniform found after being admitted made the shared var 4 | 1 = 5, the bracelet's call;
    // the call played and gave the bracelet before the 8th badge)
    FlagClear(FLAG_OUTFIT_VARS_MOVED);
    FlagClear(FLAG_BADGE08_GET);
    FlagSet(FLAG_WORLD_MAP_LEADER_ACADEMY);
    FlagSet(FLAG_HIDE_ACADEMY_GATE);
    FlagSet(FLAG_GOT_MEGA_BRACELET);
    AddBagItem(ITEM_MEGA_RING, 1);
    VarSet(VAR_0x40E5, ACADEMY_ADMITTED | (1 << (OUTFIT_FIRE - 1)));
    VarSet(VAR_OUTFITS_UNLOCKED, 0);
    VarSet(VAR_OUTFITS_UNLOCKED_2, 0);
    VarSet(VAR_MENTOR_STAGE, MENTOR_GAVE_GUARD);
    VarSet(VAR_PLAYER_TYPE, TYPE_DRAGON);
    VarSet(VAR_EVENT_CAPS, 1 << 0);   // (cap 0 "found": really the Dragon uniform)
    Outfit_FixOldSave();
    EXPECT_EQ(VarGet(VAR_ACADEMY_STATE), ACADEMY_ADMITTED);
    EXPECT(!FlagGet(FLAG_GOT_MEGA_BRACELET));
    EXPECT(!CheckBagHasItem(ITEM_MEGA_RING, 1));
    EXPECT(Outfit_IsUnlocked(OUTFIT_FIRE));
    EXPECT(Outfit_IsUnlocked(OUTFIT_DRAGON));
    EXPECT_EQ(VarGet(VAR_EVENT_CAPS), 0);
    EXPECT(FlagGet(FLAG_OUTFIT_VARS_MOVED));
    // ...and only once
    VarSet(VAR_ACADEMY_STATE, ACADEMY_BRACELET_CALL);
    Outfit_FixOldSave();
    EXPECT_EQ(VarGet(VAR_ACADEMY_STATE), ACADEMY_BRACELET_CALL);
    // Invited, then the Normal uniform (outfit 2, bit 1): 1 | 2 = 3 would skip the escort; the
    // invitation comes again instead
    FlagClear(FLAG_OUTFIT_VARS_MOVED);
    FlagClear(FLAG_HIDE_ACADEMY_GATE);
    FlagClear(FLAG_WORLD_MAP_LEADER_ACADEMY);
    VarSet(VAR_0x40E5, ACADEMY_INVITE | (1 << (OUTFIT_NORMAL - 1)));
    Outfit_FixOldSave();
    EXPECT_EQ(VarGet(VAR_ACADEMY_STATE), ACADEMY_NONE);
}

TEST("GymHack: the Move Extractor shows how many there are (it's used up)")
{
    EXPECT_EQ(GetItemImportance(ITEM_MOVE_EXTRACTOR), 0);
}

// Surfing Pikachu (src/surfing_pikachu.c): the Stadium trainer's rewards
#include "surfing_pikachu.h"
extern const u8 Route21_StadiumCave_Text_Intro[], Route21_StadiumCave_Text_Again[], Route21_StadiumCave_Text_AgainAllGiven[],
    Route21_StadiumCave_Text_Battle[], Route21_StadiumCave_Text_Start[], Route21_StadiumCave_Text_NotNow[],
    Route21_StadiumCave_Text_Defeat[], Route21_StadiumCave_Text_BringPikachu[], Route21_StadiumCave_Text_WonNothingLeft[],
    Route21_StadiumCave_Text_WithPikachu[], Route21_StadiumCave_Text_OnlyPikachu[], Route21_StadiumCave_Text_Received[],
    Route21_StadiumCave_Text_ReceivedShiny[], Route21_StadiumCave_Text_SentToPC[], Route21_StadiumCave_Text_TakeCare[],
    Route21_StadiumCave_Text_NoRoom[], Route21_StadiumCave_Text_GoldDiglett[],
    Route19_BeachHouse_Text_NoSurfer[], Route19_BeachHouse_Text_WhoaASurfer[], Route19_BeachHouse_Text_Again[],
    Route19_BeachHouse_Text_HangTen[], Route19_BeachHouse_Text_After[], Route19_BeachHouse_Text_NotToday[];
void SurfingPikachu_CheckParty(void);
void SurfingPikachu_Owed(void);
void SurfingPikachu_Give(void);

static void PartyOf(const u16 *species, u32 n)
{
    u32 i;
    ZeroPlayerPartyMons();
    for (i = 0; i < n; i++)
        CreateMon(&gParties[B_TRAINER_PLAYER][i], species[i], 50, 0, OTID_STRUCT_PLAYER_ID);
    gPartiesCount[B_TRAINER_PLAYER] = n;
}

TEST("GymHack: Surfing Pikachu: some Pikachu earn it, nothing but Pikachu the shiny one, one of each")
{
    static const u16 sNone[] = {SPECIES_RATTATA, SPECIES_PIDGEY};
    static const u16 sSome[] = {SPECIES_RATTATA, SPECIES_PIKACHU};
    static const u16 sAll[] = {SPECIES_PIKACHU, SPECIES_PIKACHU_PARTNER, SPECIES_PIKACHU_SURFING};
    struct Pokemon *mon;
    FlagClear(FLAG_STADIUM_GAVE_SURFING); FlagClear(FLAG_STADIUM_GAVE_SHINY);
    FlagClear(FLAG_STADIUM_OWES_SURFING); FlagClear(FLAG_STADIUM_OWES_SHINY);

    PartyOf(sNone, 2);
    SurfingPikachu_CheckParty();
    EXPECT_EQ(gSpecialVar_Result, STADIUM_PARTY_NO_PIKACHU);
    SurfingPikachu_Owed();
    EXPECT_EQ(gSpecialVar_Result, 0);

    PartyOf(sAll, 3);
    SurfingPikachu_CheckParty();
    EXPECT_EQ(gSpecialVar_Result, STADIUM_PARTY_ALL_PIKACHU);
    SurfingPikachu_Owed();
    EXPECT_EQ(gSpecialVar_Result, 2);
    SurfingPikachu_Give();
    EXPECT_EQ(gSpecialVar_Result, MON_GIVEN_TO_PARTY);
    EXPECT_EQ(gSpecialVar_0x8004, TRUE);
    mon = &gParties[B_TRAINER_PLAYER][3];
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_PIKACHU_SURFING);
    EXPECT(GetMonData(mon, MON_DATA_IS_SHINY));
    EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1), MOVE_SURF);
    EXPECT(FlagGet(FLAG_STADIUM_GAVE_SHINY));
    SurfingPikachu_Owed();
    EXPECT_EQ(gSpecialVar_Result, 0);

    // nothing but Pikachu again: the shiny one's been given, so the plain one
    PartyOf(sAll, 3);
    SurfingPikachu_CheckParty();
    SurfingPikachu_Owed();
    EXPECT_EQ(gSpecialVar_Result, 1);
    SurfingPikachu_Give();
    EXPECT_EQ(gSpecialVar_0x8004, FALSE);
    EXPECT(!GetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_IS_SHINY));
    EXPECT(FlagGet(FLAG_STADIUM_GAVE_SURFING));

    // and then nothing more
    PartyOf(sSome, 2);
    SurfingPikachu_CheckParty();
    EXPECT_EQ(gSpecialVar_Result, STADIUM_PARTY_SOME_PIKACHU);
    SurfingPikachu_Owed();
    EXPECT_EQ(gSpecialVar_Result, 0);
}

TEST("GymHack: Surfing Pikachu: not from Bill's Palm Pilot, and it knows Surf's in its own species")
{
    u32 i;
    for (i = 0; i < DEX_FORM_COUNT; i++)
        EXPECT_NE(gDexFormSpecies[i], SPECIES_PIKACHU_SURFING);
    EXPECT(SurfingPikachu_IsPikachu(SPECIES_PIKACHU_SURFING));
    EXPECT(SurfingPikachu_IsPikachu(SPECIES_PIKACHU_PARTNER));
    EXPECT(!SurfingPikachu_IsPikachu(SPECIES_RAICHU));
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_PIKACHU_SURFING), NULL);
}

TEST("GymHack: Surfing Pikachu: the Stadium trainer's and the Surfin' Dude's lines fit the message box")
{
    static const u8 *const texts[] = {
        Route21_StadiumCave_Text_Intro, Route21_StadiumCave_Text_Again, Route21_StadiumCave_Text_AgainAllGiven,
        Route21_StadiumCave_Text_Battle, Route21_StadiumCave_Text_Start, Route21_StadiumCave_Text_NotNow,
        Route21_StadiumCave_Text_Defeat, Route21_StadiumCave_Text_BringPikachu, Route21_StadiumCave_Text_WonNothingLeft,
        Route21_StadiumCave_Text_WithPikachu, Route21_StadiumCave_Text_OnlyPikachu, Route21_StadiumCave_Text_Received,
        Route21_StadiumCave_Text_ReceivedShiny, Route21_StadiumCave_Text_SentToPC, Route21_StadiumCave_Text_TakeCare,
        Route21_StadiumCave_Text_NoRoom, Route21_StadiumCave_Text_GoldDiglett,
        Route19_BeachHouse_Text_NoSurfer, Route19_BeachHouse_Text_WhoaASurfer, Route19_BeachHouse_Text_Again,
        Route19_BeachHouse_Text_HangTen, Route19_BeachHouse_Text_After, Route19_BeachHouse_Text_NotToday,
    };
    u32 i;
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gStringVar1, COMPOUND_STRING("9999"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectStoryTextFits(texts[i], 27 * 8 - 4, 99);
}

// ---- Canvas Smeargle and the painter (src/painter.c) ----
static void Painter_ResetForTest(void)
{
    u32 i;
    for (i = 0; i < NUM_BADGES; i++)
        FlagClear(FLAG_BADGE01_GET + i);
    FlagClear(FLAG_SYS_GAME_CLEAR);
    FlagClear(FLAG_PAINTER_GAVE_SMEARGLE);
    FlagClear(FLAG_PAINTER_MEGA_DONE);
    FlagClear(FLAG_IN_PAINTING);
    VarSet(VAR_PAINTER_SKETCHES, 0);
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_NONE);
    VarSet(VAR_PAINTING_RESULT, 0);
    VarSet(VAR_PAINTER_SKETCH, 0);
    ZeroPlayerPartyMons();
}

TEST("GymHack: Canvas Smeargle: 500 total (600 Mega), fast and special, typed by its paint")
{
    u32 t;
    const struct SpeciesInfo *s = &gSpeciesInfo[SPECIES_SMEARGLE_CANVAS];
    EXPECT_EQ(s->baseHP + s->baseAttack + s->baseDefense + s->baseSpeed + s->baseSpAttack + s->baseSpDefense, 500);
    EXPECT_EQ(s->types[0], TYPE_MYSTERY);
    EXPECT_EQ(s->abilities[0], ABILITY_SERENE_GRACE);
    EXPECT_EQ(s->abilities[2], ABILITY_TECHNICIAN);
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        u32 sp;
        if (t == TYPE_MYSTERY || t == TYPE_NONE || t == TYPE_STELLAR)
            continue;
        sp = Painter_SmeargleOfType(t);
        EXPECT_NE(sp, SPECIES_SMEARGLE_CANVAS);
        EXPECT(Painter_IsCanvasSmeargle(sp));
        EXPECT_EQ(gSpeciesInfo[sp].types[0], t);
        EXPECT_EQ(gSpeciesInfo[sp].types[1], t);
        EXPECT_EQ(GetSpeciesSyntheticAbility(sp), ABILITY_ADAPTABILITY);
    }
    s = &gSpeciesInfo[SPECIES_SMEARGLE_CANVAS_FIRE_MEGA];
    EXPECT_EQ(s->baseHP + s->baseAttack + s->baseDefense + s->baseSpeed + s->baseSpAttack + s->baseSpDefense, 600);
    EXPECT_EQ(s->abilities[0], ABILITY_PROTEAN);
    EXPECT_EQ(GetMovePower(MOVE_MASTERSTROKE), 60);
    EXPECT_EQ(GetMoveCategory(MOVE_MASTERSTROKE), DAMAGE_CATEGORY_SPECIAL);
    EXPECT_EQ(Painter_SmeargleOfType(TYPE_MYSTERY), SPECIES_SMEARGLE_CANVAS);
}

TEST("GymHack: the painter: a badge first, then her Smeargle; a Sketch quest once a badge; the Mega after the League")
{
    Painter_ResetForTest();
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_NO_BADGE);
    FlagSet(FLAG_BADGE01_GET);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_FIRST);
    FlagSet(FLAG_PAINTER_GAVE_SMEARGLE);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_NOTHING);       // (one badge: the gift was its painting)
    FlagSet(FLAG_BADGE02_GET);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_NEED_SMEARGLE); // (not in the party)
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_SMEARGLE_CANVAS_FIRE, 20, 0, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_SKETCH);
    VarSet(VAR_PAINTER_SKETCHES, 1);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_NOTHING);
    FlagSet(FLAG_BADGE03_GET);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_SKETCH);
    VarSet(VAR_PAINTER_SKETCHES, 2);
    FlagSet(FLAG_SYS_GAME_CLEAR);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_MEGA);
    FlagSet(FLAG_BADGE04_GET);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_BOTH);
    FlagSet(FLAG_PAINTER_MEGA_DONE);
    VarSet(VAR_PAINTER_SKETCHES, 50);
    EXPECT_EQ(Painter_Offer(), PAINTER_OFFER_SKETCH);         // (unlimited after both)
    Painter_ResetForTest();
}

static void Painter_StartSketchForTest(u32 oldMove)
{
    u32 move = oldMove;
    Painter_ResetForTest();
    FlagSet(FLAG_BADGE01_GET);
    FlagSet(FLAG_BADGE02_GET);
    FlagSet(FLAG_PAINTER_GAVE_SMEARGLE);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_PIKACHU, 20, 0, OTID_STRUCT_PLAYER_ID);
    CreateMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_SMEARGLE_CANVAS_FIRE, 20, 1234, OTID_STRUCT_PLAYER_ID);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2, &move);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 1;
    gSpecialVar_0x8005 = 1;
    gSpecialVar_0x8006 = 1;     // (FIRE in the script's list)
    Painter_PrepareSketch();
    // (as Painter_Enter does, without the warp)
    VarSet(VAR_PAINTER_SKETCH_MOVE, oldMove);
    move = MOVE_QUICK_SKETCH;
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2, &move);
    FlagSet(FLAG_IN_PAINTING);
}

TEST("GymHack: a Sketch quest: won, the move stays; lost, or a move from outside the quest, the old one's back")
{
    u32 move;
    Painter_StartSketchForTest(MOVE_TACKLE);
    EXPECT(Painter_SketchAllowed(MOVE_EMBER));
    EXPECT(!Painter_SketchAllowed(MOVE_THUNDERBOLT));
    move = MOVE_EMBER;
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2, &move);
    VarSet(VAR_PAINTING_RESULT, 1);
    Painter_Return();
    EXPECT_EQ(gSpecialVar_Result, PAINTER_RETURN_SKETCHED);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2), MOVE_EMBER);
    EXPECT_EQ(VarGet(VAR_PAINTER_SKETCHES), 1);
    EXPECT(!FlagGet(FLAG_IN_PAINTING));

    Painter_StartSketchForTest(MOVE_TACKLE);
    move = MOVE_EMBER;
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2, &move);
    VarSet(VAR_PAINTING_RESULT, 2);
    Painter_Return();
    EXPECT_EQ(gSpecialVar_Result, PAINTER_RETURN_SKETCH_LOST);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2), MOVE_TACKLE);
    EXPECT_EQ(VarGet(VAR_PAINTER_SKETCHES), 0);       // (a lost one doesn't use the badge's go)

    Painter_StartSketchForTest(MOVE_TACKLE);
    VarSet(VAR_PAINTING_RESULT, 1);
    Painter_Return();
    EXPECT_EQ(gSpecialVar_Result, PAINTER_RETURN_NO_SKETCH);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2), MOVE_TACKLE);

    Painter_StartSketchForTest(MOVE_TACKLE);
    VarSet(VAR_PAINTING_RESULT, 0);                    // (left without the battle)
    Painter_Return();
    EXPECT_EQ(gSpecialVar_Result, PAINTER_RETURN_LEFT);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE2), MOVE_TACKLE);
    Painter_ResetForTest();
}

TEST("GymHack: every Sketch quest move exists, and the figure only knows moves of the quest's type")
{
    Painter_ResetForTest();
    FlagSet(FLAG_SYS_GAME_CLEAR);
    VarSet(VAR_PAINTER_SKETCH, 0 | (TYPE_ICE << 8));
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_SKETCH);
    FlagSet(FLAG_IN_PAINTING);
    EXPECT(Painter_SketchAllowed(MOVE_ICE_BEAM));
    EXPECT(!Painter_SketchAllowed(MOVE_FLAMETHROWER));
    EXPECT(!Painter_SketchAllowed(MOVE_SKETCH));
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_MEGA);
    EXPECT(Painter_SketchAllowed(MOVE_FLAMETHROWER));   // (outside a Sketch quest, Sketch as usual)
    Painter_ResetForTest();
}

TEST("GymHack: the lent Smeargle goes back after the first painting")
{
    struct Pokemon *mon;
    Painter_ResetForTest();
    FlagSet(FLAG_BADGE01_GET);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_PIKACHU, 20, 0, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_FIRST);
    mon = AllocZeroed(sizeof(struct Pokemon));
    CreateRandomMon(mon, SPECIES_SMEARGLE_CANVAS, CANVAS_SMEARGLE_LEVEL);
    VarSet(VAR_PAINTER_MON_LO, GetMonData(mon, MON_DATA_PERSONALITY) & 0xFFFF);
    VarSet(VAR_PAINTER_MON_HI, GetMonData(mon, MON_DATA_PERSONALITY) >> 16);
    GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    FlagSet(FLAG_IN_PAINTING);
    VarSet(VAR_PAINTING_RESULT, 1);
    Painter_Return();
    EXPECT_EQ(gSpecialVar_Result, PAINTER_RETURN_FIRST_WON);
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU);
    VarSet(VAR_PLAYER_TYPE, TYPE_WATER);
    Painter_GiveSmeargle();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_SMEARGLE_CANVAS_WATER);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_LEVEL), CANVAS_SMEARGLE_LEVEL);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE1), MOVE_MASTERSTROKE);
    EXPECT_NE(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MAX_HP), 0);
    EXPECT(FlagGet(FLAG_PAINTER_GAVE_SMEARGLE));
    Painter_ResetForTest();
}

TEST("GymHack: bag: an older save's Smearglite moves into the Mega Stones pocket")
{
    ClearBag();
    gSaveBlock1Ptr->bag.keyItems[0] = (struct ItemSlot) {ITEM_SMEARGLITE, 1 ^ gSaveBlock2Ptr->encryptionKey};
    Bag_UpgradeLayout();
    EXPECT(CheckBagHasItem(ITEM_SMEARGLITE, 1));
    EXPECT_EQ(gSaveBlock1Ptr->bag.keyItems[0].itemId, ITEM_NONE);
    // (Hold Items' old third slot is the Mega Stones' extra one now: what's there that isn't a stone goes back)
    ClearBag();
    gSaveBlock2Ptr->bagMegaStones2[0] = (struct ItemSlot) {ITEM_LEFTOVERS, 2 ^ gSaveBlock2Ptr->encryptionKey};
    Bag_UpgradeLayout();
    EXPECT(CheckBagHasItem(ITEM_LEFTOVERS, 2));
    EXPECT_EQ(gSaveBlock2Ptr->bagMegaStones2[0].itemId, ITEM_NONE);
    ClearBag();
}

TEST("GymHack: Mega Smeargle needs to hold the SMEARGLITE, like any Mega Stone")
{
    struct FormChangeContext ctx = {.method = FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM, .currentSpecies = SPECIES_SMEARGLE_CANVAS_FIRE};
    AddBagItem(ITEM_SMEARGLITE, 1);
    EXPECT_EQ(GetFormChangeTargetSpecies_Internal(ctx), SPECIES_SMEARGLE_CANVAS_FIRE);   // (in the bag: no)
    RemoveBagItem(ITEM_SMEARGLITE, 99);
    ctx.heldItem = ITEM_SMEARGLITE;
    EXPECT_EQ(GetFormChangeTargetSpecies_Internal(ctx), SPECIES_SMEARGLE_CANVAS_FIRE_MEGA);
    EXPECT_EQ(GetItemPocket(ITEM_SMEARGLITE), POCKET_MEGA_STONES);
    EXPECT_EQ(GetItemHoldEffect(ITEM_SMEARGLITE), HOLD_EFFECT_MEGA_STONE);
}

extern const u8 PainterLoft_Text_NoBadge[], PainterLoft_Text_FirstIntro[], PainterLoft_Text_FirstAsk[], PainterLoft_Text_FirstGo[], PainterLoft_Text_NoRoom[], PainterLoft_Text_Later[], PainterLoft_Text_Ready[], PainterLoft_Text_NeedSmeargle[], PainterLoft_Text_TooFew[], PainterLoft_Text_Nothing[], PainterLoft_Text_SketchIntro[], PainterLoft_Text_SketchAsk[], PainterLoft_Text_WhichType[], PainterLoft_Text_WhichSlot[], PainterLoft_Text_ConfirmSlot[], PainterLoft_Text_SketchGo[], PainterLoft_Text_MegaIntro[], PainterLoft_Text_MegaAsk[], PainterLoft_Text_MegaGo[], PainterLoft_Text_BothAsk[], PainterLoft_Text_EaselBlank[], PainterLoft_Text_EaselAsk[], PainterLoft_Text_Lend[], PainterLoft_Text_Borrowed[], PainterLoft_Text_SteppingIn[], PainterLoft_Text_Smeargle[], PainterLoft_Text_Canvases[], PainterLoft_Text_Paint[], PainterLoft_Text_PaintPot[], PainterLoft_Text_CameBackEarly[], PainterLoft_Text_FirstWon[], PainterLoft_Text_ReceivedSmeargle[], PainterLoft_Text_GiftToPC[], PainterLoft_Text_GiftNoRoom[], PainterLoft_Text_FirstExplain[], PainterLoft_Text_FirstLost[], PainterLoft_Text_Sketched[], PainterLoft_Text_NoSketch[], PainterLoft_Text_SketchLost[], PainterLoft_Text_MegaWon[], PainterLoft_Text_MegaExplain[], PainterLoft_Text_MegaLost[], PaintWorld_Text_FigureFirst[], PaintWorld_Text_FigureSketch[], PaintWorld_Text_FigureMega[], PaintWorld_Text_FigureDefeat[], PaintWorld_Text_Dissolve[];

TEST("GymHack: the painter's lines fit the message box")
{
    static const u8 *const texts[] = {
        PainterLoft_Text_NoBadge,
        PainterLoft_Text_FirstIntro,
        PainterLoft_Text_FirstAsk,
        PainterLoft_Text_FirstGo,
        PainterLoft_Text_NoRoom,
        PainterLoft_Text_Later,
        PainterLoft_Text_Ready,
        PainterLoft_Text_NeedSmeargle,
        PainterLoft_Text_TooFew,
        PainterLoft_Text_Nothing,
        PainterLoft_Text_SketchIntro,
        PainterLoft_Text_SketchAsk,
        PainterLoft_Text_WhichType,
        PainterLoft_Text_WhichSlot,
        PainterLoft_Text_ConfirmSlot,
        PainterLoft_Text_SketchGo,
        PainterLoft_Text_MegaIntro,
        PainterLoft_Text_MegaAsk,
        PainterLoft_Text_MegaGo,
        PainterLoft_Text_BothAsk,
        PainterLoft_Text_EaselBlank,
        PainterLoft_Text_EaselAsk,
        PainterLoft_Text_Lend,
        PainterLoft_Text_Borrowed,
        PainterLoft_Text_SteppingIn,
        PainterLoft_Text_Smeargle,
        PainterLoft_Text_Canvases,
        PainterLoft_Text_Paint,
        PainterLoft_Text_PaintPot,
        PainterLoft_Text_CameBackEarly,
        PainterLoft_Text_FirstWon,
        PainterLoft_Text_ReceivedSmeargle,
        PainterLoft_Text_GiftToPC,
        PainterLoft_Text_GiftNoRoom,
        PainterLoft_Text_FirstExplain,
        PainterLoft_Text_FirstLost,
        PainterLoft_Text_Sketched,
        PainterLoft_Text_NoSketch,
        PainterLoft_Text_SketchLost,
        PainterLoft_Text_MegaWon,
        PainterLoft_Text_MegaExplain,
        PainterLoft_Text_MegaLost,
        PaintWorld_Text_FigureFirst,
        PaintWorld_Text_FigureSketch,
        PaintWorld_Text_FigureMega,
        PaintWorld_Text_FigureDefeat,
        PaintWorld_Text_Dissolve,
    };
    u32 i;
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gStringVar1, COMPOUND_STRING("WWWWWWWWWW"));
    StringCopy(gStringVar2, COMPOUND_STRING("WWWWWWWWWWWW"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectStoryTextFits(texts[i], 27 * 8 - 4, 99);
}

TEST("GymHack: QUICK SKETCH copies the move the player picked, if the target knows it and it's the quest's")
{
    static const enum Move foe[MAX_MON_MOVES] = {MOVE_EMBER, MOVE_FLAMETHROWER, MOVE_THUNDERBOLT, MOVE_NONE};
    static const enum Move mine[MAX_MON_MOVES] = {MOVE_MASTERSTROKE, MOVE_QUICK_SKETCH, MOVE_FLAMETHROWER, MOVE_NONE};
    u32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        gBattleMons[0].moves[i] = mine[i];
        gBattleMons[1].moves[i] = foe[i];
    }
    FlagClear(FLAG_IN_PAINTING);
    EXPECT(Painter_QuickSketchCanCopy(0, MOVE_EMBER));
    EXPECT(Painter_QuickSketchCanCopy(0, MOVE_THUNDERBOLT));     // (outside a Sketch quest: anything)
    EXPECT(!Painter_QuickSketchCanCopy(0, MOVE_FLAMETHROWER));   // (already known)
    EXPECT(!Painter_QuickSketchCanCopy(0, MOVE_NONE));
    EXPECT(!Painter_QuickSketchCanCopy(0, MOVE_SKETCH));         // (Sketch-banned)
    FlagSet(FLAG_IN_PAINTING);
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_SKETCH);
    VarSet(VAR_PAINTER_SKETCH, 2 | (TYPE_FIRE << 8));
    EXPECT(Painter_QuickSketchCanCopy(0, MOVE_EMBER));
    EXPECT(!Painter_QuickSketchCanCopy(0, MOVE_THUNDERBOLT));    // (not the quest's type)
    gQuickSketchChoice[0] = MOVE_EMBER;
    EXPECT_EQ(Painter_QuickSketchMove(0, 1), MOVE_EMBER);
    gQuickSketchChoice[0] = MOVE_SURF;
    EXPECT_EQ(Painter_QuickSketchMove(0, 1), MOVE_NONE);         // (the target doesn't know it)
    EXPECT_EQ(GetMoveEffect(MOVE_QUICK_SKETCH), EFFECT_QUICK_SKETCH);
    Painter_ResetForTest();
    VarSet(VAR_PAINTER_SKETCH, 0);
}

#include "energy_cards.h"
TEST("GymHack: energy cards: +5 once per damaging move of their types, on any of the four; it follows the move")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    enum Move moves[MAX_MON_MOVES] = {MOVE_EMBER, MOVE_DRAGON_BREATH, MOVE_GROWL, MOVE_THUNDERBOLT};
    u32 i;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_CHARMANDER, 20, 0, OTID_STRUCT_PLAYER_ID);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
    // Fire: Fire, Dragon, Flying
    EXPECT(EnergyCard_CanBoostSlot(mon, 0, ITEM_FIRE_ENERGY));
    EXPECT(EnergyCard_CanBoostSlot(mon, 1, ITEM_FIRE_ENERGY));
    EXPECT(!EnergyCard_CanBoostSlot(mon, 2, ITEM_FIRE_ENERGY));     // (Growl: a status move)
    EXPECT(!EnergyCard_CanBoostSlot(mon, 3, ITEM_FIRE_ENERGY));     // (Electric)
    EXPECT(EnergyCard_CanBoostSlot(mon, 3, ITEM_LIGHTNING_ENERGY));
    EXPECT(!EnergyCard_CanBoostAny(mon, ITEM_GRASS_ENERGY));
    EnergyCard_Boost(mon, 0);
    EXPECT(!EnergyCard_CanBoostSlot(mon, 0, ITEM_FIRE_ENERGY));     // (once per move)
    EXPECT(IsEnergyBoostedMove(mon, MOVE_EMBER));
    EnergyCard_Boost(mon, 1);
    EnergyCard_Boost(mon, 3);
    EXPECT_EQ(GetMonData(mon, MON_DATA_ENERGY_BOOSTS), 0xB);
    // forgetting a move takes its boost; the others keep theirs
    {
        enum Move m = MOVE_SCRATCH;
        SetMonData(mon, MON_DATA_MOVE1 + 1, &m);
    }
    EXPECT(!IsEnergyBoostedMove(mon, MOVE_SCRATCH));
    EXPECT(IsEnergyBoostedMove(mon, MOVE_EMBER));
    EXPECT(IsEnergyBoostedMove(mon, MOVE_THUNDERBOLT));
    // the first move forgotten for a new one: the boosts move up with their moves
    DeleteFirstMoveAndGiveMoveToMon(mon, MOVE_FLAMETHROWER);
    EXPECT(IsEnergyBoostedMove(mon, MOVE_THUNDERBOLT));
    EXPECT(!IsEnergyBoostedMove(mon, MOVE_FLAMETHROWER));
    EXPECT_EQ(GetMonData(mon, MON_DATA_ENERGY_BOOSTS), 0x4);
    EXPECT_EQ(SwapStolenMoveBits(GetMonData(mon, MON_DATA_ENERGY_BOOSTS), 2, 0), 0x1);
    // the six type energies cover all eighteen types, each once
    {
        static const u16 energies[] = {ITEM_GRASS_ENERGY, ITEM_FIRE_ENERGY, ITEM_WATER_ENERGY, ITEM_LIGHTNING_ENERGY, ITEM_PSYCHIC_ENERGY, ITEM_FIGHTING_ENERGY};
        u32 t, e;
        for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
        {
            u32 n = 0;
            if (t == TYPE_MYSTERY || t == TYPE_STELLAR)
                continue;
            for (e = 0; e < ARRAY_COUNT(energies); e++)
                n += EnergyCard_CoversType(energies[e], t);
            EXPECT_EQ(n, 1);
        }
    }
}

TEST("GymHack: Double Colorless Energy: a PP Max on every move, once")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    enum Move moves[MAX_MON_MOVES] = {MOVE_EMBER, MOVE_GROWL, MOVE_NONE, MOVE_NONE};
    u32 i, pp;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_CHARMANDER, 20, 0, OTID_STRUCT_PLAYER_ID);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        pp = GetMovePP(moves[i]);
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &pp);
    }
    EXPECT(EnergyCard_MaxAllPP(mon));
    EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), 0x0F);
    EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), GetMovePP(MOVE_EMBER) * 8 / 5);
    EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), GetMovePP(MOVE_GROWL) * 8 / 5);
    EXPECT(!EnergyCard_MaxAllPP(mon));
}

#include "drink_machine.h"
TEST("GymHack: the Drink Machine's recipes; anything else makes nothing")
{
    u16 b[2];
#define MIX1(d, x) (b[0] = (x), DrinkMachine_Recipe(d, b, 1))
#define MIX2(d, x, y) (b[0] = (x), b[1] = (y), DrinkMachine_Recipe(d, b, 2))
    EXPECT_EQ(MIX1(ITEM_FRESH_WATER, ITEM_ORAN_BERRY), ITEM_SUPER_POTION);
    EXPECT_EQ(MIX1(ITEM_FRESH_WATER, ITEM_SITRUS_BERRY), ITEM_SUPER_POTION);
    EXPECT_EQ(MIX1(ITEM_SODA_POP, ITEM_ORAN_BERRY), ITEM_HYPER_POTION);
    EXPECT_EQ(MIX1(ITEM_LEMONADE, ITEM_ORAN_BERRY), ITEM_HYPER_POTION);
    EXPECT_EQ(MIX1(ITEM_LEMONADE, ITEM_SITRUS_BERRY), ITEM_MAX_POTION);
    EXPECT_EQ(MIX1(ITEM_FRESH_WATER, ITEM_CHERI_BERRY), ITEM_FULL_HEAL);
    EXPECT_EQ(MIX1(ITEM_SODA_POP, ITEM_LUM_BERRY), ITEM_FULL_HEAL);
    EXPECT_EQ(MIX1(ITEM_FRESH_WATER, ITEM_LEPPA_BERRY), ITEM_ETHER);
    EXPECT_EQ(MIX1(ITEM_SODA_POP, ITEM_LEPPA_BERRY), ITEM_ELIXIR);
    EXPECT_EQ(MIX1(ITEM_LEMONADE, ITEM_LEPPA_BERRY), ITEM_MAX_ELIXIR);
    EXPECT_EQ(MIX2(ITEM_LEMONADE, ITEM_SITRUS_BERRY, ITEM_LUM_BERRY), ITEM_FULL_RESTORE);
    EXPECT_EQ(MIX2(ITEM_LEMONADE, ITEM_LUM_BERRY, ITEM_SITRUS_BERRY), ITEM_FULL_RESTORE);
    EXPECT_EQ(MIX2(ITEM_FRESH_WATER, ITEM_ORAN_BERRY, ITEM_SITRUS_BERRY), ITEM_REVIVE);
    EXPECT_EQ(MIX2(ITEM_LEMONADE, ITEM_ORAN_BERRY, ITEM_SITRUS_BERRY), ITEM_REVIVE);
    EXPECT_EQ(MIX2(ITEM_LEMONADE, ITEM_SITRUS_BERRY, ITEM_SITRUS_BERRY), ITEM_MAX_REVIVE);
    // not recipes
    EXPECT_EQ(DrinkMachine_Recipe(ITEM_FRESH_WATER, b, 0), ITEM_NONE);
    EXPECT_EQ(MIX1(ITEM_FRESH_WATER, ITEM_OCCA_BERRY), ITEM_NONE);
    EXPECT_EQ(MIX2(ITEM_SODA_POP, ITEM_SITRUS_BERRY, ITEM_LUM_BERRY), ITEM_NONE);
    EXPECT_EQ(MIX2(ITEM_LEMONADE, ITEM_CHERI_BERRY, ITEM_LUM_BERRY), ITEM_NONE);
    EXPECT_EQ(MIX1(ITEM_POTION, ITEM_ORAN_BERRY), ITEM_NONE);
    // the soda: one type berry, and nothing else
    EXPECT_EQ(MIX1(ITEM_LIMITED_EDITION_SODA, ITEM_OCCA_BERRY), ITEM_LIMITED_EDITION_SODA);
    EXPECT_EQ(MIX1(ITEM_LIMITED_EDITION_SODA, ITEM_ROSELI_BERRY), ITEM_LIMITED_EDITION_SODA);
    EXPECT_EQ(MIX1(ITEM_LIMITED_EDITION_SODA, ITEM_SITRUS_BERRY), ITEM_NONE);
    EXPECT_EQ(MIX2(ITEM_LIMITED_EDITION_SODA, ITEM_OCCA_BERRY, ITEM_OCCA_BERRY), ITEM_NONE);
#undef MIX1
#undef MIX2
}

TEST("GymHack: the LIMITED EDITION SODA: a second type for good, not the first; it lasts through evolution")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    enum Species species = SPECIES_IVYSAUR;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_BULBASAUR, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetMonType(mon, 0), TYPE_GRASS);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_POISON);
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_GRASS), DRINK_SODA_FIRST_TYPE);
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_POISON), DRINK_SODA_ALREADY);
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_FAIRY), DRINK_SODA_DONE);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_FAIRY);
    SetMonData(mon, MON_DATA_SPECIES, &species);
    EXPECT_EQ(GetMonType(mon, 0), TYPE_GRASS);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_FAIRY);
    // again: another type replaces it
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_STEEL), DRINK_SODA_DONE);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_STEEL);
    // a one-type Pokemon gets a second
    CreateMon(mon, SPECIES_CHARMANDER, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_FIRE);
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_FIRE), DRINK_SODA_FIRST_TYPE);
    EXPECT_EQ(DrinkMachine_GiveSecondType(mon, TYPE_DRAGON), DRINK_SODA_DONE);
    EXPECT_EQ(GetMonType(mon, 1), TYPE_DRAGON);
}

TEST("GymHack: a soda's second type is the battler's")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    struct BattlePokemon bm;
    u8 t = TYPE_WATER;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_CHARMANDER, 20, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(mon, MON_DATA_SECOND_TYPE, &t);
    PokemonToBattleMon(mon, &bm);
    EXPECT_EQ(bm.types[0], TYPE_FIRE);
    EXPECT_EQ(bm.types[1], TYPE_WATER);
}

TEST("GymHack: tournament wins can give a LIMITED EDITION SODA once the player has the Drink Machine")
{
    FlagClear(FLAG_GOT_DRINK_MACHINE);
    VarSet(VAR_SODAS_FOR_MASTERY, 0);
    EXPECT(!DrinkMachine_TournamentSodaUnlocked());
    FlagSet(FLAG_GOT_DRINK_MACHINE);
    EXPECT(DrinkMachine_TournamentSodaUnlocked());     // (whether or not the girl's sodas were collected)
    FlagClear(FLAG_GOT_DRINK_MACHINE);
}

#include "tournament.h"
TEST("GymHack: tournament prizes: one every win; a MOVE EXTRACTOR ~20% and a soda ~10% a title run")
{
    u32 roll, total, extractor = 0, soda = 0, rareCount = 0;
    bool32 rare;
    FlagClear(FLAG_GOT_DRINK_MACHINE);
    total = Tourney_DropTotal();
    for (roll = 0; roll < total; roll++)
        EXPECT_NE(Tourney_DropForRoll(roll, &rare), ITEM_LIMITED_EDITION_SODA);   // (no machine: no soda)
    FlagSet(FLAG_GOT_DRINK_MACHINE);
    total = Tourney_DropTotal();
    EXPECT_EQ(total, 10000);
    for (roll = 0; roll < total; roll++)
    {
        u32 item = Tourney_DropForRoll(roll, &rare);
        EXPECT(item != ITEM_NONE && item < ITEMS_COUNT);
        extractor += (item == ITEM_MOVE_EXTRACTOR);
        soda += (item == ITEM_LIMITED_EDITION_SODA);
        rareCount += rare;
    }
    // per win; over a run of TOURNEY_ROUNDS wins: 1 - (1 - p)^7 ~ 20% and ~10%
    EXPECT_EQ(extractor, 312);
    EXPECT_EQ(soda, 149);
    EXPECT_LT(rareCount, total / 8);
    FlagClear(FLAG_GOT_DRINK_MACHINE);
}

#include "medical_center.h"
#include "pokerus.h"
TEST("GymHack: the HEAL CHARM: the player's medicine heals 10% more, with it in the bag")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u16 hp = 1;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_SNORLAX, 100, 0, OTID_STRUCT_PLAYER_ID);
    CalculateMonStats(mon);
    SetMonData(mon, MON_DATA_HP, &hp);
    RemoveBagItem(ITEM_HEAL_CHARM, 1);
    EXPECT_EQ(HealCharm_Boost(120), 120);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 1);
    EXPECT_EQ(PokemonUseItemEffects(mon, ITEM_HYPER_POTION, 0, 0, FALSE), FALSE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 1 + 120);
    AddBagItem(ITEM_HEAL_CHARM, 1);
    EXPECT_EQ(HealCharm_Boost(120), 132);
    SetMonData(mon, MON_DATA_HP, &hp);
    PokemonUseItemEffects(mon, ITEM_HYPER_POTION, 0, 0, FALSE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 1 + 132);
    SetMonData(mon, MON_DATA_HP, &hp);
    PokemonUseItemEffects(mon, ITEM_MAX_POTION, 0, 0, FALSE);           // (a full heal: just full)
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
    RemoveBagItem(ITEM_HEAL_CHARM, 1);
}

TEST("GymHack: the Vermilion foreman's helpers: MACHOP, MACHOKE, MACHAMP, then any shiny one")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    bool8 fateful = TRUE;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_MACHOP, 20, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 0, 2), HELPER_OK);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 0, 1), HELPER_LAST_MON);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 1, 2), HELPER_WRONG_SPECIES);
    CreateMon(mon, SPECIES_MACHAMP, 50, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 2, 2), HELPER_OK);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 3, 2), GetMonData(mon, MON_DATA_IS_SHINY) ? HELPER_OK : HELPER_NOT_SHINY);
    {
        bool8 shiny = TRUE;
        SetMonData(mon, MON_DATA_IS_SHINY, &shiny);
    }
    EXPECT_EQ(MedCenter_HelperCheck(mon, 3, 2), HELPER_OK);
    SetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &fateful);       // (Bill's Palm Pilot: fine too)
    EXPECT_EQ(MedCenter_HelperCheck(mon, 3, 2), HELPER_OK);
    EXPECT_EQ(MedCenter_HelperCheck(mon, 4, 2), HELPER_WRONG_SPECIES);
}

TEST("GymHack: Pokérus from the Medical Center wears off a day of steps at a time, and keeps its effect")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u32 i;
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_PIDGEY, 20, 0, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
    EXPECT_EQ(MedCenter_TryGivePokerus(mon), POKERUS_GIVE_DONE);
    EXPECT(CheckMonPokerus(mon));
    EXPECT_EQ(MedCenter_TryGivePokerus(mon), POKERUS_GIVE_HAS_IT);
    FlagSet(FLAG_POKERUS_GIVEN_TODAY);
    VarSet(VAR_STEP_DAY, 0);
    for (i = 0; i < MEDCENTER_DAY_STEPS - 1; i++)
        MedicalCenter_OnStep();
    EXPECT(FlagGet(FLAG_POKERUS_GIVEN_TODAY));
    MedicalCenter_OnStep();                                             // (a day: the nurse can give it again)
    EXPECT(!FlagGet(FLAG_POKERUS_GIVEN_TODAY));
    for (i = 0; i < 4 * MEDCENTER_DAY_STEPS; i++)                       // (4 days: the longest it lasts)
        MedicalCenter_OnStep();
    EXPECT(!CheckMonPokerus(mon));
    EXPECT(CheckMonHasHadPokerus(mon));                                 // (cured: still double EVs, can't catch it again)
    EXPECT_EQ(MedCenter_TryGivePokerus(mon), POKERUS_GIVE_HAD_IT);
}

#include "item_menu.h"
TEST("GymHack: SELECT: up to five items registered, in order; a sixth is refused")
{
    u32 i;
    static const u16 items[] = {ITEM_BICYCLE, ITEM_OLD_ROD, ITEM_GOOD_ROD, ITEM_SUPER_ROD, ITEM_TOWN_MAP, ITEM_ITEMFINDER};
    RegisteredItems_Clear();
    for (i = 0; i < REGISTERED_ITEMS_MAX; i++)
        EXPECT(ToggleRegisteredItem(items[i]));
    EXPECT(!ToggleRegisteredItem(items[5]));
    EXPECT_EQ(CountRegisteredItems(), 5);
    EXPECT(ToggleRegisteredItem(items[1]));         // (deselected: the rest move up)
    EXPECT(!IsItemRegistered(items[1]));
    EXPECT_EQ(GetRegisteredItem(1), items[2]);
    EXPECT(ToggleRegisteredItem(items[5]));
    EXPECT_EQ(GetRegisteredItem(4), items[5]);
    SwapRegisteredBike();
    EXPECT_EQ(GetRegisteredItem(0), ITEM_BICYCLE);  // (FireRed's one bike: not swapped)
    RegisteredItems_Clear();
    EXPECT_EQ(CountRegisteredItems(), 0);
}

#include "pit.h"
extern const u8 Pit_Text_HiddenDoorFound[], Pit_Text_DoormanStop[], Pit_Text_DoormanWrong[], Pit_Text_DoormanRight[],
    Pit_Text_DoormanLetIn[], Pit_Text_CardHeader[], Pit_Text_BookieFirst[], Pit_Text_BookieCard[], Pit_Text_BookieBetTaken[],
    Pit_Text_BookieLater[], Pit_Text_BookieBroke[], Pit_Text_BookieWatch[], Pit_Text_BookieAlreadyBet[], Pit_Text_BookieBell[],
    Pit_Text_BookieResult[], Pit_Text_BetWon[], Pit_Text_BetLost[], Pit_Text_BookieNextBout[], Pit_Text_TipsterOffer[],
    Pit_Text_TipsterNo[], Pit_Text_TipsterBroke[], Pit_Text_TipClean[], Pit_Text_TipDive[], Pit_Text_TipItem[], Pit_Text_TipLevel[],
    Pit_Text_Crowd0[], Pit_Text_Crowd1[], Pit_Text_Crowd2[], Pit_Text_Crowd3[], Pit_Text_Crowd4[], Pit_Text_Crowd5[],
    Pit_Text_Crowd6[], Pit_Text_Crowd7[], Pit_Text_Overheard[], Pit_Text_WhispererCaught[],
    Pit_Text_ListenerBeatIt[], Pit_Text_BlueLoses[];
extern const u8 GymRooms_Text_OakCaughtSoMany[], GymRooms_Text_OakBackRoom[], GymRooms_Text_OakLeftRoom[],
    GymRooms_Text_OakRightRoom[], GymRooms_Text_DoorwayOpened[], GymRooms_Text_OakShowItOff[];

TEST("GymHack: the pit's and the gym rooms' lines fit the message box")
{
    static const u8 *const texts[] = {Pit_Text_HiddenDoorFound, Pit_Text_DoormanStop, Pit_Text_DoormanWrong, Pit_Text_DoormanRight,
        Pit_Text_DoormanLetIn, Pit_Text_CardHeader, Pit_Text_BookieFirst, Pit_Text_BookieCard, Pit_Text_BookieBetTaken,
        Pit_Text_BookieLater, Pit_Text_BookieBroke, Pit_Text_BookieWatch, Pit_Text_BookieAlreadyBet, Pit_Text_BookieBell,
        Pit_Text_BookieResult, Pit_Text_BetWon, Pit_Text_BetLost, Pit_Text_BookieNextBout, Pit_Text_TipsterOffer,
        Pit_Text_TipsterNo, Pit_Text_TipsterBroke, Pit_Text_TipClean, Pit_Text_TipDive, Pit_Text_TipItem, Pit_Text_TipLevel,
        Pit_Text_Crowd0, Pit_Text_Crowd1, Pit_Text_Crowd2, Pit_Text_Crowd3, Pit_Text_Crowd4, Pit_Text_Crowd5,
        Pit_Text_Crowd6, Pit_Text_Crowd7, Pit_Text_Overheard, Pit_Text_WhispererCaught,
        Pit_Text_ListenerBeatIt, Pit_Text_BlueLoses, GymRooms_Text_OakCaughtSoMany, GymRooms_Text_OakBackRoom,
        GymRooms_Text_OakLeftRoom, GymRooms_Text_OakRightRoom, GymRooms_Text_DoorwayOpened, GymRooms_Text_OakShowItOff};
    u32 i, page, card;
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gStringVar1, COMPOUND_STRING("CRAWDAUNT"));
    StringCopy(gStringVar2, COMPOUND_STRING("9999999"));
    StringCopy(gStringVar3, COMPOUND_STRING("LAGGING TAIL"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
    // The card on the rim, for many cards
    for (card = 0; card < 60; card++)
    {
        Pit_NextCard();
        for (page = 0; page < 5; page++)
        {
            gSpecialVar_0x8004 = page;
            Pit_BufferCardPage();
            ExpectLinesFit(gStringVar4, 27 * 8 - 4, FALSE);
        }
    }
}

TEST("GymHack: the pit: the tipster takes ten percent of the bet, once a card")
{
    u32 card, fixed = 0;
    for (card = 0; card < 200; card++)
    {
        // (the tip first: free now, then 10% on top of the bet)
        Pit_NextCard();
        SetMoney(&gSaveBlock1Ptr->money, 11000);
        Pit_BuyTip();
        EXPECT_GE(gSpecialVar_Result, 1);
        EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 11000);
        fixed += (gSpecialVar_Result >= 2);
        Pit_BuyTip();
        EXPECT_GE(gSpecialVar_Result, 1);
        gSpecialVar_0x8004 = 0;
        gSpecialVar_0x8005 = 1;                // (¥10000)
        Pit_PlaceBet();
        EXPECT_EQ(gSpecialVar_Result, TRUE);
        EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 0);
        Pit_BuyTip();                          // (paid up: no more)
        EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 0);
    }
    EXPECT_GT(fixed, 20);                      // (about 25% fixed)
    EXPECT_LT(fixed, 100);

    // The bet first: his cut's taken when he talks, if it can be paid
    Pit_NextCard();
    SetMoney(&gSaveBlock1Ptr->money, 5000);
    gSpecialVar_0x8004 = 1;
    gSpecialVar_0x8005 = 0;                    // (¥5000)
    Pit_PlaceBet();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 0);
    Pit_BuyTip();
    EXPECT_EQ(gSpecialVar_Result, 0);
    SetMoney(&gSaveBlock1Ptr->money, 600);
    Pit_BuyTip();
    EXPECT_GE(gSpecialVar_Result, 1);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 100);
    Pit_BuyTip();
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 100);
}

TEST("GymHack: the pit: every bout's fair: the underdog pays at most about x6, the favorite at least x1.1")
{
    u32 card;
    for (card = 0; card < 150; card++)
    {
        Pit_NextCard();
        EXPECT_NE(Pit_GetSpecies(0), Pit_GetSpecies(1));
        EXPECT_GE(Pit_GetOdds(0), 11);
        EXPECT_GE(Pit_GetOdds(1), 11);
        EXPECT_LE(Pit_GetOdds(0), 61);
        EXPECT_LE(Pit_GetOdds(1), 61);
    }
}

#include "pit_boss.h"
#include "gym_legend.h"
extern const u8 Pit_Text_Saving[], Pit_Text_Saved[], PitBoss_Text_TheMon[], PitBoss_Text_Rules[], PitBoss_Text_Today[],
    PitBoss_Text_ShowMe[], PitBoss_Text_Confirm[], PitBoss_Text_Official[], PitBoss_Text_Defeat[], PitBoss_Text_Won[],
    PitBoss_Text_Received[], PitBoss_Text_ToPC[], PitBoss_Text_Draw[], PitBoss_Text_Lost[], PitBoss_Text_Forfeited[],
    PitBoss_Text_TellBuyBack[], PitBoss_Text_OfferBuyBack[], PitBoss_Text_BoughtBack[], PitBoss_Text_BoughtBackToPC[],
    PitBoss_Text_KeepIt[], PitBoss_Text_BuyBackBroke[], PitBoss_Text_BuyBackNoRoom[], PitBoss_Text_CleanedOut[],
    PitBoss_Text_NoDeal[], PitBoss_Text_Egg[], PitBoss_Text_Fainted[], PitBoss_Text_OnlyOne[], PitBoss_Text_TooStrong[],
    PitBoss_Text_TooWeak[], PitBoss_Text_Underdog[], PitBoss_Text_UnderdogSure[];

TEST("GymHack: the back room's lines fit the message box")
{
    static const u8 *const texts[] = {Pit_Text_Saving, Pit_Text_Saved, PitBoss_Text_TheMon, PitBoss_Text_Rules,
        PitBoss_Text_Today, PitBoss_Text_ShowMe, PitBoss_Text_Confirm, PitBoss_Text_Official, PitBoss_Text_Defeat,
        PitBoss_Text_Won, PitBoss_Text_Received, PitBoss_Text_ToPC, PitBoss_Text_Draw, PitBoss_Text_Lost,
        PitBoss_Text_Forfeited, PitBoss_Text_TellBuyBack, PitBoss_Text_OfferBuyBack, PitBoss_Text_BoughtBack,
        PitBoss_Text_BoughtBackToPC, PitBoss_Text_KeepIt, PitBoss_Text_BuyBackBroke, PitBoss_Text_BuyBackNoRoom,
        PitBoss_Text_CleanedOut, PitBoss_Text_NoDeal, PitBoss_Text_Egg, PitBoss_Text_Fainted, PitBoss_Text_OnlyOne,
        PitBoss_Text_TooStrong, PitBoss_Text_TooWeak, PitBoss_Text_Underdog, PitBoss_Text_UnderdogSure};
    u32 i;
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("WWWWWWW"));
    StringCopy(gStringVar1, COMPOUND_STRING("Crabominable"));
    StringCopy(gStringVar2, COMPOUND_STRING("Feraligatr"));
    for (i = 0; i < ARRAY_COUNT(texts); i++)
        ExpectLinesFit(texts[i], 27 * 8 - 4, TRUE);
}

TEST("GymHack: the back room: his Pokémon of the day - rare, now and then a legendary, never a gym legendary")
{
    u32 day, legends = 0, type, species, changes = 0, last = SPECIES_NONE;
    FlagSet(FLAG_PALLET_GYM_AWARDED);       // (before the gym: no legendaries at all)
    PitBoss_Clear();
    for (day = 0; day < 300; day++)
    {
        const struct SpeciesInfo *info;
        gPitBossSave.day = day;
        species = PitBoss_GetSpecies();
        EXPECT_NE(species, SPECIES_NONE);
        EXPECT_EQ(species, PitBoss_GetSpecies());          // (the same all day)
        info = &gSpeciesInfo[species];
        legends += info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
        for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
            EXPECT_NE(species, gGymLegends[type]);
        changes += (species != last);
        last = species;
    }
    EXPECT_GT(changes, 250);
    EXPECT_LT(legends, 25);         // (about 3%)
    FlagClear(FLAG_PALLET_GYM_AWARDED);
}

static void BackRoomParty(u32 species0, u32 level0, u32 species1)
{
    u32 i, iv = MAX_PER_STAT_IVS;
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], species0, level0, 0, OTID_STRUCT_PLAYER_ID);
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP_IV + i, &iv);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][0]);
    if (species1 != SPECIES_NONE)
        CreateMon(&gParties[B_TRAINER_PLAYER][1], species1, 20, 1, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
}

TEST("GymHack: the back room: a fair fight only - his Pokémon at the stake's level, stats within 10%")
{
    u32 day, strongDay = 0xFFFF, weakDay = 0xFFFF;
    PitBoss_Clear();
    for (day = 0; day < 500 && (strongDay == 0xFFFF || weakDay == 0xFFFF); day++)
    {
        gPitBossSave.day = day;
        if (GetSpeciesBaseStatTotal(PitBoss_GetSpecies()) >= 540)
            strongDay = day;
        else if (GetSpeciesBaseStatTotal(PitBoss_GetSpecies()) <= 330)
            weakDay = day;
    }
    EXPECT_NE(strongDay, 0xFFFF);
    EXPECT_NE(weakDay, 0xFFFF);
    gSpecialVar_0x8004 = 0;

    // The same Pokémon, perfect IVs: about even
    gPitBossSave.day = strongDay;
    BackRoomParty(PitBoss_GetSpecies(), 50, SPECIES_PIDGEY);
    PitBoss_CheckStake();
    EXPECT_EQ(gSpecialVar_Result, 0);
    // A Magikarp against a strong one: too weak; a Mewtwo against a weak one: too strong
    BackRoomParty(SPECIES_MAGIKARP, 50, SPECIES_PIDGEY);
    PitBoss_CheckStake();
    EXPECT_EQ(gSpecialVar_Result, 6);
    gPitBossSave.day = weakDay;
    BackRoomParty(SPECIES_MEWTWO, 50, SPECIES_PIDGEY);
    PitBoss_CheckStake();
    EXPECT_EQ(gSpecialVar_Result, 5);
    // Within 10% either way: yes; more than 10% stronger: no; 10-20% weaker: only if the player
    // insists (7); weaker still: no
    {
        static const u16 tries[] = {SPECIES_RATTATA, SPECIES_PIDGEOTTO, SPECIES_RATICATE, SPECIES_ARBOK, SPECIES_PERSIAN,
            SPECIES_GOLDUCK, SPECIES_PRIMEAPE, SPECIES_NIDOKING, SPECIES_RAPIDASH, SPECIES_SLOWBRO, SPECIES_DODRIO,
            SPECIES_HYPNO, SPECIES_KINGLER, SPECIES_ELECTRODE, SPECIES_RHYDON, SPECIES_SCYTHER, SPECIES_GYARADOS,
            SPECIES_ARCANINE, SPECIES_TYRANITAR, SPECIES_SNORLAX, SPECIES_SANDSLASH, SPECIES_FEAROW, SPECIES_VILEPLUME};
        u32 i, seen[8] = {0};
        gPitBossSave.day = strongDay;
        for (i = 0; i < ARRAY_COUNT(tries); i++)
        {
            u32 mp, bp = PitBoss_DayPower(50), want;
            BackRoomParty(tries[i], 50, SPECIES_PIDGEY);
            mp = PitBoss_Power(&gParties[B_TRAINER_PLAYER][0]);
            want = (mp * 100 > bp * 110) ? 5 : (mp * 100 >= bp * 90) ? 0 : (mp * 100 >= bp * 80) ? 7 : 6;
            PitBoss_CheckStake();
            EXPECT_EQ(gSpecialVar_Result, want);
            seen[want]++;
        }
        EXPECT_GT(seen[7], 0);
        EXPECT_GT(seen[6], 0);
    }
    // Not the player's only Pokémon
    BackRoomParty(SPECIES_MEWTWO, 50, SPECIES_NONE);
    PitBoss_CheckStake();
    EXPECT_EQ(gSpecialVar_Result, 4);
}

TEST("GymHack: the back room: lose and he keeps it (sold back for 500000); win and his is the player's")
{
    u32 boss;
    PitBoss_Clear();
    gPitBossSave.day = 7;
    boss = PitBoss_GetSpecies();
    gSpecialVar_0x8004 = 0;

    // Lost
    BackRoomParty(SPECIES_RAICHU, 40, SPECIES_PIDGEY);
    PitBoss_HoldStake();
    EXPECT(gPitBossSave.stakeHeld);
    PitBoss_SetupFight();
    EXPECT(PitBoss_FightActive());
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    gBattleOutcome = B_OUTCOME_LOST;
    PitBoss_EndFight();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIDGEY);
    EXPECT(!gPitBossSave.stakeHeld);
    PitBoss_HasHeld();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    SetMoney(&gSaveBlock1Ptr->money, 499999);
    PitBoss_BuyBack();
    EXPECT_EQ(gSpecialVar_Result, 0);
    SetMoney(&gSaveBlock1Ptr->money, 500000);
    PitBoss_BuyBack();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 0);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 2);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_RAICHU);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP), GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MAX_HP));
    PitBoss_HasHeld();
    EXPECT_EQ(gSpecialVar_Result, FALSE);

    // Won
    FlagClear(FLAG_PIT_BOSS_BEATEN);
    BackRoomParty(SPECIES_RAICHU, 40, SPECIES_PIDGEY);
    PitBoss_HoldStake();
    PitBoss_SetupFight();
    gBattleOutcome = B_OUTCOME_WON;
    PitBoss_EndFight();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 3);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_RAICHU);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_SPECIES), boss);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_LEVEL), 40);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_SPEED_IV), MAX_PER_STAT_IVS);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_OT_ID), GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_OT_ID));
    EXPECT(!GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_SANITY_IS_BAD_EGG));
    EXPECT(FlagGet(FLAG_PIT_BOSS_BEATEN));
    // (a new day: he's got another)
    PitBoss_NewDay();
    EXPECT(!FlagGet(FLAG_PIT_BOSS_BEATEN));
    EXPECT_EQ(gPitBossSave.day, 8);
}

TEST("GymHack: the back room: switching off mid-battle loses the stake")
{
    PitBoss_Clear();
    gSpecialVar_0x8004 = 1;
    BackRoomParty(SPECIES_PIDGEY, 30, SPECIES_RAICHU);
    PitBoss_HoldStake();
    // (the save before the battle; the power goes off; the save's loaded; a step)
    PitBoss_OnStep();
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIDGEY);
    PitBoss_HasHeld();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    PitBoss_CheckForfeit();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    PitBoss_CheckForfeit();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
}

TEST("GymHack: no Gen 4+ Pokémon to win in the back room before the gym (evolving and breeding still work)")
{
    struct Pokemon mon;
    u32 day, species, legends = 0, modern = 0;
    bool32 canStop;

    EXPECT(GymHack_IsModernSpecies(SPECIES_MAGNEZONE));
    EXPECT(GymHack_IsModernSpecies(SPECIES_MUNCHLAX));
    EXPECT(!GymHack_IsModernSpecies(SPECIES_MAGNETON));
    EXPECT(!GymHack_IsModernSpecies(SPECIES_RAICHU_ALOLA));
    EXPECT(!GymHack_IsModernSpecies(SPECIES_DEOXYS));

    // Before
    FlagClear(FLAG_PALLET_GYM_AWARDED);
    CreateMon(&mon, SPECIES_EEVEE, 30, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_LEAF_STONE, NULL, &canStop, CHECK_EVO), SPECIES_LEAFEON);
    PitBoss_Clear();
    for (day = 0; day < 400; day++)
    {
        const struct SpeciesInfo *info;
        gPitBossSave.day = day;
        species = PitBoss_GetSpecies();
        info = &gSpeciesInfo[species];
        legends += info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
        modern += GymHack_IsModernSpecies(species);
    }
    EXPECT_EQ(legends, 0);
    EXPECT_EQ(modern, 0);

    // After
    FlagSet(FLAG_PALLET_GYM_AWARDED);
    for (day = 0; day < 400; day++)
    {
        const struct SpeciesInfo *info;
        gPitBossSave.day = day;
        species = PitBoss_GetSpecies();
        info = &gSpeciesInfo[species];
        legends += info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
        modern += GymHack_IsModernSpecies(species);
    }
    EXPECT_GT(legends, 0);
    EXPECT_GT(modern, 100);
    FlagClear(FLAG_PALLET_GYM_AWARDED);
}
