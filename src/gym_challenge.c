// Gym Trainer hack: challengers at the player's Pallet Town Gym, after the League.
//
// Oak's aide in the gym sends challengers in one at a time, Battle Frontier style: the player
// battles until they lose. Each challenger is built here, into one trainer slot
// (TRAINER_GYM_CHALLENGER), which GetTrainerStructFromId (include/data.h) hands to the battle.
//   - most are random trainers with a random name and 4-6 strong Pokémon (legendaries included);
//   - some are famous: gym leaders, the Elite Four, the rival, Giovanni or Oak, with their teams;
//   - a few are townsfolk who don't usually battle: Mom, Nurse Joy, Bill, Daisy, Mr. Fuji.
// Every team's average level matches the player's. Wins pay money and an item; the streak,
// record and everyone who beats the player are saved (in the secret base space after the gym
// decorations) and shown on the gym statue. Scripts: data/scripts/gym_challenge.inc.
#include "global.h"
#include "gym_challenge.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "malloc.h"
#include "metatile_behavior.h"
#include "gym_decor.h"
#include "gym_candidate.h"
#include "money.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"
#include "constants/event_object_movement.h"
#include "constants/gym_decor.h"
#include "constants/gym_room.h"

#define GYM_CHALLENGE_MAGIC 0x4347   // "GC"

STATIC_ASSERT(sizeof(struct GymChallengeSave) <= 340, GymChallengeSaveFitsAfterDecorations);

EWRAM_DATA static struct Trainer sChallenger;
EWRAM_DATA static struct TrainerMon sChallengerMons[PARTY_SIZE];
EWRAM_DATA static u8 sLastFamous;
EWRAM_DATA static u8 sKind;
EWRAM_DATA static u16 sLastStreak;
EWRAM_DATA u8 gGymChallengeIntroText[320];
EWRAM_DATA u8 gGymChallengeDefeatText[120];  // in battle, when the challenger loses
EWRAM_DATA u8 gGymChallengeAfterText[160];   // after losing to the player
EWRAM_DATA u8 gGymChallengeWinText[160];     // after beating the player
EWRAM_DATA bool8 gGymPracticeBattle;         // a practice battle: no prize money (data/battle_scripts_1.s)
EWRAM_DATA bool8 gGymGuestBattle;            // a Guest Leader battle: a big prize (src/battle_script_commands.c)

// ---------------------------------------------------------------------------
// Save data
// ---------------------------------------------------------------------------

struct GymChallengeSave *GymChallenge_GetSave(void)
{
    u32 size;
    struct GymChallengeSave *save = GymDecor_GetSpareSaveSpace(&size);
    if (save->magic != GYM_CHALLENGE_MAGIC)
    {
        memset(save, 0, sizeof(*save));
        save->magic = GYM_CHALLENGE_MAGIC;
    }
    return save;
}

const struct Trainer *GymChallenge_GetTrainer(u16 trainerId)
{
#if IS_FRLG
    if (trainerId == TRAINER_GYM_CHALLENGER)
        return &sChallenger;
#endif
    return NULL;
}

// The challenges open once the gym is the player's: after beating Oak for it
// (data/scripts/gym_finale.inc)
bool8 GymChallenge_IsOpen(void)
{
    return FlagGet(FLAG_PALLET_GYM_AWARDED);
}

u32 GymChallenge_PlayerAverageLevel(void)
{
    u32 i, total = 0, count = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        total += GetMonData(mon, MON_DATA_LEVEL);
        count++;
    }
    if (count == 0)
        return 50;
    return (total + count / 2) / count;
}

// ---------------------------------------------------------------------------
// Who can show up
// ---------------------------------------------------------------------------

struct FamousChallenger
{
    u16 trainerId;              // their battle (its name, class, picture and team); 0 = Oak
    u16 gfx;
    const u16 *extras;          // species added (at the average level) to reach 4 Pokémon
    const u8 *intro;
    const u8 *defeat;
    const u8 *after;
    const u8 *win;
};

static const u16 sBrockExtras[] = {SPECIES_GOLEM, SPECIES_RHYDON, SPECIES_NONE};
static const u16 sMistyExtras[] = {SPECIES_LAPRAS, SPECIES_GOLDUCK, SPECIES_NONE};
static const u16 sSurgeExtras[] = {SPECIES_ELECTABUZZ, SPECIES_NONE};
static const u16 sErikaExtras[] = {SPECIES_EXEGGUTOR, SPECIES_NONE};
static const u16 sNoExtras[] = {SPECIES_NONE};

static const struct FamousChallenger sFamous[] =
{
    {TRAINER_LEADER_BROCK, OBJ_EVENT_GFX_BROCK, sBrockExtras,
        COMPOUND_STRING("BROCK: I heard PALLET TOWN has a\nnew GYM LEADER.\pTime for a rock-solid challenge!"),
        COMPOUND_STRING("Your defense is as tough\nas mine!"),
        COMPOUND_STRING("BROCK: Hah! You've built something\ngreat here, {PLAYER}."),
        COMPOUND_STRING("BROCK: Solid as a rock!\nKeep training, {PLAYER}.")},
    {TRAINER_LEADER_MISTY, OBJ_EVENT_GFX_MISTY, sMistyExtras,
        COMPOUND_STRING("MISTY: Don't think being a GYM\nLEADER makes you special!\pI'm one too, you know!"),
        COMPOUND_STRING("Ugh! Not again!"),
        COMPOUND_STRING("MISTY: Fine, you're good.\nHappy now?"),
        COMPOUND_STRING("MISTY: Ha! Who's the boss now?")},
    {TRAINER_LEADER_LT_SURGE, OBJ_EVENT_GFX_LT_SURGE, sSurgeExtras,
        COMPOUND_STRING("LT. SURGE: Hey, kid!\nReady for a shocking rematch?!"),
        COMPOUND_STRING("Whoa! You've still got\nthe spark!"),
        COMPOUND_STRING("LT. SURGE: You run a tight GYM,\nsoldier!"),
        COMPOUND_STRING("LT. SURGE: Ahahaha! Lightning never\nstrikes twice? Wrong!")},
    {TRAINER_LEADER_ERIKA, OBJ_EVENT_GFX_ERIKA, sErikaExtras,
        COMPOUND_STRING("ERIKA: What a lovely GYM.\nShall we have a gentle battle?"),
        COMPOUND_STRING("Oh… how splendid."),
        COMPOUND_STRING("ERIKA: Your GYM could use more\nflowers, I think."),
        COMPOUND_STRING("ERIKA: Oh my. I seem to have won.\nPlease don't be sad.")},
    {TRAINER_LEADER_KOGA, OBJ_EVENT_GFX_KOGA, sNoExtras,
        COMPOUND_STRING("KOGA: Fwahahaha! A ninja tests his\nstudents when they least expect it!"),
        COMPOUND_STRING("Fwahahaha… well done!"),
        COMPOUND_STRING("KOGA: You learn quickly. My\ndaughter could learn from you."),
        COMPOUND_STRING("KOGA: You let your guard down!")},
    {TRAINER_LEADER_SABRINA, OBJ_EVENT_GFX_SABRINA, sNoExtras,
        COMPOUND_STRING("SABRINA: I foresaw this battle.\pI also foresaw how it ends."),
        COMPOUND_STRING("My vision… was wrong?"),
        COMPOUND_STRING("SABRINA: The future isn't set.\nYou proved that."),
        COMPOUND_STRING("SABRINA: Just as I foresaw.")},
    {TRAINER_LEADER_BLAINE, OBJ_EVENT_GFX_BLAINE, sNoExtras,
        COMPOUND_STRING("BLAINE: Hah! Pop quiz!\nWho's about to get burned?"),
        COMPOUND_STRING("I've burned out!"),
        COMPOUND_STRING("BLAINE: Hah! Correct answer!"),
        COMPOUND_STRING("BLAINE: Hah! The answer was you!")},
    {TRAINER_LEADER_GIOVANNI, OBJ_EVENT_GFX_GIOVANNI, sNoExtras,
        COMPOUND_STRING("GIOVANNI: So this is your GYM.\nLet me test what it's worth."),
        COMPOUND_STRING("Hmph. A fine investment."),
        COMPOUND_STRING("GIOVANNI: …We will meet again."),
        COMPOUND_STRING("GIOVANNI: Weak. As I expected.")},
    {TRAINER_ELITE_FOUR_LORELEI, OBJ_EVENT_GFX_LORELEI, sNoExtras,
        COMPOUND_STRING("LORELEI: I took a day off from the\nLEAGUE just to freeze you solid!"),
        COMPOUND_STRING("You're better than\nI thought!"),
        COMPOUND_STRING("LORELEI: You're a credit to the\nLEAGUE, {PLAYER}."),
        COMPOUND_STRING("LORELEI: Ice beats everything,\nif you do it right.")},
    {TRAINER_ELITE_FOUR_BRUNO, OBJ_EVENT_GFX_BRUNO, sNoExtras,
        COMPOUND_STRING("BRUNO: I trained in the mountains\nfor this battle. Hoo hah!"),
        COMPOUND_STRING("Why? How could I lose?"),
        COMPOUND_STRING("BRUNO: Your POKéMON are strong.\nI respect that."),
        COMPOUND_STRING("BRUNO: Hoo hah! Strength wins!")},
    {TRAINER_ELITE_FOUR_AGATHA, OBJ_EVENT_GFX_AGATHA, sNoExtras,
        COMPOUND_STRING("AGATHA: So you're OAK's pet project.\nLet's see what you've got!"),
        COMPOUND_STRING("Oh ho! You're something\nspecial!"),
        COMPOUND_STRING("AGATHA: Tell OAK I said hello.\nOr don't."),
        COMPOUND_STRING("AGATHA: Kekeke!\nBack to school with you!")},
    {TRAINER_ELITE_FOUR_LANCE, OBJ_EVENT_GFX_LANCE, sNoExtras,
        COMPOUND_STRING("LANCE: The LEAGUE sent me to check\non its newest GYM.\pShow me your power!"),
        COMPOUND_STRING("My dragons… lost?"),
        COMPOUND_STRING("LANCE: The LEAGUE chose well."),
        COMPOUND_STRING("LANCE: A GYM LEADER must be\nstronger than that.")},
    {TRAINER_CHAMPION_FIRST_SQUIRTLE, OBJ_EVENT_GFX_BLUE, sNoExtras,
        COMPOUND_STRING("{RIVAL}: Hey, {PLAYER}!\pYou think you can keep this GYM?\nI'm taking it back!"),
        COMPOUND_STRING("What?! Again?!"),
        COMPOUND_STRING("{RIVAL}: Enjoy it while it lasts.\nSmell ya later!"),
        COMPOUND_STRING("{RIVAL}: Ha! I knew it!\nGramps should've picked me!")},
    {0, OBJ_EVENT_GFX_PROF_OAK, sNoExtras,
        COMPOUND_STRING("OAK: Ho ho! Don't look so surprised,\n{PLAYER}!\pI said I'd keep an eye on this GYM.\nLet's battle!"),
        COMPOUND_STRING("Ho ho! Splendid!\nSimply splendid!"),
        COMPOUND_STRING("OAK: I'm proud of you, {PLAYER}.\nBut don't get comfortable!"),
        COMPOUND_STRING("OAK: Ho ho! The old man still\nhas it!")},
};

