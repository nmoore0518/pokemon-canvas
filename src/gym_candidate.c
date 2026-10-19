// Gym Trainer hack: the player as a candidate for the Pallet Town Gym (data/scripts/gym_candidate.inc).
//
// To be named a Gym Leader, the League wants the approval of all eight Gym Leaders and the Elite
// Four. After a badge, each Gym Leader asks the player to cover their gym for a while: the player
// stands in for them against one challenger (two from Koga on), and wins their approval. The
// League's front door won't open without all eight. The Elite Four give theirs when they lose.
//
// A "mentor" is the Gym Leader or Elite Four member who shares (or is closest to) the player's
// type. Some are friendly, some are prickly about a kid who might outdo them; all come around
// once they've seen the player hold a gym (or beaten them).
//
// Trainers met on the road (one per type) offer to join the player's gym after losing to them;
// they become practice trainers (src/gym_decor.c, src/gym_challenge.c).
#include "global.h"
#include "gym_candidate.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "gym_challenge.h"
#include "gym_decor.h"
#include "string_util.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/opponents.h"
#include "constants/vars.h"

// ---------------------------------------------------------------------------
// The Gym Leaders
// ---------------------------------------------------------------------------
const struct GymCandidateLeader gGymCandidateLeaders[GYM_CANDIDATE_LEADERS] =
{
    [0] = {
        .name = COMPOUND_STRING("BROCK"), .approvalFlag = FLAG_GYM_APPROVAL_BROCK,
        .guests = 1, .level = 12, .partySize = 2, .tier = 0,
        .offer = COMPOUND_STRING("BROCK: So you want to be a GYM\nLEADER yourself? Good for you.\pBut a LEADER's job doesn't end\nwith one battle.\pChallengers come every day, and\nthey all expect the LEADER.\pHow about you watch my GYM for an\nafternoon? Prove you can hold it!"),
        .approve = COMPOUND_STRING("BROCK: Solid work! You held my\nGYM like a real LEADER.\pThe LEAGUE will hear that you have\nmy approval. Rock on, {PLAYER}!"),
    },
    [1] = {
        .name = COMPOUND_STRING("MISTY"), .approvalFlag = FLAG_GYM_APPROVAL_MISTY,
        .guests = 1, .level = 20, .partySize = 3, .tier = 0,
        .offer = COMPOUND_STRING("MISTY: A GYM of your own, huh?\nFine. Here's the deal.\pI need a break. You watch my GYM,\nand if a challenger shows up…\pyou beat them. Easy, right?\nUnless you're scared!"),
        .approve = COMPOUND_STRING("MISTY: Hmph. Not bad.\nNot bad at all.\pOK, OK! You've got my approval.\nDon't let it go to your head!"),
    },
    [2] = {
        .name = COMPOUND_STRING("LT. SURGE"), .approvalFlag = FLAG_GYM_APPROVAL_SURGE,
        .guests = 1, .level = 24, .partySize = 3, .tier = 1,
        .offer = COMPOUND_STRING("LT. SURGE: Hey, kid! Word is\nyou're after a GYM of your own!\pIn the army, we'd call this a\nfield test!\pYou hold my GYM against the next\nchallenger. Roger that?"),
        .approve = COMPOUND_STRING("LT. SURGE: Outstanding, soldier!\pYou've got my approval, kid!\nI'll radio it to the LEAGUE!"),
    },
    [3] = {
        .name = COMPOUND_STRING("ERIKA"), .approvalFlag = FLAG_GYM_APPROVAL_ERIKA,
        .guests = 1, .level = 29, .partySize = 4, .tier = 1,
        .offer = COMPOUND_STRING("ERIKA: Oh my… A GYM in PALLET\nTOWN? How lovely.\pWould you mind watching my GYM\nwhile I tend to the flowers?\pIf a challenger comes, please\ngreet them for me."),
        .approve = COMPOUND_STRING("ERIKA: How wonderful. You welcomed\nthem as a LEADER should.\pYou have my approval, of course.\nI shall tell the LEAGUE today."),
    },
    [4] = {
        .name = COMPOUND_STRING("KOGA"), .approvalFlag = FLAG_GYM_APPROVAL_KOGA,
        .guests = 2, .level = 37, .partySize = 4, .tier = 1,
        .offer = COMPOUND_STRING("KOGA: Fwahahaha! So the child\nwould be a LEADER?\pA ninja tests his student in\nsecret. Hold my GYM while I…\pwatch from the shadows. Two\nchallengers approach. Ready?"),
        .approve = COMPOUND_STRING("KOGA: …Hmm! You did not falter.\pVery well. KOGA, master of\nPOISON, grants his approval!"),
    },
    [5] = {
        .name = COMPOUND_STRING("SABRINA"), .approvalFlag = FLAG_GYM_APPROVAL_SABRINA,
        .guests = 2, .level = 40, .partySize = 4, .tier = 2,
        .offer = COMPOUND_STRING("SABRINA: I knew you would ask.\nI saw this moment a week ago.\pTwo challengers will come\nthrough my GYM today.\pYou will face them. I already\nknow how it ends… Do you?"),
        .approve = COMPOUND_STRING("SABRINA: …Just as I foresaw.\pYou have my approval. The\nLEAGUE knows it already."),
    },
    [6] = {
        .name = COMPOUND_STRING("BLAINE"), .approvalFlag = FLAG_GYM_APPROVAL_BLAINE,
        .guests = 2, .level = 44, .partySize = 5, .tier = 2,
        .offer = COMPOUND_STRING("BLAINE: Hah! A future LEADER!\nHere's a quiz for you!\pQuestion: who holds a GYM when\nits LEADER is out?\pAnswer: YOU do! Two challengers\nare on their way. Fire it up!"),
        .approve = COMPOUND_STRING("BLAINE: Correct answer! You've\ngot fire in you, kid!\pMy approval is yours!\nWahahahah!"),
    },
    [7] = {
        .name = COMPOUND_STRING("GIOVANNI"), .approvalFlag = FLAG_GYM_APPROVAL_GIOVANNI,
        .guests = 2, .level = 47, .partySize = 5, .tier = 2,
        .offer = COMPOUND_STRING("GIOVANNI: So. You want the\nLEAGUE's blessing as well.\pThen hold my GYM. Two\nchallengers are waiting.\pLet us see if you can protect\nwhat is yours."),
        .approve = COMPOUND_STRING("GIOVANNI: …You protect what is\nyours. I respect that.\pThe LEAGUE will have my approval.\nNow leave me."),
    },
};

static const struct
{
    const u8 *name;
    u16 trainerId;
    const u8 *approve;
} sEliteFour[GYM_CANDIDATE_ELITE_FOUR] =
{
    {COMPOUND_STRING("LORELEI"), TRAINER_ELITE_FOUR_LORELEI,
        COMPOUND_STRING("LORELEI: And you hope to run a\nGYM of your own, I hear.\pYou have my approval. The\nLEAGUE will need LEADERS like you.")},
    {COMPOUND_STRING("BRUNO"), TRAINER_ELITE_FOUR_BRUNO,
        COMPOUND_STRING("BRUNO: A GYM LEADER, eh?\nWith strength like that…\pYou have my approval! Hoo hah!")},
    {COMPOUND_STRING("AGATHA"), TRAINER_ELITE_FOUR_AGATHA,
        COMPOUND_STRING("AGATHA: Oak's little GYM, eh?\pFine, fine. You have my approval.\nNow shoo! Go on!")},
    {COMPOUND_STRING("LANCE"), TRAINER_ELITE_FOUR_LANCE,
        COMPOUND_STRING("LANCE: You battle like a LEADER\nalready.\pThe ELITE FOUR approves. Only the\nCHAMPION's room remains.")},
};

// ---------------------------------------------------------------------------
// Mentors: who mentors each type (a Gym Leader 0-7, or the Elite Four 8-11), what they say
// when they learn the player's type, and when they come around
// ---------------------------------------------------------------------------
#define LORELEI 8
#define BRUNO   9
#define AGATHA  10
#define LANCE   11