static const u16 sOakTeam[] = {SPECIES_TAUROS, SPECIES_EXEGGUTOR, SPECIES_ARCANINE, SPECIES_GYARADOS, SPECIES_SNORLAX, SPECIES_DRAGONITE, SPECIES_NONE};

struct Townsfolk
{
    const u8 *name;
    u8 trainerClass;
    u8 trainerPic;
    u8 gender;                  // 0 = male, 1 = female (trainer)
    u16 gfx;
    const u16 *team;
    const u8 *intro;
    const u8 *defeat;
    const u8 *after;
    const u8 *win;
};

static const u16 sMomTeam[] = {SPECIES_KANGASKHAN, SPECIES_BLISSEY, SPECIES_MILTANK, SPECIES_CLEFABLE, SPECIES_WIGGLYTUFF, SPECIES_NONE};
static const u16 sNurseTeam[] = {SPECIES_CHANSEY, SPECIES_BLISSEY, SPECIES_TOGETIC, SPECIES_CLEFABLE, SPECIES_LANTURN, SPECIES_NONE};
static const u16 sBillTeam[] = {SPECIES_VAPOREON, SPECIES_JOLTEON, SPECIES_FLAREON, SPECIES_ESPEON, SPECIES_UMBREON, SPECIES_PORYGON2, SPECIES_NONE};
static const u16 sDaisyTeam[] = {SPECIES_PERSIAN, SPECIES_NINETALES, SPECIES_RAPIDASH, SPECIES_VILEPLUME, SPECIES_DELCATTY, SPECIES_NONE};
static const u16 sFujiTeam[] = {SPECIES_MAROWAK, SPECIES_LAPRAS, SPECIES_GENGAR, SPECIES_SLOWKING, SPECIES_HYPNO, SPECIES_NONE};

static const struct Townsfolk sTownsfolk[] =
{
    {COMPOUND_STRING("MOM"), TRAINER_CLASS_PKMN_BREEDER_FRLG, TRAINER_PIC_POKEMON_BREEDER_FRLG, 1, OBJ_EVENT_GFX_MOM_FRLG, sMomTeam,
        COMPOUND_STRING("MOM: Surprise! I signed up as a\nchallenger, {PLAYER}!\pDon't go easy on me just because\nI'm your mom!"),
        COMPOUND_STRING("Oh my! You've grown\nso strong!"),
        COMPOUND_STRING("MOM: Don't forget to come home for\ndinner, GYM LEADER!"),
        COMPOUND_STRING("MOM: Ho ho! Mom still knows best!")},
    {COMPOUND_STRING("JOY"), TRAINER_CLASS_NURSE, TRAINER_PIC_AROMA_LADY_FRLG, 1, OBJ_EVENT_GFX_NURSE_FRLG, sNurseTeam,
        COMPOUND_STRING("NURSE JOY: I heal POKéMON all day.\nToday, I'd like to battle one!"),
        COMPOUND_STRING("We hope to see you again!"),
        COMPOUND_STRING("NURSE JOY: Thank you for such a\nlovely battle!"),
        COMPOUND_STRING("NURSE JOY: Your POKéMON need rest.\nPlease visit the POKéMON CENTER!")},
    {COMPOUND_STRING("BILL"), TRAINER_CLASS_POKEMANIAC_FRLG, TRAINER_PIC_POKEMANIAC_FRLG, 0, OBJ_EVENT_GFX_BILL, sBillTeam,
        COMPOUND_STRING("BILL: Hiya! I'm a POKéMANIAC, and\nI raised this team myself!\pWanna see?"),
        COMPOUND_STRING("Hey, nice battling!"),
        COMPOUND_STRING("BILL: Drop by the SEA COTTAGE\nsometime!"),
        COMPOUND_STRING("BILL: Whoa, I won! I'd better save\nthat to my PC!")},
    {COMPOUND_STRING("DAISY"), TRAINER_CLASS_LADY_FRLG, TRAINER_PIC_BEAUTY_FRLG, 1, OBJ_EVENT_GFX_DAISY, sDaisyTeam,
        COMPOUND_STRING("DAISY: Grandpa told me all about\nyour GYM.\pMy POKéMON are groomed and ready!"),
        COMPOUND_STRING("Oh! Their fur is all\nmessy now."),
        COMPOUND_STRING("DAISY: Come by for tea sometime!"),
        COMPOUND_STRING("DAISY: My POKéMON looked great\nAND won!")},
    {COMPOUND_STRING("FUJI"), TRAINER_CLASS_MR, TRAINER_PIC_GENTLEMAN_FRLG, 0, OBJ_EVENT_GFX_MR_FUJI, sFujiTeam,
        COMPOUND_STRING("MR. FUJI: My POKéMON wanted to meet\nthe trainer who saved\lLAVENDER TOWN."),
        COMPOUND_STRING("You are kind, and strong."),
        COMPOUND_STRING("MR. FUJI: Thank you, {PLAYER}."),
        COMPOUND_STRING("MR. FUJI: Be kind to your POKéMON.\nThey gave their all.")},
};

// Random trainers
struct ChallengerClass
{
    u8 trainerClass;
    u8 trainerPic;
    u8 female;
    u16 gfx;
};