static const struct GymCandidateMentor sMentors[NUMBER_OF_MON_TYPES] =
{
    [TYPE_ROCK] = {0,
        COMPOUND_STRING("BROCK: Wait… you train ROCK types\ntoo?\pHa! That's great! There's always\nroom for one more rock-solid GYM.\pIf you ever want tips, you know\nwhere to find me!"),
        COMPOUND_STRING("BROCK: Two ROCK GYMS in KANTO!\nI couldn't be prouder, {PLAYER}.\pThe LEAGUE will hear that you have\nmy approval. Rock on!")},
    [TYPE_NORMAL] = {0,
        COMPOUND_STRING("BROCK: NORMAL types, huh?\pPeople underestimate them.\nNo fancy tricks, just basics.\pThat's how I train too. Stick\nwith it, {PLAYER}!"),
        COMPOUND_STRING("BROCK: Basics win battles. You\nproved it today!\pThe LEAGUE will hear that you have\nmy approval!")},
    [TYPE_WATER] = {1,
        COMPOUND_STRING("MISTY: Hold on. You train WATER\nPOKéMON?\pAnd you want a GYM of your own?\nRight next door?!\pThere's only room for ONE WATER\nGYM in KANTO, and it's mine!"),
        COMPOUND_STRING("MISTY: …Fine. FINE!\nYou're really good with them.\pMaybe KANTO can handle two WATER\nGYMS. Maybe.\pYou've got my approval. Now get\nout before I change my mind!")},
    [TYPE_ELECTRIC] = {2,
        COMPOUND_STRING("LT. SURGE: Hey! ELECTRIC types?\nThat's MY turf, kid!\pIn the war, ELECTRIC POKéMON saved\nmy life. You think you can just…\pTake over? Not on my watch!"),
        COMPOUND_STRING("LT. SURGE: Ha! I'll admit it, kid.\nYou've got a real spark.\pYou've got my approval! And if\nyou need parts, call me!")},
    [TYPE_STEEL] = {2,
        COMPOUND_STRING("LT. SURGE: STEEL types, huh? Like\nmy MAGNEMITE!\pTough as tanks. But a STEEL GYM?\nIn PALLET? I don't buy it, kid."),
        COMPOUND_STRING("LT. SURGE: Your GYM's built like\na fortress! I take it back!\pYou've got my approval, soldier!")},
    [TYPE_GRASS] = {3,
        COMPOUND_STRING("ERIKA: Oh! You love GRASS POKéMON\ntoo? How delightful!\pA GYM full of flowers in PALLET\nTOWN… I would love to see it.\pPlease, come visit my garden any\ntime you like."),
        COMPOUND_STRING("ERIKA: Your POKéMON were in full\nbloom today.\pYou have my approval, and my\nfriendship. Please, take care.")},
    [TYPE_POISON] = {4,
        COMPOUND_STRING("KOGA: …POISON? Hmph.\pA ninja guards his secrets\njealously, child.\pI will not teach a rival who\nwould open a GYM of my type!"),
        COMPOUND_STRING("KOGA: …Fwahaha. I was wrong to\ndoubt you.\pA true POISON master. You have\nmy approval… and my respect.")},
    [TYPE_BUG] = {4,
        COMPOUND_STRING("KOGA: BUG types? My VENOMOTH would\nbe flattered.\pBut a BUG GYM? Most think them\nweak. Prove them wrong, if you can."),
        COMPOUND_STRING("KOGA: Fwahaha! Your BUG POKéMON\nstrike like ninja.\pYou have my approval!")},
    [TYPE_PSYCHIC] = {5,
        COMPOUND_STRING("SABRINA: …PSYCHIC POKéMON.\pI foresaw a second PSYCHIC GYM\nin KANTO. I did not like it.\pI still do not. But visions do\nnot ask for permission."),
        COMPOUND_STRING("SABRINA: I saw a PSYCHIC GYM in\nPALLET TOWN. I resented it.\pNow I see why it was there.\nYou have my approval.")},
    [TYPE_FAIRY] = {5,
        COMPOUND_STRING("SABRINA: FAIRY POKéMON. Like my\nMR. MIME.\pThey have a mind of their own.\nAs do you, it seems."),
        COMPOUND_STRING("SABRINA: Your FAIRY POKéMON\ntrust you completely.\pYou have my approval.")},
    [TYPE_FIRE] = {6,
        COMPOUND_STRING("BLAINE: Hah! A FIRE trainer!\nThat makes my blood boil!\pIn a good way! Quiz: who's going\nto help you? Answer: ME!"),
        COMPOUND_STRING("BLAINE: That's the spirit! Two\nFIRE GYMS in KANTO!\pMy approval is yours, kid!\nWahahahah!")},
    [TYPE_GROUND] = {7,
        COMPOUND_STRING("GIOVANNI: You train GROUND types.\pVIRIDIAN's GYM is the GROUND GYM\nof KANTO. It always will be.\pDo not imagine you will take\nanything of mine."),
        COMPOUND_STRING("GIOVANNI: …You have a feel for the\nearth. I can't deny it.\pThe LEAGUE will have my approval.\nGuard your GYM well.")},
    [TYPE_DARK] = {7,
        COMPOUND_STRING("GIOVANNI: DARK POKéMON…\nInteresting.\pYou have the makings of a\nfearsome LEADER. Or something\lmore.\pBut a child is still a child."),
        COMPOUND_STRING("GIOVANNI: I underestimated you.\nI don't do that often.\pThe LEAGUE will have my approval.")},
    [TYPE_ICE] = {LORELEI,
        COMPOUND_STRING("LORELEI: …You train ICE POKéMON?\pHow refreshing. Few have the\npatience for them.\pShow me how cold you can be!"),
        COMPOUND_STRING("LORELEI: A fellow ICE trainer…\nand a fine one.\pYou have my approval. KANTO could\nuse an ICE GYM. Stay cool.")},
    [TYPE_FIGHTING] = {BRUNO,
        COMPOUND_STRING("BRUNO: You train FIGHTING types?\pHoo hah! Then let our POKéMON\ntalk with their fists!"),
        COMPOUND_STRING("BRUNO: Such power! You trained\nthem well!\pYou have my approval. Come train\nwith me any time!")},
    [TYPE_GHOST] = {AGATHA,
        COMPOUND_STRING("AGATHA: GHOST POKéMON?\nYou, a whippersnapper?\pAnd I hear you want a GYM in\nOak's backyard. Hah!\pI've trained GHOSTS since before\nyou were born. Let's see!"),
        COMPOUND_STRING("AGATHA: …Hmph. You've got the\ngift, child. I'll grant you that.\pYou have my approval. Don't make\nme regret it!")},
    [TYPE_DRAGON] = {LANCE,
        COMPOUND_STRING("LANCE: You raise DRAGONS?\pThey are the pride of the\nELITE FOUR. My pride.\pA child with a DRAGON GYM?\nThen prove you're worthy!"),
        COMPOUND_STRING("LANCE: Your DRAGONS fly true.\nI was wrong about you.\pYou have my approval, DRAGON\nmaster.")},
    [TYPE_FLYING] = {LANCE,
        COMPOUND_STRING("LANCE: FLYING POKéMON. My\nDRAGONITE rules the skies.\pLet's see if yours can keep up!"),
        COMPOUND_STRING("LANCE: Your POKéMON own the sky.\pYou have my approval.")},
};

// ---------------------------------------------------------------------------
// Approvals
// ---------------------------------------------------------------------------
u32 GymCandidate_CountApprovals(void)
{
    u32 i, n = 0;
    for (i = 0; i < GYM_CANDIDATE_LEADERS; i++)
    {
        if (FlagGet(gGymCandidateLeaders[i].approvalFlag))
            n++;
    }
    return n;
}

static u32 Leader(void)
{
    return min(gSpecialVar_0x8009, GYM_CANDIDATE_LEADERS - 1);
}

// Special: VAR_RESULT = how many Gym Leaders approve; STR_VAR_1 = the same as text
void GymCandidate_BufferApprovals(void)
{
    gSpecialVar_Result = GymCandidate_CountApprovals();
    ConvertIntToDecimalStringN(gStringVar1, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 1);
}

// Special: VAR_0x8009 = leader -> VAR_RESULT = whether they've approved
void GymCandidate_IsApproved(void)
{
    gSpecialVar_Result = FlagGet(gGymCandidateLeaders[Leader()].approvalFlag);
}

// Special: VAR_0x8009 = leader -> gStringVar4 = their request
void GymCandidate_BufferOffer(void)
{
    StringExpandPlaceholders(gStringVar4, gGymCandidateLeaders[Leader()].offer);
}