static const struct ChallengerClass sClasses[] =
{
    {TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_M_FRLG, FALSE, OBJ_EVENT_GFX_COOLTRAINER_M},
    {TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_F_FRLG, TRUE,  OBJ_EVENT_GFX_COOLTRAINER_F},
    {TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_M_FRLG, FALSE, OBJ_EVENT_GFX_COOLTRAINER_M},
    {TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_F_FRLG, TRUE,  OBJ_EVENT_GFX_COOLTRAINER_F},
    {TRAINER_CLASS_BLACK_BELT_FRLG,  TRAINER_PIC_BLACK_BELT_FRLG,    FALSE, OBJ_EVENT_GFX_BLACK_BELT_FRLG},
    {TRAINER_CLASS_GENTLEMAN_FRLG,   TRAINER_PIC_GENTLEMAN_FRLG,     FALSE, OBJ_EVENT_GFX_GENTLEMAN_FRLG},
    {TRAINER_CLASS_HIKER_FRLG,       TRAINER_PIC_HIKER_FRLG,         FALSE, OBJ_EVENT_GFX_HIKER_FRLG},
    {TRAINER_CLASS_ROCKER_FRLG,      TRAINER_PIC_ROCKER_FRLG,        FALSE, OBJ_EVENT_GFX_ROCKER},
    {TRAINER_CLASS_BIKER_FRLG,       TRAINER_PIC_BIKER_FRLG,         FALSE, OBJ_EVENT_GFX_BIKER},
    {TRAINER_CLASS_ENGINEER_FRLG,    TRAINER_PIC_ENGINEER_FRLG,      FALSE, OBJ_EVENT_GFX_WORKER_M},
    {TRAINER_CLASS_SCIENTIST_FRLG,   TRAINER_PIC_SCIENTIST_FRLG,     FALSE, OBJ_EVENT_GFX_SCIENTIST},
    {TRAINER_CLASS_POKEMANIAC_FRLG,  TRAINER_PIC_POKEMANIAC_FRLG,    FALSE, OBJ_EVENT_GFX_POKE_MANIAC_FRLG},
    {TRAINER_CLASS_CAMPER_FRLG,      TRAINER_PIC_CAMPER_FRLG,        FALSE, OBJ_EVENT_GFX_CAMPER_FRLG},
    {TRAINER_CLASS_SAILOR_FRLG,      TRAINER_PIC_SAILOR_FRLG,        FALSE, OBJ_EVENT_GFX_SAILOR_FRLG},
    {TRAINER_CLASS_FISHERMAN_FRLG,   TRAINER_PIC_FISHERMAN_FRLG,     FALSE, OBJ_EVENT_GFX_FISHER},
    {TRAINER_CLASS_YOUNGSTER_FRLG,   TRAINER_PIC_YOUNGSTER_FRLG,     FALSE, OBJ_EVENT_GFX_YOUNGSTER_FRLG},
    {TRAINER_CLASS_BEAUTY_FRLG,      TRAINER_PIC_BEAUTY_FRLG,        TRUE,  OBJ_EVENT_GFX_BEAUTY_FRLG},
    {TRAINER_CLASS_LASS_FRLG,        TRAINER_PIC_LASS_FRLG,          TRUE,  OBJ_EVENT_GFX_LASS_FRLG},
    {TRAINER_CLASS_PICNICKER_FRLG,   TRAINER_PIC_PICNICKER_FRLG,     TRUE,  OBJ_EVENT_GFX_PICNICKER_FRLG},
    {TRAINER_CLASS_CRUSH_GIRL_FRLG,  TRAINER_PIC_CRUSH_GIRL_FRLG,    TRUE,  OBJ_EVENT_GFX_CRUSH_GIRL},
    {TRAINER_CLASS_SWIMMER_F_FRLG,   TRAINER_PIC_SWIMMER_F_FRLG,     TRUE,  OBJ_EVENT_GFX_SWIMMER_F_LAND},
    {TRAINER_CLASS_SWIMMER_M_FRLG,   TRAINER_PIC_SWIMMER_M_FRLG,     FALSE, OBJ_EVENT_GFX_SWIMMER_M_LAND},
};

static const u8 *const sMaleNames[] =
{
    COMPOUND_STRING("AIDEN"), COMPOUND_STRING("BRODY"), COMPOUND_STRING("CALEB"), COMPOUND_STRING("DANTE"),
    COMPOUND_STRING("ELI"), COMPOUND_STRING("FELIX"), COMPOUND_STRING("GRANT"), COMPOUND_STRING("HUGO"),
    COMPOUND_STRING("IAN"), COMPOUND_STRING("JASPER"), COMPOUND_STRING("KAI"), COMPOUND_STRING("LEON"),
    COMPOUND_STRING("MILES"), COMPOUND_STRING("NOLAN"), COMPOUND_STRING("OSCAR"), COMPOUND_STRING("PARKER"),
    COMPOUND_STRING("QUINN"), COMPOUND_STRING("ROWAN"), COMPOUND_STRING("SILAS"), COMPOUND_STRING("TOBIAS"),
    COMPOUND_STRING("VICTOR"), COMPOUND_STRING("WESLEY"), COMPOUND_STRING("XAVIER"), COMPOUND_STRING("ZANE"),
    COMPOUND_STRING("MARCUS"), COMPOUND_STRING("DEREK"), COMPOUND_STRING("ROY"), COMPOUND_STRING("GUS"),
};

static const u8 *const sFemaleNames[] =
{
    COMPOUND_STRING("ADA"), COMPOUND_STRING("BELLA"), COMPOUND_STRING("CORA"), COMPOUND_STRING("DIANA"),
    COMPOUND_STRING("ELLIE"), COMPOUND_STRING("FIONA"), COMPOUND_STRING("GWEN"), COMPOUND_STRING("HAZEL"),
    COMPOUND_STRING("IVY"), COMPOUND_STRING("JUNE"), COMPOUND_STRING("KIRA"), COMPOUND_STRING("LUNA"),
    COMPOUND_STRING("MAYA"), COMPOUND_STRING("NORA"), COMPOUND_STRING("OLIVE"), COMPOUND_STRING("PIPER"),
    COMPOUND_STRING("RUBY"), COMPOUND_STRING("SADIE"), COMPOUND_STRING("TESSA"), COMPOUND_STRING("UMA"),
    COMPOUND_STRING("VERA"), COMPOUND_STRING("WILLA"), COMPOUND_STRING("YARA"), COMPOUND_STRING("ZOE"),
    COMPOUND_STRING("NINA"), COMPOUND_STRING("CLAIRE"), COMPOUND_STRING("MEG"), COMPOUND_STRING("LILA"),
};

static const u8 *const sTrainerIntros[] =
{
    COMPOUND_STRING("I came all the way to PALLET TOWN\nfor this! Let's battle, LEADER!"),
    COMPOUND_STRING("They say the new GYM LEADER is\ntough. Let's see about that!"),
    COMPOUND_STRING("I've beaten the ELITE FOUR twice.\nYou're next, {PLAYER}!"),
    COMPOUND_STRING("Nice GYM!\pBut I'm not here to sightsee.\nLet's go!"),
    COMPOUND_STRING("I trained my team just for you.\nDon't hold back!"),
    COMPOUND_STRING("I heard you only use one type.\nThat'll be your downfall!"),
    COMPOUND_STRING("I've been in line since sunrise.\nFinally! Let's battle!"),
    COMPOUND_STRING("My team is on a winning streak too.\nOnly one of us keeps it today!"),
    COMPOUND_STRING("PROF. OAK says you're the best.\nProve it!"),
    COMPOUND_STRING("I caught a legend for this battle.\nHope you're ready!"),
};

static const u8 *const sTrainerDefeats[] =
{
    COMPOUND_STRING("I can't believe it…"),
    COMPOUND_STRING("You earned that win."),
    COMPOUND_STRING("So that's a GYM LEADER's\npower!"),
    COMPOUND_STRING("I'll train harder and\ncome back!"),
    COMPOUND_STRING("Wow… that was close!"),
    COMPOUND_STRING("Your one-type team got me…"),
};

static const u8 *const sTrainerAfters[] =
{
    COMPOUND_STRING("I'll be back with a stronger team!"),
    COMPOUND_STRING("Good battle, LEADER. Really."),
    COMPOUND_STRING("Your GYM is going to be famous."),
    COMPOUND_STRING("Next time, I'm bringing\na Water-type!"),
};

static const u8 *const sTrainerWins[] =
{
    COMPOUND_STRING("Ha! I did it!\nI beat a GYM LEADER!"),
    COMPOUND_STRING("Wait until everyone hears about\nthis!"),
    COMPOUND_STRING("Good battle, {PLAYER}!\nYour type has a weak spot after all."),
    COMPOUND_STRING("My name's going on that statue!"),
};