// Special: VAR_0x8009 = leader, VAR_0x800B = challengers beaten so far -> VAR_RESULT = whether
// another one is coming
void GymCandidate_MoreGuests(void)
{
    gSpecialVar_Result = gSpecialVar_0x800B < gGymCandidateLeaders[Leader()].guests;
}

// Special: VAR_0x8009 = leader, VAR_0x800B = challengers beaten so far. Builds the next one.
void GymCandidate_GenerateGuest(void)
{
    const struct GymCandidateLeader *leader = &gGymCandidateLeaders[Leader()];
    StringCopy(gStringVar3, leader->name);
    GymChallenge_GenerateGuest(leader->level, leader->partySize, leader->tier);
}

// The mentor for the player's type, if it's this leader (0-11)
static const struct GymCandidateMentor *GetMentor(u32 who)
{
    u32 type = VarGet(VAR_PLAYER_TYPE);
    if (type >= NUMBER_OF_MON_TYPES || sMentors[type].first == NULL || sMentors[type].who != who)
        return NULL;
    return &sMentors[type];
}

const struct GymCandidateMentor *GymCandidate_GetMentorForType(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || sMentors[type].first == NULL)
        return NULL;
    return &sMentors[type];
}

// Special: VAR_0x8009 = leader (0-7) or Elite Four (8-11). If they mentor the player's type:
// gStringVar4 = their first reaction, VAR_RESULT = TRUE
void GymCandidate_BufferMentorFirst(void)
{
    const struct GymCandidateMentor *mentor = GetMentor(gSpecialVar_0x8009);
    gSpecialVar_Result = (mentor != NULL);
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, mentor->first);
}

// Special: VAR_0x8009 = leader. Approves; gStringVar4 = what they say (the mentor's version if
// they mentor the player's type); STR_VAR_1 = approvals so far
void GymCandidate_Approve(void)
{
    const struct GymCandidateMentor *mentor = GetMentor(Leader());
    FlagSet(gGymCandidateLeaders[Leader()].approvalFlag);
    StringExpandPlaceholders(gStringVar4, mentor != NULL ? mentor->approve : gGymCandidateLeaders[Leader()].approve);
    ConvertIntToDecimalStringN(gStringVar1, GymCandidate_CountApprovals(), STR_CONV_MODE_LEFT_ALIGN, 1);
}

// Special: VAR_0x8009 = Elite Four member (8-11) -> gStringVar4 = their approval
void GymCandidate_BufferEliteFourApproval(void)
{
    u32 who = gSpecialVar_0x8009;
    const struct GymCandidateMentor *mentor = GetMentor(who);
    who = min(who, (u32)LANCE) - LORELEI;
    StringExpandPlaceholders(gStringVar4, mentor != NULL ? mentor->approve : sEliteFour[who].approve);
}

const u8 *GymCandidate_GetEliteFourApproval(u32 i)
{
    return sEliteFour[i].approve;
}

// ---------------------------------------------------------------------------
// Where the challenger stands: next to the player, preferably behind them (the leader is in
// front, watching). VAR_0x8005/6 = the spot, VAR_RESULT = whether there is one.
// ---------------------------------------------------------------------------
static const s8 sDirDx[] = {[DIR_SOUTH] = 0, [DIR_NORTH] = 0, [DIR_WEST] = -1, [DIR_EAST] = 1};
static const s8 sDirDy[] = {[DIR_SOUTH] = 1, [DIR_NORTH] = -1, [DIR_WEST] = 0, [DIR_EAST] = 0};
static const u8 sOpposite[] = {[DIR_SOUTH] = DIR_NORTH, [DIR_NORTH] = DIR_SOUTH, [DIR_WEST] = DIR_EAST, [DIR_EAST] = DIR_WEST};

static EWRAM_DATA u8 sGuestDir;   // where the challenger is, seen from the player

static bool32 IsFreeSpot(s16 x, s16 y, u8 elevation)
{
    u8 e = MapGridGetElevationAt(x, y);
    if (MapGridGetCollisionAt(x, y))
        return FALSE;
    if (GetObjectEventIdByXY(x, y) != OBJECT_EVENTS_COUNT)
        return FALSE;
    return e == elevation || e == 0 || e == 15 || elevation == 0 || elevation == 15;
}

void GymCandidate_PlaceGuest(void)
{
    s16 px, py;
    u32 i;
    u8 facing = GetPlayerFacingDirection();
    u8 order[4];
    u8 elevation;

    PlayerGetDestCoords(&px, &py);
    elevation = MapGridGetElevationAt(px, py);
    order[0] = sOpposite[facing];
    order[1] = (facing == DIR_NORTH || facing == DIR_SOUTH) ? DIR_WEST : DIR_NORTH;
    order[2] = sOpposite[order[1]];
    order[3] = facing;
    gSpecialVar_Result = FALSE;
    for (i = 0; i < 4; i++)
    {
        s16 x = px + sDirDx[order[i]], y = py + sDirDy[order[i]];
        if (IsFreeSpot(x, y, elevation))
        {
            gSpecialVar_0x8005 = x - MAP_OFFSET;
            gSpecialVar_0x8006 = y - MAP_OFFSET;
            sGuestDir = order[i];
            gSpecialVar_Result = TRUE;
            return;
        }
    }
}

// Special: VAR_0x800A = the challenger's local id. The player and challenger face each other.
void GymCandidate_FaceGuest(void)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(gSpecialVar_0x800A, gSaveBlock1Ptr->location.mapNum,
                                            gSaveBlock1Ptr->location.mapGroup, &objectEventId))
        ObjectEventTurn(&gObjectEvents[objectEventId], sOpposite[sGuestDir]);
    ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], sGuestDir);
}