static const u16 sHeldItems[] =
{
    ITEM_LEFTOVERS, ITEM_SITRUS_BERRY, ITEM_LUM_BERRY, ITEM_QUICK_CLAW, ITEM_KINGS_ROCK,
    ITEM_FOCUS_BAND, ITEM_SHELL_BELL, ITEM_SCOPE_LENS, ITEM_BRIGHT_POWDER, ITEM_WHITE_HERB,
};

u32 GymChallenge_CountFamous(void)
{
    return ARRAY_COUNT(sFamous);
}

u32 GymChallenge_CountTownsfolk(void)
{
    return ARRAY_COUNT(sTownsfolk);
}

// ---------------------------------------------------------------------------
// Building a challenger
// ---------------------------------------------------------------------------

static bool32 HasEvolution(u32 species)
{
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    return evos != NULL && evos[0].method != EVOLUTIONS_END;
}

// Random trainers use fully evolved Pokémon (and legendaries) from the first three generations
static bool32 IsStrongSpecies(u32 species)
{
    return IsSpeciesEnabled(species) && !HasEvolution(species)
        && GetSpeciesBaseStatTotal(species) >= 420;
}

// A Pokémon from an early-game team that's now high level evolves (Geodude -> Golem)
u32 GymChallenge_EvolveForLevel(u32 species, u32 level)
{
    u32 i;
    for (i = 0; i < 3 && HasEvolution(species); i++)
    {
        const struct Evolution *evo = &GetSpeciesEvolutions(species)[0];
        if (evo->method == EVO_LEVEL ? evo->param > level : level < 36)
            break;
        species = evo->targetSpecies;
    }
    return species;
}

static u32 ClampLevel(s32 level)
{
    if (level < 1)
        return 1;
    if (level > MAX_LEVEL)
        return MAX_LEVEL;
    return level;
}

// Level spreads around the average that keep it exact
static const s8 sLevelSpread[PARTY_SIZE + 1][PARTY_SIZE] =
{
    [1] = {0},
    [2] = {-1, 1},
    [3] = {-1, 0, 1},
    [4] = {-1, 0, 0, 1},
    [5] = {-2, -1, 0, 1, 2},
    [6] = {-2, -1, 0, 0, 1, 2},
};

static void ShuffleLevels(s8 *spread, u32 n)
{
    u32 i;
    for (i = n - 1; i > 0; i--)
    {
        u32 j = Random() % (i + 1);
        s8 t = spread[i];
        spread[i] = spread[j];
        spread[j] = t;
    }
}

static void InitMon(struct TrainerMon *mon, u32 species, u32 level, u32 iv)
{
    memset(mon, 0, sizeof(*mon));
    mon->species = species;
    mon->lvl = level;
    mon->iv = TRAINER_PARTY_IVS(iv, iv, iv, iv, iv, iv);
    mon->nature = Random() % NUM_NATURES;
    mon->gender = TRAINER_MON_RANDOM_GENDER;
    mon->ball = POKEBALL_COUNT;
    mon->friendship = MAX_FRIENDSHIP;
}

static void SetupTrainer(u32 trainerClass, u32 pic, bool32 female, const u8 *name, u32 partySize)
{
    memset(&sChallenger, 0, sizeof(sChallenger));
    sChallenger.aiFlags = AI_FLAG_SMART_TRAINER;
    sChallenger.party = sChallengerMons;
    sChallenger.partySize = partySize;
    sChallenger.trainerClass = trainerClass;
    sChallenger.trainerPic = pic;
    sChallenger.gender = female;
    sChallenger.encounterMusic = female ? TRAINER_ENCOUNTER_MUSIC_FEMALE : TRAINER_ENCOUNTER_MUSIC_MALE;
    sChallenger.battleType = TRAINER_BATTLE_TYPE_SINGLES;
    StringCopyN(sChallenger.trainerName, name, TRAINER_NAME_LENGTH);
    sChallenger.trainerName[TRAINER_NAME_LENGTH] = EOS;
}

static void SetTexts(const u8 *intro, const u8 *defeat, const u8 *after, const u8 *win)
{
    StringExpandPlaceholders(gGymChallengeIntroText, intro);
    StringExpandPlaceholders(gGymChallengeDefeatText, defeat);
    StringExpandPlaceholders(gGymChallengeAfterText, after);
    StringExpandPlaceholders(gGymChallengeWinText, win);
}

static void GenerateTrainer(u32 level)
{
    const struct ChallengerClass *cls = &sClasses[Random() % ARRAY_COUNT(sClasses)];
    const u8 *name = cls->female ? sFemaleNames[Random() % ARRAY_COUNT(sFemaleNames)]
                                 : sMaleNames[Random() % ARRAY_COUNT(sMaleNames)];
    u32 n = 4 + Random() % 3, i, j;
    s8 spread[PARTY_SIZE];

    memcpy(spread, sLevelSpread[n], sizeof(spread));
    ShuffleLevels(spread, n);
    SetupTrainer(cls->trainerClass, cls->trainerPic, cls->female, name, n);
    for (i = 0; i < n; i++)
    {
        u32 species;
        do
        {
            species = 1 + Random() % SPECIES_DEOXYS;
            for (j = 0; j < i; j++)
            {
                if (sChallengerMons[j].species == species)
                    species = SPECIES_NONE;
            }
        } while (species == SPECIES_NONE || !IsStrongSpecies(species));
        InitMon(&sChallengerMons[i], species, ClampLevel(level + spread[i]), 24);
        if (Random() % 100 < 40)
            sChallengerMons[i].heldItem = sHeldItems[Random() % ARRAY_COUNT(sHeldItems)];
    }
    VarSet(VAR_OBJ_GFX_ID_0, cls->gfx);
    SetTexts(sTrainerIntros[Random() % ARRAY_COUNT(sTrainerIntros)],
             sTrainerDefeats[Random() % ARRAY_COUNT(sTrainerDefeats)],
             sTrainerAfters[Random() % ARRAY_COUNT(sTrainerAfters)],
             sTrainerWins[Random() % ARRAY_COUNT(sTrainerWins)]);
}

static void GenerateFamous(u32 index, u32 level)
{
    const struct FamousChallenger *famous = &sFamous[index];
    u32 n = 0, i, total = 0;
    s32 shift;

    if (famous->trainerId == 0)
    {
        // Oak doesn't battle in FireRed, so he gets a team here
        s8 spread[PARTY_SIZE];
        for (n = 0; sOakTeam[n] != SPECIES_NONE; n++)
            ;
        memcpy(spread, sLevelSpread[n], sizeof(spread));
        SetupTrainer(TRAINER_CLASS_PKMN_PROF_FRLG, TRAINER_PIC_PROFESSOR_OAK_FRLG, FALSE, COMPOUND_STRING("OAK"), n);
        for (i = 0; i < n; i++)
            InitMon(&sChallengerMons[i], sOakTeam[i], ClampLevel(level + spread[i]), 31);
    }
    else
    {
        const struct Trainer *source = GetTrainerStructFromId(famous->trainerId);
        n = min(source->partySize, PARTY_SIZE);
        if (n == 0)   // no team data (only in the test build): a random team, their lines
        {
            GenerateTrainer(level);
            VarSet(VAR_OBJ_GFX_ID_0, famous->gfx);
            SetTexts(famous->intro, famous->defeat, famous->after, famous->win);
            return;
        }
        SetupTrainer(source->trainerClass, source->trainerPic, source->gender,
                     famous->trainerId == TRAINER_CHAMPION_FIRST_SQUIRTLE ? gSaveBlock1Ptr->rivalName : source->trainerName, n);
        sChallenger.aiFlags = source->aiFlags | AI_FLAG_SMART_TRAINER;
        for (i = 0; i < n; i++)
        {
            sChallengerMons[i] = source->party[i];
            total += source->party[i].lvl;
        }
        // Same team, moved so its average level is the player's
        shift = (s32)level - (s32)((total + n / 2) / n);
        for (i = 0; i < n; i++)
        {
            sChallengerMons[i].lvl = ClampLevel(sChallengerMons[i].lvl + shift);
            sChallengerMons[i].species = GymChallenge_EvolveForLevel(sChallengerMons[i].species, sChallengerMons[i].lvl);
        }
        for (i = 0; famous->extras[i] != SPECIES_NONE && n < 4; i++, n++)
            InitMon(&sChallengerMons[n], famous->extras[i], level, 31);
        sChallenger.partySize = n;
    }
    VarSet(VAR_OBJ_GFX_ID_0, famous->gfx);
    SetTexts(famous->intro, famous->defeat, famous->after, famous->win);
}

static void GenerateTownsfolk(u32 index, u32 level)
{
    const struct Townsfolk *folk = &sTownsfolk[index];
    u32 n, i;
    s8 spread[PARTY_SIZE];
    for (n = 0; folk->team[n] != SPECIES_NONE; n++)
        ;
    memcpy(spread, sLevelSpread[n], sizeof(spread));
    ShuffleLevels(spread, n);
    SetupTrainer(folk->trainerClass, folk->trainerPic, folk->gender, folk->name, n);
    for (i = 0; i < n; i++)
        InitMon(&sChallengerMons[i], folk->team[i], ClampLevel(level + spread[i]), 31);
    VarSet(VAR_OBJ_GFX_ID_0, folk->gfx);
    SetTexts(folk->intro, folk->defeat, folk->after, folk->win);
}

// ---------------------------------------------------------------------------
// Oak's battle for the gym (data/scripts/gym_finale.inc): his real team, built to be hard.
// Levels: five above the player's average, and never below 65.
// ---------------------------------------------------------------------------
struct OakMon
{
    u16 species;
    u16 item;
    u8 nature;
    const u8 *evs;
    u16 moves[MAX_MON_MOVES];
};

static const u8 sEvs_PhysicalFast[] = {4, 252, 0, 0, 0, 252};   // HP Atk Def SpA SpD Spe
static const u8 sEvs_SpecialFast[] = {4, 0, 0, 252, 0, 252};
static const u8 sEvs_Bulky[] = {252, 4, 252, 0, 0, 0};

static const struct OakMon sOakFinaleTeam[] =
{
    {SPECIES_TAUROS,    ITEM_CHOICE_BAND, NATURE_JOLLY, sEvs_PhysicalFast,
        {MOVE_DOUBLE_EDGE, MOVE_EARTHQUAKE, MOVE_ROCK_SLIDE, MOVE_ZEN_HEADBUTT}},
    {SPECIES_EXEGGUTOR, ITEM_SITRUS_BERRY, NATURE_MODEST, sEvs_SpecialFast,
        {MOVE_PSYCHIC, MOVE_GIGA_DRAIN, MOVE_SLEEP_POWDER, MOVE_EXPLOSION}},
    {SPECIES_ARCANINE,  ITEM_LEFTOVERS, NATURE_ADAMANT, sEvs_PhysicalFast,
        {MOVE_FLARE_BLITZ, MOVE_EXTREME_SPEED, MOVE_CRUNCH, MOVE_WILL_O_WISP}},
    {SPECIES_GYARADOS,  ITEM_LUM_BERRY, NATURE_JOLLY, sEvs_PhysicalFast,
        {MOVE_DRAGON_DANCE, MOVE_WATERFALL, MOVE_EARTHQUAKE, MOVE_ICE_FANG}},
    {SPECIES_SNORLAX,   ITEM_LEFTOVERS, NATURE_CAREFUL, sEvs_Bulky,
        {MOVE_BODY_SLAM, MOVE_CURSE, MOVE_REST, MOVE_EARTHQUAKE}},
    {SPECIES_DRAGONITE, ITEM_LUM_BERRY, NATURE_ADAMANT, sEvs_PhysicalFast,
        {MOVE_DRAGON_DANCE, MOVE_OUTRAGE, MOVE_EXTREME_SPEED, MOVE_EARTHQUAKE}},
};

// ---------------------------------------------------------------------------
// Practice trainers (bought at the gym PC, data/scripts/gym_practice.inc): random teams of the
// player's type, no duplicates, 3-5 Pokémon, 5 levels below the player's average.
//   Rookie (beginner): mostly first forms, some second forms, a simple AI.
//   Skilled (intermediate): mostly second forms, some first forms, a decent AI.
//   Expert (advanced): fully evolved Pokémon (earlier forms only when there aren't enough), a smart AI.
// Legendaries are left out. Practice battles pay no money and can't be lost for real.
// ---------------------------------------------------------------------------
enum
{
    PRACTICE_ROOKIE,
    PRACTICE_SKILLED,
    PRACTICE_EXPERT,
    PRACTICE_TIERS
};

struct PracticeTier
{
    const u8 *title;
    u16 trainerClass;
    u16 trainerPic;
    bool8 female;
    u8 iv;
    u8 heldItemChance;
    u64 aiFlags;
    const u8 *defeat;
};

static const struct PracticeTier sPracticeTiers[PRACTICE_TIERS] =
{
    [PRACTICE_ROOKIE]  = {COMPOUND_STRING("ROOKIE"), TRAINER_CLASS_YOUNGSTER_FRLG, TRAINER_PIC_YOUNGSTER_FRLG, FALSE, 5, 0,
                          AI_FLAG_CHECK_BAD_MOVE, COMPOUND_STRING("Wow, LEADER!\nYou're so strong!")},
    [PRACTICE_SKILLED] = {COMPOUND_STRING("SKILLED"), TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_F_FRLG, TRUE, 16, 20,
                          AI_FLAG_BASIC_TRAINER, COMPOUND_STRING("Good battle, LEADER!\nI learned a lot.")},
    [PRACTICE_EXPERT]  = {COMPOUND_STRING("EXPERT"), TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_M_FRLG, FALSE, 31, 60,
                          AI_FLAG_SMART_TRAINER, COMPOUND_STRING("That's why you're\nthe LEADER.")},
};

// How many times a species has evolved (pre: each species' pre-evolution, from BuildPreEvolutions)
static u32 EvolutionStage(const u16 *pre, u32 species)
{
    u32 stage = 0;
    while (stage < 3 && (species = pre[species]) != SPECIES_NONE)
        stage++;
    return stage;
}

// pre[species] = what it evolves from, for the first three generations (one pass over the
// evolutions: GetSpeciesPreEvolution searches every species each time)
static void BuildPreEvolutions(u16 *pre)
{
    u32 species, i;
    memset(pre, 0, (SPECIES_DEOXYS + 1) * sizeof(u16));
    for (species = 1; species <= SPECIES_DEOXYS; species++)
    {
        const struct Evolution *evos = GetSpeciesEvolutions(species);
        if (evos == NULL)
            continue;
        for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            if (evos[i].targetSpecies <= SPECIES_DEOXYS)
                pre[evos[i].targetSpecies] = species;
        }
    }
}

// type NUMBER_OF_MON_TYPES: any type
static bool32 IsPracticeSpecies(u32 species, u32 type)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return IsSpeciesEnabled(species) && (type >= NUMBER_OF_MON_TYPES || info->types[0] == type)
        && !info->isRestrictedLegendary && !info->isSubLegendary && !info->isMythical;
}

// Which list a species goes in for a tier: 0 = what the tier is about, 1 = also fine, 2 = only if
// there's nothing else
static u32 PracticePreference(const u16 *pre, u32 species, u32 tier)
{
    u32 stage = EvolutionStage(pre, species);
    switch (tier)
    {
    case PRACTICE_ROOKIE:
        return stage == 0 ? 0 : (stage == 1 ? 1 : 2);
    case PRACTICE_SKILLED:
        return stage == 1 ? 0 : (stage == 0 ? 1 : 2);
    default:
        return !HasEvolution(species) ? 0 : (stage >= 1 ? 1 : 2);
    }
}

// Fills sChallengerMons with a practice team; returns its size
static u32 BuildTeam(u32 tier, u32 type, u32 level, u32 count);

u32 GymChallenge_BuildPracticeTeam(u32 tier, u32 type, u32 level)
{
    return BuildTeam(tier, type, level, 0);
}

// count 0: 3-5 Pokémon
static u32 BuildTeam(u32 tier, u32 type, u32 level, u32 count)
{
    u16 *lists[3];
    u16 *pre = Alloc((SPECIES_DEOXYS + 1) * sizeof(u16));
    u32 counts[3] = {0}, species = SPECIES_NONE, i, k, n, total;
    s8 spread[PARTY_SIZE];

    BuildPreEvolutions(pre);
    for (k = 0; k < 3; k++)
        lists[k] = Alloc(SPECIES_DEOXYS * sizeof(u16));
    for (species = 1; species <= SPECIES_DEOXYS; species++)
    {
        if (IsPracticeSpecies(species, type))
        {
            k = PracticePreference(pre, species, tier);
            lists[k][counts[k]++] = species;
        }
    }
    total = counts[0] + counts[1] + counts[2];
    n = count != 0 ? min(count, (u32)PARTY_SIZE) : 3 + Random() % 3;
    if (n > total)
        n = total;
    memcpy(spread, sLevelSpread[n], sizeof(spread));
    ShuffleLevels(spread, n);
    for (i = 0; i < n; i++)
    {
        // The rookie and skilled trainers mix in the other early form now and then
        u32 order[3] = {0, 1, 2};
        if (tier != PRACTICE_EXPERT && Random() % 3 == 0)
        {
            order[0] = 1;
            order[1] = 0;
        }
        for (k = 0; k < 3; k++)
        {
            u32 *count = &counts[order[k]];
            if (*count != 0)
            {
                // Take one at random (and drop it from the list: no duplicates)
                u16 *list = lists[order[k]];
                u32 j = Random() % *count;
                species = list[j];
                list[j] = list[--*count];
                break;
            }
        }
        InitMon(&sChallengerMons[i], species, ClampLevel((s32)level + spread[i]), sPracticeTiers[tier].iv);
        if (Random() % 100 < sPracticeTiers[tier].heldItemChance)
            sChallengerMons[i].heldItem = sHeldItems[Random() % ARRAY_COUNT(sHeldItems)];
    }
    for (k = 0; k < 3; k++)
        Free(lists[k]);
    Free(pre);
    return n;
}