// ---------------------------------------------------------------------------
// Trainers who join the player's gym (data/scripts/gym_candidate.inc offers, after battle)
// ---------------------------------------------------------------------------
const struct GymRecruit gGymRecruits[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = {GYM_DECOR_RECRUIT_NORMAL, TRAINER_LASS_ANDREA, OBJ_EVENT_GFX_LASS_FRLG, TRUE,
        COMPOUND_STRING("ANDREA: You're building a GYM?\nA real one?\pCould my MEOWTH and I join?\nWe'd practice with you every day!")},
    [TYPE_FIGHTING] = {GYM_DECOR_RECRUIT_FIGHTING, TRAINER_BLACK_BELT_HITOSHI, OBJ_EVENT_GFX_BLACK_BELT_FRLG, FALSE,
        COMPOUND_STRING("HITOSHI: Your strength is real!\pThe DOJO taught me all it can.\nLet me train at your GYM!")},
    [TYPE_FLYING]   = {GYM_DECOR_RECRUIT_FLYING, TRAINER_BIRD_KEEPER_PERRY, OBJ_EVENT_GFX_ROCKER, FALSE,
        COMPOUND_STRING("PERRY: You fly high, friend!\pIs it true you're starting a GYM?\nMy birds and I want in!")},
    [TYPE_POISON]   = {GYM_DECOR_RECRUIT_POISON, TRAINER_BIKER_JARED, OBJ_EVENT_GFX_BIKER, FALSE,
        COMPOUND_STRING("JARED: Man, you're tough!\pA GYM, huh? Need some muscle?\nMe and my KOFFING are in!")},
    [TYPE_GROUND]   = {GYM_DECOR_RECRUIT_GROUND, TRAINER_CAMPER_DREW, OBJ_EVENT_GFX_CAMPER_FRLG, FALSE,
        COMPOUND_STRING("DREW: Wow, you dug deep!\pYou're starting a GYM? Can my\nSANDSHREW and I join?")},
    [TYPE_ROCK]     = {GYM_DECOR_RECRUIT_ROCK, TRAINER_HIKER_MARCOS, OBJ_EVENT_GFX_HIKER_FRLG, FALSE,
        COMPOUND_STRING("MARCOS: Whoa! You're as hard as\nthese rocks!\pA GYM of your own? I'd climb a\nmountain to train there!")},
    [TYPE_BUG]      = {GYM_DECOR_RECRUIT_BUG, TRAINER_BUG_CATCHER_RICK, OBJ_EVENT_GFX_BUG_CATCHER_FRLG, FALSE,
        COMPOUND_STRING("RICK: Aww, you beat my bugs!\pHey… are you gonna be a GYM\nLEADER? Can I be in your GYM?")},
    [TYPE_GHOST]    = {GYM_DECOR_RECRUIT_GHOST, TRAINER_CHANNELER_ANGELICA, OBJ_EVENT_GFX_CHANNELER, TRUE,
        COMPOUND_STRING("ANGELICA: The spirits speak of\nyou… and a GYM in PALLET.\pThey wish to go there.\nMay I follow them?")},
    [TYPE_STEEL]    = {GYM_DECOR_RECRUIT_STEEL, TRAINER_ENGINEER_BERNIE, OBJ_EVENT_GFX_BALDING_MAN, FALSE,
        COMPOUND_STRING("BERNIE: You've got a solid frame!\pBuilding a GYM, are you? Every\nGYM needs an engineer!")},
    [TYPE_FIRE]     = {GYM_DECOR_RECRUIT_FIRE, TRAINER_GENTLEMAN_THOMAS, OBJ_EVENT_GFX_GENTLEMAN_FRLG, FALSE,
        COMPOUND_STRING("THOMAS: Splendid! My GROWLITHE\nhave not had such fun in years.\pI hear you plan to open a GYM.\nMight I offer my services?")},
    [TYPE_WATER]    = {GYM_DECOR_RECRUIT_WATER, TRAINER_FISHERMAN_NED, OBJ_EVENT_GFX_FISHER, FALSE,
        COMPOUND_STRING("NED: You reeled me right in!\pA GYM, eh? I'll trade my rod for\na spot in it any day!")},
    [TYPE_GRASS]    = {GYM_DECOR_RECRUIT_GRASS, TRAINER_PICNICKER_ALICIA, OBJ_EVENT_GFX_PICNICKER_FRLG, TRUE,
        COMPOUND_STRING("ALICIA: You're amazing!\pIf you're starting a GYM, can my\nflowers and I come too?")},
    [TYPE_ELECTRIC] = {GYM_DECOR_RECRUIT_ELECTRIC, TRAINER_ROCKER_LUCA, OBJ_EVENT_GFX_ROCKER, FALSE,
        COMPOUND_STRING("LUCA: Whoa, what a jolt!\pA GYM? That's electric! Let me\njoin the band!")},
    [TYPE_PSYCHIC]  = {GYM_DECOR_RECRUIT_PSYCHIC, TRAINER_JUGGLER_NELSON, OBJ_EVENT_GFX_ROCKER, FALSE,
        COMPOUND_STRING("NELSON: You saw through every\ntrick!\pA GYM of your own… I sense you'll\nneed a PSYCHIC. Me!")},
    [TYPE_ICE]      = {GYM_DECOR_RECRUIT_ICE, TRAINER_SWIMMER_FEMALE_NORA, OBJ_EVENT_GFX_SWIMMER_F_LAND, TRUE,
        COMPOUND_STRING("NORA: Brr! That was chilling!\pI swim near SEAFOAM, where the\nwater's ice-cold. Like my\lPOKéMON!\pCan I join your GYM?")},
    [TYPE_DRAGON]   = {GYM_DECOR_RECRUIT_DRAGON, TRAINER_SWIMMER_FEMALE_SHIRLEY, OBJ_EVENT_GFX_SWIMMER_F_LAND, TRUE,
        COMPOUND_STRING("SHIRLEY: Wow! My SEADRA will be\na KINGDRA one day, you'll see!\pA DRAGON type! Can I train at\nyour GYM until then?")},
    [TYPE_DARK]     = {GYM_DECOR_RECRUIT_DARK, TRAINER_BURGLAR_LEWIS, OBJ_EVENT_GFX_POKE_MANIAC_FRLG, FALSE,
        COMPOUND_STRING("LEWIS: You caught me fair and\nsquare.\pI'm done sneaking around. Maybe\nI'll go legit… with DARK types.\pGot room at your GYM?")},
    [TYPE_FAIRY]    = {GYM_DECOR_RECRUIT_FAIRY, TRAINER_LASS_JULIA, OBJ_EVENT_GFX_LASS_FRLG, TRUE,
        COMPOUND_STRING("JULIA: My CLEFAIRY think you're\nwonderful!\pIf you open a GYM, can we join?\nPlease, please?")},
};