// Each of the player's trainers keeps the same name (instance: how many of that rank came before)
static const u8 *PracticeName(u32 tier, u32 instance)
{
    u32 id = gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8);
    if (sPracticeTiers[tier].female)
        return sFemaleNames[(id + tier * 7 + instance * 5) % ARRAY_COUNT(sFemaleNames)];
    return sMaleNames[(id + tier * 11 + instance * 5) % ARRAY_COUNT(sMaleNames)];
}

// VAR_0x8004 = the trainer's slot in the gym -> their decoration and which one of it
static void GetPracticeSlot(u32 *decor, u32 *instance)
{
    s32 x, y;
    if (!GymDecor_GetTrainerSlot(gSpecialVar_0x8004, decor, instance, &x, &y))
    {
        *decor = GYM_DECOR_TRAINER_BEGINNER;
        *instance = 0;
    }
}

static u32 DecorTier(u32 decor)
{
    if (decor >= GYM_DECOR_TRAINER_BEGINNER && decor <= GYM_DECOR_TRAINER_ADVANCED)
        return decor - GYM_DECOR_TRAINER_BEGINNER;
    return PRACTICE_SKILLED;
}

// Special: VAR_0x8004 = slot -> STR_VAR_1 = the trainer's name, STR_VAR_2 = their title
void GymChallenge_BufferPracticeTrainer(void)
{
    u32 decor, instance;
    const struct GymRecruit *recruit;
    GetPracticeSlot(&decor, &instance);
    recruit = GymCandidate_GetRecruitByDecor(decor);
    if (recruit != NULL)
    {
        StringCopy(gStringVar1, GetTrainerNameFromId(recruit->trainerId));
        StringCopy(gStringVar2, gTrainerClasses[GetTrainerClassFromId(recruit->trainerId)].name);
        return;
    }
    StringCopy(gStringVar1, PracticeName(DecorTier(decor), instance));
    StringCopy(gStringVar2, sPracticeTiers[DecorTier(decor)].title);
    StringAppend(gStringVar2, COMPOUND_STRING(" TRAINER"));
}

// Builds a practice trainer of a rank into TRAINER_GYM_CHALLENGER
void GymChallenge_GeneratePracticeTier(u32 tier, u32 instance)
{
    const struct PracticeTier *t;
    s32 level = (s32)GymChallenge_PlayerAverageLevel() - 5;
    u32 n;

    tier = min(tier, PRACTICE_TIERS - 1);
    t = &sPracticeTiers[tier];
    SetupTrainer(t->trainerClass, t->trainerPic, t->female, PracticeName(tier, instance), 1);
    n = GymChallenge_BuildPracticeTeam(tier, VarGet(VAR_PLAYER_TYPE), max(level, 2));
    sChallenger.partySize = max(n, 1u);
    sChallenger.aiFlags = t->aiFlags;
    StringExpandPlaceholders(gGymChallengeDefeatText, t->defeat);
    gGymPracticeBattle = TRUE;
    gGymGuestBattle = FALSE;
}

// A trainer who joined from the road: their own name and look, a team of their favorite type
void GymChallenge_GenerateRecruit(u32 type)
{
    const struct GymRecruit *recruit = GymCandidate_GetRecruit(type);
    s32 level = (s32)GymChallenge_PlayerAverageLevel() - 5;
    u32 n;
    if (recruit == NULL)
        return;
    SetupTrainer(GetTrainerClassFromId(recruit->trainerId), GetTrainerPicFromId(recruit->trainerId),
                 recruit->female, GetTrainerNameFromId(recruit->trainerId), 1);
    n = GymChallenge_BuildPracticeTeam(PRACTICE_SKILLED, type, max(level, 2));
    sChallenger.partySize = max(n, 1u);
    sChallenger.aiFlags = AI_FLAG_BASIC_TRAINER;
    StringExpandPlaceholders(gGymChallengeDefeatText, COMPOUND_STRING("Ha! Good practice, LEADER!"));
    gGymPracticeBattle = TRUE;
    gGymGuestBattle = FALSE;
}

// Special: VAR_0x8004 = slot
void GymChallenge_GeneratePractice(void)
{
    u32 decor, instance, t;
    GetPracticeSlot(&decor, &instance);
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (gGymRecruits[t].decor != 0 && gGymRecruits[t].decor == decor)
        {
            GymChallenge_GenerateRecruit(t);
            return;
        }
    }
    GymChallenge_GeneratePracticeTier(DecorTier(decor), instance);
}

// ---------------------------------------------------------------------------
// Guest Leader battles (src/gym_candidate.c): a challenger at another gym while the player
// stands in for its leader (STR_VAR_3 = the leader's name). Any type, no legendaries.
// ---------------------------------------------------------------------------
static const u8 *const sGuestIntros[] =
{
    COMPOUND_STRING("Huh? Where's {STR_VAR_3}?\pYou're filling in? Then you're the\none I have to beat!"),
    COMPOUND_STRING("I came all this way to battle\n{STR_VAR_3}…\pFine! A stand-in will do!"),
    COMPOUND_STRING("A substitute LEADER?\nThis BADGE is as good as mine!"),
    COMPOUND_STRING("{STR_VAR_3} sent YOU? Is this\nsome kind of test?\pWhatever. Let's battle!"),
};
static const u8 *const sGuestDefeats[] =
{
    COMPOUND_STRING("You're tougher than\nyou look!"),
    COMPOUND_STRING("So that's a LEADER's\nstrength…"),
    COMPOUND_STRING("Some substitute!"),
};
static const u8 *const sGuestAfters[] =
{
    COMPOUND_STRING("Tell {STR_VAR_3} I'll be back…\nfor both of you!"),
    COMPOUND_STRING("You'd make a great LEADER.\nNo, really!"),
    COMPOUND_STRING("Man… I'll train harder and\ncome back!"),
};
static const u8 *const sGuestWins[] =
{
    COMPOUND_STRING("Ha! Some stand-in!\nGo get {STR_VAR_3}!"),
    COMPOUND_STRING("Better luck next time,\nsub!"),
};

void GymChallenge_GenerateGuest(u32 baseLevel, u32 partySize, u32 tier)
{
    const struct ChallengerClass *cls = &sClasses[Random() % ARRAY_COUNT(sClasses)];
    const u8 *name = cls->female ? sFemaleNames[Random() % ARRAY_COUNT(sFemaleNames)]
                                 : sMaleNames[Random() % ARRAY_COUNT(sMaleNames)];
    s32 level = max((s32)baseLevel, (s32)GymChallenge_PlayerAverageLevel() - 3);
    u32 n;

    SetupTrainer(cls->trainerClass, cls->trainerPic, cls->female, name, 1);
    n = BuildTeam(min(tier, PRACTICE_TIERS - 1), NUMBER_OF_MON_TYPES, level, partySize);
    sChallenger.partySize = max(n, 1u);
    sChallenger.aiFlags = sPracticeTiers[min(tier, PRACTICE_TIERS - 1)].aiFlags;
    VarSet(VAR_OBJ_GFX_ID_0, cls->gfx);
    SetTexts(sGuestIntros[Random() % ARRAY_COUNT(sGuestIntros)],
             sGuestDefeats[Random() % ARRAY_COUNT(sGuestDefeats)],
             sGuestAfters[Random() % ARRAY_COUNT(sGuestAfters)],
             sGuestWins[Random() % ARRAY_COUNT(sGuestWins)]);
    gGymPracticeBattle = FALSE;
    gGymGuestBattle = TRUE;        // the challenger pays the stand-in LEADER a Gym Leader's prize
}

// Special: after a practice battle
void GymChallenge_EndPractice(void)
{
    gGymPracticeBattle = FALSE;
    gGymGuestBattle = FALSE;
}

u32 GymChallenge_OakFinaleLevel(void)
{
    u32 level = GymChallenge_PlayerAverageLevel() + 5;
    if (level < 65)
        level = 65;
    return ClampLevel(level);
}

// Special
void GymChallenge_GenerateOakFinale(void)
{
    gGymPracticeBattle = FALSE;
    gGymGuestBattle = FALSE;
    u32 level = GymChallenge_OakFinaleLevel(), i;
    SetupTrainer(TRAINER_CLASS_PKMN_PROF_FRLG, TRAINER_PIC_PROFESSOR_OAK_FRLG, FALSE, COMPOUND_STRING("OAK"),
                 ARRAY_COUNT(sOakFinaleTeam));
    sChallenger.items[0] = ITEM_FULL_RESTORE;
    sChallenger.items[1] = ITEM_FULL_RESTORE;
    for (i = 0; i < ARRAY_COUNT(sOakFinaleTeam); i++)
    {
        const struct OakMon *oak = &sOakFinaleTeam[i];
        InitMon(&sChallengerMons[i], oak->species, level, MAX_PER_STAT_IVS);
        sChallengerMons[i].heldItem = oak->item;
        sChallengerMons[i].nature = oak->nature;
        sChallengerMons[i].ev = oak->evs;
        memcpy(sChallengerMons[i].moves, oak->moves, sizeof(oak->moves));
    }
    sKind = GYM_CHALLENGER_FAMOUS;
    StringExpandPlaceholders(gGymChallengeDefeatText, COMPOUND_STRING("Ho… ho ho…\nSplendid. Truly splendid."));
}

void GymChallenge_Generate(u32 kind, u32 index)
{
    gGymPracticeBattle = FALSE;
    gGymGuestBattle = FALSE;
    u32 level = GymChallenge_PlayerAverageLevel();
    sKind = kind;
    switch (kind)
    {
    case GYM_CHALLENGER_FAMOUS:
        GenerateFamous(index % ARRAY_COUNT(sFamous), level);
        break;
    case GYM_CHALLENGER_TOWNSFOLK:
        GenerateTownsfolk(index % ARRAY_COUNT(sTownsfolk), level);
        break;
    default:
        GenerateTrainer(level);
        break;
    }
}