const struct GymRecruit *GymCandidate_GetRecruit(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || gGymRecruits[type].decor == 0)
        return NULL;
    return &gGymRecruits[type];
}

// The recruit a trainer decoration stands for, or NULL
const struct GymRecruit *GymCandidate_GetRecruitByDecor(u32 decor)
{
    u32 t;
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (gGymRecruits[t].decor != 0 && gGymRecruits[t].decor == decor)
            return &gGymRecruits[t];
    }
    return NULL;
}

// Special: VAR_0x8004 = type -> VAR_RESULT = whether they've joined; gStringVar4 = their pitch;
// STR_VAR_1 = their name
void GymCandidate_BufferRecruit(void)
{
    const struct GymRecruit *recruit = GymCandidate_GetRecruit(gSpecialVar_0x8004);
    gSpecialVar_Result = TRUE;
    if (recruit == NULL)
        return;
    gSpecialVar_Result = GymDecor_CountOwned(recruit->decor) != 0;
    StringExpandPlaceholders(gStringVar4, recruit->pitch);
    StringCopy(gStringVar1, gGymDecor[recruit->decor].name);
}

// Special: VAR_0x8004 = type. They join: one of them goes to the gym's storage.
void GymCandidate_Recruit(void)
{
    const struct GymRecruit *recruit = GymCandidate_GetRecruit(gSpecialVar_0x8004);
    if (recruit != NULL)
        GymDecor_SetOwned(recruit->decor, 1);
}