// Special: picks and builds the next challenger. Every 7th challenger in a streak is famous;
// otherwise 1 in 6 is famous and 1 in 12 is someone from around town.
void GymChallenge_NextChallenger(void)
{
    u32 next = GymChallenge_GetSave()->streak + 1, roll = Random() % 12;
    if (next % 7 == 0 || roll < 2)
    {
        u32 index;
        do
            index = Random() % ARRAY_COUNT(sFamous);
        while (index == sLastFamous && ARRAY_COUNT(sFamous) > 1);
        sLastFamous = index;
        GymChallenge_Generate(GYM_CHALLENGER_FAMOUS, index);
    }
    else if (roll == 2)
    {
        GymChallenge_Generate(GYM_CHALLENGER_TOWNSFOLK, Random() % ARRAY_COUNT(sTownsfolk));
    }
    else
    {
        GymChallenge_Generate(GYM_CHALLENGER_TRAINER, 0);
    }
    gSpecialVar_Result = sKind;
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

static void BufferChallengerName(u8 *dest, u32 trainerClass, const u8 *name)
{
    dest = StringCopy(dest, gTrainerClasses[trainerClass].name);
    *dest++ = CHAR_SPACE;
    StringCopy(dest, name);
}

// Special: STR_VAR_1 = streak, STR_VAR_2 = best streak, STR_VAR_3 = "W wins, L losses"
void GymChallenge_BufferRecord(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    u8 *end;
    ConvertIntToDecimalStringN(gStringVar1, save->streak, STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, save->bestStreak, STR_CONV_MODE_LEFT_ALIGN, 5);
    end = ConvertIntToDecimalStringN(gStringVar3, save->wins, STR_CONV_MODE_LEFT_ALIGN, 5);
    end = StringCopy(end, COMPOUND_STRING("-"));
    ConvertIntToDecimalStringN(end, save->losses, STR_CONV_MODE_LEFT_ALIGN, 5);
}

static const u16 sRewardItems[] =
{
    // common
    ITEM_RARE_CANDY, ITEM_RARE_CANDY, ITEM_NUGGET, ITEM_NUGGET, ITEM_PP_UP, ITEM_FULL_RESTORE,
    ITEM_MAX_REVIVE, ITEM_MAX_ELIXIR, ITEM_HP_UP, ITEM_PROTEIN, ITEM_IRON, ITEM_CALCIUM, ITEM_ZINC,
    ITEM_CARBOS, ITEM_RARE_CANDY, ITEM_NUGGET,
    // less common
    ITEM_BOTTLE_CAP, ITEM_BOTTLE_CAP, ITEM_PP_MAX, ITEM_BIG_NUGGET,
};

static const u16 sRareRewardItems[] =
{
    ITEM_GOLD_BOTTLE_CAP, ITEM_ABILITY_CAPSULE, ITEM_CILANTRO, ITEM_MULTIVITAMIN, ITEM_MASTER_BALL,
};

u32 GymChallenge_RewardMoney(u32 streak, u32 kind)
{
    u32 money = 2000 + 200 * min(streak, 40u);
    if (kind != GYM_CHALLENGER_TRAINER)
        money *= 2;
    return money;
}

// Special, after a win: counts it, pays out and picks the item. VAR_RESULT = TRUE on a new best
// streak; VAR_0x8009 = the item; STR_VAR_1 = money, STR_VAR_2 = streak, STR_VAR_3 = challenger.
void GymChallenge_RecordWin(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    u32 money;

    if (save->streak < 0xFFFF)
        save->streak++;
    if (save->wins < 0xFFFF)
        save->wins++;
    gSpecialVar_Result = save->streak > save->bestStreak;
    if (save->streak > save->bestStreak)
        save->bestStreak = save->streak;

    money = GymChallenge_RewardMoney(save->streak, sKind);
    AddMoney(&gSaveBlock1Ptr->money, money);
    // Famous challengers and every 10th win always leave something rare
    if (sKind != GYM_CHALLENGER_TRAINER || save->streak % 10 == 0 || Random() % 16 == 0)
        gSpecialVar_0x8009 = sRareRewardItems[Random() % ARRAY_COUNT(sRareRewardItems)];
    else
        gSpecialVar_0x8009 = sRewardItems[Random() % ARRAY_COUNT(sRewardItems)];

    ConvertIntToDecimalStringN(gStringVar1, money, STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, save->streak, STR_CONV_MODE_LEFT_ALIGN, 5);
    BufferChallengerName(gStringVar3, sChallenger.trainerClass, sChallenger.trainerName);
}

// Special, after a loss: ends the streak and puts the challenger on the statue.
// STR_VAR_1 = the streak that ended, STR_VAR_2 = best streak, STR_VAR_3 = challenger.
void GymChallenge_RecordLoss(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    struct GymChallengeVictor *victor;
    u32 i;

    if (save->losses < 0xFFFF)
        save->losses++;
    // Newest first; the oldest drops off when the list is full
    for (i = GYM_CHALLENGE_MAX_VICTORS - 1; i > 0; i--)
        save->victors[i] = save->victors[i - 1];
    victor = &save->victors[0];
    StringCopy(victor->name, sChallenger.trainerName);
    victor->trainerClass = sChallenger.trainerClass;
    victor->streak = save->streak;
    if (save->numVictors < 0xFFFF)
        save->numVictors++;

    ConvertIntToDecimalStringN(gStringVar1, save->streak, STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, save->bestStreak, STR_CONV_MODE_LEFT_ALIGN, 5);
    BufferChallengerName(gStringVar3, sChallenger.trainerClass, sChallenger.trainerName);
    sLastStreak = save->streak;
    save->streak = 0;
}

// Special: the loss again (STR_VAR_1 = the streak that ended, STR_VAR_2 = best streak)
void GymChallenge_BufferLastResult(void)
{
    ConvertIntToDecimalStringN(gStringVar1, sLastStreak, STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, GymChallenge_GetSave()->bestStreak, STR_CONV_MODE_LEFT_ALIGN, 5);
}

// ---------------------------------------------------------------------------
// Getting everyone to the battle stage
// ---------------------------------------------------------------------------
// The leader stands on the stage's top mark and challengers on the bottom one (see BATTLE_STAGE
// in tools/gym_hack/build_gym_decor.py). With no stage, they battle below the aide. The walks are
// found with a breadth-first search over the room and written as movement scripts here.

#define PATH_MAX_STEPS 120

EWRAM_DATA u8 gGymChallengePlayerPath[PATH_MAX_STEPS + 2];
EWRAM_DATA u8 gGymChallengeChallengerPath[PATH_MAX_STEPS + 2];
EWRAM_DATA static s8 sLeaderX, sLeaderY, sChallengerX, sChallengerY;

static bool32 IsMat(s32 x, s32 y)
{
    return y == GYM_ROOM_HEIGHT - 2 && x >= GYM_MAT_X0 && x <= GYM_MAT_X1;
}

static bool32 IsPassable(s32 x, s32 y, s32 blockX, s32 blockY)
{
    s32 mx = x + MAP_OFFSET, my = y + MAP_OFFSET;
    if (x < 0 || y < 0 || x >= GYM_ROOM_WIDTH || y >= GYM_ROOM_HEIGHT)
        return FALSE;
    if ((x == blockX && y == blockY) || (x == GYM_AIDE_X && y == GYM_AIDE_Y))
        return FALSE;
    if (MapGridGetCollisionAt(mx, my) || MapGridGetElevationAt(mx, my) != 3)
        return FALSE;
    // No walking over spinners (a scripted walk doesn't spin, and it would look wrong)
    return !MetatileBehavior_IsForcedMovementTile(MapGridGetMetatileBehaviorAt(mx, my));
}

// Writes the walk from (fromX, fromY) to (toX, toY) into `out`, ending with a turn to `face`.
// (blockX, blockY) is a square someone else stands on. Returns FALSE if there's no way.
bool32 GymChallenge_FindPath(s32 fromX, s32 fromY, s32 toX, s32 toY, s32 blockX, s32 blockY, bool32 allowMat, u8 face, u8 *out)
{
    static const s8 dx[] = {0, 0, -1, 1}, dy[] = {1, -1, 0, 0};
    static const u8 walk[] = {MOVEMENT_ACTION_WALK_NORMAL_DOWN, MOVEMENT_ACTION_WALK_NORMAL_UP,
                              MOVEMENT_ACTION_WALK_NORMAL_LEFT, MOVEMENT_ACTION_WALK_NORMAL_RIGHT};
    u32 cells = GYM_ROOM_WIDTH * GYM_ROOM_HEIGHT;
    s16 *from = Alloc(cells * sizeof(s16));
    u8 *dir = Alloc(cells);
    s16 *queue = Alloc(cells * sizeof(s16));
    s32 head = 0, tail = 0, i, n = 0, cur;
    bool32 found = FALSE;

    for (i = 0; i < (s32)cells; i++)
        from[i] = -1;
    if (fromX < 0 || fromY < 0 || fromX >= GYM_ROOM_WIDTH || fromY >= GYM_ROOM_HEIGHT)
        goto done;
    from[fromX + fromY * GYM_ROOM_WIDTH] = fromX + fromY * GYM_ROOM_WIDTH;
    queue[tail++] = fromX + fromY * GYM_ROOM_WIDTH;
    while (head < tail)
    {
        s32 x, y;
        cur = queue[head++];
        x = cur % GYM_ROOM_WIDTH;
        y = cur / GYM_ROOM_WIDTH;
        if (x == toX && y == toY)
        {
            found = TRUE;
            break;
        }
        for (i = 0; i < 4; i++)
        {
            s32 nx = x + dx[i], ny = y + dy[i], next = nx + ny * GYM_ROOM_WIDTH;
            if (nx < 0 || ny < 0 || nx >= GYM_ROOM_WIDTH || ny >= GYM_ROOM_HEIGHT || from[next] >= 0)
                continue;
            if (!IsPassable(nx, ny, blockX, blockY) || (!allowMat && IsMat(nx, ny)))
                continue;
            from[next] = cur;
            dir[next] = i;
            queue[tail++] = next;
        }
    }
    if (found)
    {
        // Walk back from the goal to count the steps, then write them in order
        s32 steps = 0, c;
        for (c = cur; c != fromX + fromY * GYM_ROOM_WIDTH; c = from[c])
            steps++;
        if (steps > PATH_MAX_STEPS)
        {
            found = FALSE;
            goto done;
        }
        n = steps;
        for (c = cur; c != fromX + fromY * GYM_ROOM_WIDTH; c = from[c])
            out[--steps] = walk[dir[c]];
    }
done:
    out[n] = face;
    out[n + 1] = MOVEMENT_ACTION_STEP_END;
    Free(from);
    Free(dir);
    Free(queue);
    return found;
}

// Special: plans the player's walk to the leader's spot (into gGymChallengePlayerPath).
// VAR_RESULT: 1 = to the battle stage, 2 = below the aide (no stage placed),
// 3 = below the aide (the stage can't be reached), 0 = nowhere reachable.
void GymChallenge_PlanPlayerWalk(void)
{
    s16 px, py;
    s32 sx, sy;
    PlayerGetDestCoords(&px, &py);
    px -= MAP_OFFSET;
    py -= MAP_OFFSET;
    if (GymDecor_FindPlaced(GYM_DECOR_BATTLE_STAGE, &sx, &sy))
    {
        sLeaderX = sx + 1;
        sLeaderY = sy + 1;
        sChallengerX = sx + 1;
        sChallengerY = sy + 3;
        if (GymChallenge_FindPath(px, py, sLeaderX, sLeaderY, -1, -1, FALSE, MOVEMENT_ACTION_FACE_DOWN, gGymChallengePlayerPath)
         && GymChallenge_FindPath(GYM_CHALLENGER_START_X, GYM_CHALLENGER_START_Y, sChallengerX, sChallengerY,
                                  sLeaderX, sLeaderY, TRUE, MOVEMENT_ACTION_FACE_UP, gGymChallengeChallengerPath))
        {
            gSpecialVar_Result = 1;
            return;
        }
        gSpecialVar_Result = 3;
    }
    else
    {
        gSpecialVar_Result = 2;
    }
    sLeaderX = GYM_AIDE_X;
    sLeaderY = GYM_AIDE_Y + 1;
    sChallengerX = GYM_AIDE_X;
    sChallengerY = GYM_AIDE_Y + 2;
    if (!GymChallenge_FindPath(px, py, sLeaderX, sLeaderY, -1, -1, FALSE, MOVEMENT_ACTION_FACE_DOWN, gGymChallengePlayerPath))
        gSpecialVar_Result = 0;
}

// Special: plans a challenger's walk (into gGymChallengeChallengerPath). VAR_0x8004: 0 = in, from
// the door to their mark; 1 = back out. VAR_RESULT = FALSE if there's no way, and then
// VAR_0x8005/6 = their mark, to put them there directly.
void GymChallenge_PlanChallengerWalk(void)
{
    if (gSpecialVar_0x8004 == 0)
        gSpecialVar_Result = GymChallenge_FindPath(GYM_CHALLENGER_START_X, GYM_CHALLENGER_START_Y, sChallengerX, sChallengerY,
                                                   sLeaderX, sLeaderY, TRUE, MOVEMENT_ACTION_FACE_UP, gGymChallengeChallengerPath);
    else
        gSpecialVar_Result = GymChallenge_FindPath(sChallengerX, sChallengerY, GYM_CHALLENGER_START_X, GYM_CHALLENGER_START_Y,
                                                   sLeaderX, sLeaderY, TRUE, MOVEMENT_ACTION_FACE_DOWN, gGymChallengeChallengerPath);
    gSpecialVar_0x8005 = sChallengerX;
    gSpecialVar_0x8006 = sChallengerY;
}

// Special: the statue's list of challengers who beat the player, newest first, two to a page,
// into gStringVar4. VAR_RESULT = how many.
void GymChallenge_BufferVictorList(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    u32 i, n = min(save->numVictors, (u16)GYM_CHALLENGE_MAX_VICTORS);
    u8 *end = gStringVar4;
    for (i = 0; i < n; i++)
    {
        if (i > 0)
            *end++ = (i % 2) ? CHAR_NEWLINE : CHAR_PROMPT_CLEAR;
        BufferChallengerName(end, save->victors[i].trainerClass, save->victors[i].name);
        end += StringLength(end);
    }
    *end = EOS;
    gSpecialVar_Result = n;
}
