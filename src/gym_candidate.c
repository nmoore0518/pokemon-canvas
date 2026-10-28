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
#include "malloc.h"
#include "gym_candidate.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "metatile_behavior.h"
#include "gym_challenge.h"
#include "gym_decor.h"
#include "string_util.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/script_commands.h"
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
        .repeatOffer = COMPOUND_STRING("BROCK: Back again, {PLAYER}?\nPerfect timing!\pI've got a stew on the stove.\nMind watching the GYM for a bit?"),
        .repeatThanks = COMPOUND_STRING("BROCK: Thanks for covering,\n{PLAYER}! Come back any time."),
        .ownGym = COMPOUND_STRING("BROCK: Hey, LEADER! You've got\nyour own GYM to run now.\pYour challengers are waiting in\nPALLET TOWN!"),
        .observe = COMPOUND_STRING("BROCK: Great! I'll stand over here\nand watch. Pretend I'm not here!\pTake my spot, {PLAYER}!"),
    },
    [1] = {
        .name = COMPOUND_STRING("MISTY"), .approvalFlag = FLAG_GYM_APPROVAL_MISTY,
        .guests = 1, .level = 20, .partySize = 3, .tier = 0,
        .offer = COMPOUND_STRING("MISTY: A GYM of your own, huh?\nFine. Here's the deal.\pI need a break. You watch my GYM,\nand if a challenger shows up…\pyou beat them. Easy, right?\nUnless you're scared!"),
        .approve = COMPOUND_STRING("MISTY: Hmph. Not bad.\nNot bad at all.\pOK, OK! You've got my approval.\nDon't let it go to your head!"),
        .repeatOffer = COMPOUND_STRING("MISTY: Oh, it's you. Great timing!\pI'm going for a swim. You watch\nthe GYM… if you're up to it."),
        .repeatThanks = COMPOUND_STRING("MISTY: Hmph. Not bad. You can\ncover again sometime. Maybe."),
        .ownGym = COMPOUND_STRING("MISTY: Don't you have your own GYM\nnow? Go run it!\pI'm not sharing my challengers!"),
        .observe = COMPOUND_STRING("MISTY: I'll be right over there,\nwatching EVERYTHING.\pDon't you dare mess up my GYM!"),
    },
    [2] = {
        .name = COMPOUND_STRING("LT. SURGE"), .approvalFlag = FLAG_GYM_APPROVAL_SURGE,
        .guests = 1, .level = 24, .partySize = 3, .tier = 1,
        .offer = COMPOUND_STRING("LT. SURGE: Hey, kid! Word is\nyou're after a GYM of your own!\pIn the army, we'd call this a\nfield test!\pYou hold my GYM against the next\nchallenger. Roger that?"),
        .approve = COMPOUND_STRING("LT. SURGE: Outstanding, soldier!\pYou've got my approval, kid!\nI'll radio it to the LEAGUE!"),
        .repeatOffer = COMPOUND_STRING("LT. SURGE: Hey, soldier! Reporting\nfor duty?\pI need some R&R. Hold the fort\nfor me!"),
        .repeatThanks = COMPOUND_STRING("LT. SURGE: Mission accomplished!\nDismissed, soldier!"),
        .ownGym = COMPOUND_STRING("LT. SURGE: Ha! You've got your own\nbase to defend now, LEADER!\pGet back to PALLET!"),
        .observe = COMPOUND_STRING("LT. SURGE: I'll observe from the\nflank, soldier!\pYou take the command post!"),
    },
    [3] = {
        .name = COMPOUND_STRING("ERIKA"), .approvalFlag = FLAG_GYM_APPROVAL_ERIKA,
        .guests = 1, .level = 29, .partySize = 4, .tier = 1,
        .offer = COMPOUND_STRING("ERIKA: Oh my… A GYM in PALLET\nTOWN? How lovely.\pWould you mind watching my GYM\nwhile I tend to the flowers?\pIf a challenger comes, please\ngreet them for me."),
        .approve = COMPOUND_STRING("ERIKA: How wonderful. You welcomed\nthem as a LEADER should.\pYou have my approval, of course.\nI shall tell the LEAGUE today."),
        .repeatOffer = COMPOUND_STRING("ERIKA: Oh, hello again…\nI was just about to take a nap.\pWould you mind looking after the\nGYM for a little while?"),
        .repeatThanks = COMPOUND_STRING("ERIKA: Thank you ever so much…\nI feel so refreshed."),
        .ownGym = COMPOUND_STRING("ERIKA: Oh my, you have a GYM of\nyour own now.\pYour challengers will miss you.\nPlease, go to them."),
        .observe = COMPOUND_STRING("ERIKA: I'll sit with my flowers over\nthere… and watch quietly.\pPlease, take my place."),
    },
    [4] = {
        .name = COMPOUND_STRING("KOGA"), .approvalFlag = FLAG_GYM_APPROVAL_KOGA,
        .guests = 2, .level = 37, .partySize = 4, .tier = 1,
        .offer = COMPOUND_STRING("KOGA: Fwahahaha! So the child\nwould be a LEADER?\pA ninja tests his student in\nsecret. Hold my GYM while I…\pwatch from the shadows. Two\nchallengers approach. Ready?"),
        .approve = COMPOUND_STRING("KOGA: …Hmm! You did not falter.\pVery well. KOGA, master of\nPOISON, grants his approval!"),
        .repeatOffer = COMPOUND_STRING("KOGA: Fwahaha! You return.\pA ninja must train in secret now\nand then. Guard my GYM while I\lvanish!"),
        .repeatThanks = COMPOUND_STRING("KOGA: …Well guarded. I saw\neverything, of course."),
        .ownGym = COMPOUND_STRING("KOGA: A LEADER guards their own\nGYM, not another's.\pReturn to PALLET TOWN!"),
        .observe = COMPOUND_STRING("KOGA: I shall vanish into the\nshadows… and watch.\pTake my place, child."),
    },
    [5] = {
        .name = COMPOUND_STRING("SABRINA"), .approvalFlag = FLAG_GYM_APPROVAL_SABRINA,
        .guests = 2, .level = 40, .partySize = 4, .tier = 2,
        .offer = COMPOUND_STRING("SABRINA: I knew you would ask.\nI saw this moment a week ago.\pTwo challengers will come\nthrough my GYM today.\pYou will face them. I already\nknow how it ends… Do you?"),
        .approve = COMPOUND_STRING("SABRINA: …Just as I foresaw.\pYou have my approval. The\nLEAGUE knows it already."),
        .repeatOffer = COMPOUND_STRING("SABRINA: I knew you would come\nback today.\pA challenger is on the way.\nYou will take my place."),
        .repeatThanks = COMPOUND_STRING("SABRINA: As I foresaw.\nThank you."),
        .ownGym = COMPOUND_STRING("SABRINA: I see a GYM in PALLET\nTOWN… and challengers waiting\lfor you. Go."),
        .observe = COMPOUND_STRING("SABRINA: I will watch from over\nthere.\pThough I already know how it ends."),
    },
    [6] = {
        .name = COMPOUND_STRING("BLAINE"), .approvalFlag = FLAG_GYM_APPROVAL_BLAINE,
        .guests = 2, .level = 44, .partySize = 5, .tier = 2,
        .offer = COMPOUND_STRING("BLAINE: Hah! A future LEADER!\nHere's a quiz for you!\pQuestion: who holds a GYM when\nits LEADER is out?\pAnswer: YOU do! Two challengers\nare on their way. Fire it up!"),
        .approve = COMPOUND_STRING("BLAINE: Correct answer! You've\ngot fire in you, kid!\pMy approval is yours!\nWahahahah!"),
        .repeatOffer = COMPOUND_STRING("BLAINE: Hah! Quiz time! Who's\ncovering my GYM while I soak in\lthe hot springs?\pAnswer: YOU!"),
        .repeatThanks = COMPOUND_STRING("BLAINE: Correct! Fire it up again\nany time!"),
        .ownGym = COMPOUND_STRING("BLAINE: Quiz! Who has their own\nGYM now?\pAnswer: YOU! So go run it!\nWahahahah!"),
        .observe = COMPOUND_STRING("BLAINE: I'll watch from the sidelines!\pPop quiz: who's LEADER today?\nAnswer: YOU!"),
    },
    [7] = {
        .name = COMPOUND_STRING("GIOVANNI"), .approvalFlag = FLAG_GYM_APPROVAL_GIOVANNI,
        .guests = 2, .level = 47, .partySize = 5, .tier = 2,
        .offer = COMPOUND_STRING("GIOVANNI: So. You want the\nLEAGUE's blessing as well.\pThen hold my GYM. Two\nchallengers are waiting.\pLet us see if you can protect\nwhat is yours."),
        .approve = COMPOUND_STRING("GIOVANNI: …You protect what is\nyours. I respect that.\pThe LEAGUE will have my approval.\nNow leave me."),
        .repeatOffer = COMPOUND_STRING("GIOVANNI: You again.\nI have… business elsewhere.\pWatch my GYM. Don't disappoint\nme."),
        .repeatThanks = COMPOUND_STRING("GIOVANNI: Adequate.\nYou may go."),
        .ownGym = COMPOUND_STRING("GIOVANNI: You have a GYM of your\nown now. Guard it.\pMine needs no help from you."),
        .observe = COMPOUND_STRING("GIOVANNI: I will be watching.\pTake my place… for now."),
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
        COMPOUND_STRING("BROCK: Hi there! I'm BROCK, the\nPEWTER CITY GYM LEADER.\pPROF. OAK wrote to me about you.\nA ROCK trainer with a GYM plan!\pThat's great! I'll be keeping an\neye out for you on the road."),
        COMPOUND_STRING("BROCK: {PLAYER}! You made it!\pYour ROCK types have gotten solid\nsince OAK's lab.\pTwo ROCK GYMS in KANTO?\nI can't wait!"),
        COMPOUND_STRING("BROCK: Two ROCK GYMS in KANTO!\nI couldn't be prouder, {PLAYER}.\pThe LEAGUE will hear that you have\nmy approval. Rock on!")},
    [TYPE_NORMAL] = {0,
        COMPOUND_STRING("BROCK: Hey! I'm BROCK, LEADER of\nthe PEWTER CITY GYM.\pOAK says you raise NORMAL types.\nNo tricks, just basics, huh?\pThat's how I train too. I'll be\nkeeping tabs on you!"),
        COMPOUND_STRING("BROCK: Good to see you again,\n{PLAYER}!\pYour NORMAL types fought well.\nBasics win battles. Stick with it!"),
        COMPOUND_STRING("BROCK: Basics win battles. You\nproved it today!\pThe LEAGUE will hear that you have\nmy approval!")},
    [TYPE_WATER] = {1,
        COMPOUND_STRING("MISTY: So YOU're the kid from\nOAK's letter? I'm MISTY.\pI run the CERULEAN GYM. The WATER\nGYM. The ONLY one in KANTO!\pA WATER GYM in PALLET TOWN?\nI'm keeping my eye on you!"),
        COMPOUND_STRING("MISTY: Hmph. So the kid from\nOAK's lab made it all the way here.\pYour WATER POKéMON are good.\nAnnoyingly good.\pThere's still only room for ONE\nWATER GYM in KANTO!"),
        COMPOUND_STRING("MISTY: …Fine. FINE!\nYou're really good with them.\pMaybe KANTO can handle two WATER\nGYMS. Maybe.\pYou've got my approval. Now get\nout before I change my mind!")},
    [TYPE_ELECTRIC] = {2,
        COMPOUND_STRING("LT. SURGE: Ten-hut! LT. SURGE,\nVERMILION CITY GYM!\pOAK says you train ELECTRIC types\nand want a GYM. That's MY turf!\pI'll be watching you, kid.\nDon't short out!"),
        COMPOUND_STRING("LT. SURGE: Well, well! The ELECTRIC\nkid from OAK's lab!\pYou've got a spark, I'll give you\nthat. But this is still MY turf!"),
        COMPOUND_STRING("LT. SURGE: Ha! I'll admit it, kid.\nYou've got a real spark.\pYou've got my approval! And if\nyou need parts, call me!")},
    [TYPE_STEEL] = {2,
        COMPOUND_STRING("LT. SURGE: Ten-hut! LT. SURGE,\nVERMILION CITY GYM!\pOAK says you raise STEEL types.\nTough as tanks. I like that.\pBut a STEEL GYM in PALLET?\nI don't buy it. Prove me wrong!"),
        COMPOUND_STRING("LT. SURGE: The STEEL kid from\nOAK's lab! Your team held up under\lfire.\pStill don't buy a STEEL GYM in\nPALLET, though!"),
        COMPOUND_STRING("LT. SURGE: Your GYM's built like\na fortress! I take it back!\pYou've got my approval, soldier!")},
    [TYPE_GRASS] = {3,
        COMPOUND_STRING("ERIKA: Oh, hello… I'm ERIKA,\nfrom the CELADON CITY GYM.\pPROF. OAK told me you love GRASS\nPOKéMON. How delightful!\pI would love to help your GYM\ngrow. I'll come to see you."),
        COMPOUND_STRING("ERIKA: Oh, {PLAYER}! It's so nice\nto see you again.\pYour GRASS POKéMON have grown so\nmuch since we met at OAK's lab."),
        COMPOUND_STRING("ERIKA: Your POKéMON were in full\nbloom today.\pYou have my approval, and my\nfriendship. Please, take care.")},
    [TYPE_POISON] = {4,
        COMPOUND_STRING("KOGA: …You did not hear me come\nin. Good.\pI am KOGA, of the FUCHSIA CITY\nGYM. OAK says you train POISON.\pHmph. A ninja does not share his\nsecrets lightly. Earn them."),
        COMPOUND_STRING("KOGA: …So. The child from OAK's\nlab grows stronger.\pI have watched you. I still will\nnot make it easy for a rival."),
        COMPOUND_STRING("KOGA: …Fwahaha. I was wrong to\ndoubt you.\pA true POISON master. You have\nmy approval… and my respect.")},
    [TYPE_BUG] = {4,
        COMPOUND_STRING("KOGA: Fwahaha! You did not see me\nenter. I am KOGA, of FUCHSIA's GYM.\pOAK says you train BUG types.\nMost call them weak.\pProve them wrong, child. I will\nbe watching from the shadows."),
        COMPOUND_STRING("KOGA: Fwahaha! You've come far\nsince OAK's lab.\pYour BUG POKéMON surprised even\nme. A little."),
        COMPOUND_STRING("KOGA: Fwahaha! Your BUG POKéMON\nstrike like ninja.\pYou have my approval!")},
    [TYPE_PSYCHIC] = {5,
        COMPOUND_STRING("SABRINA: …I am SABRINA, of the\nSAFFRON CITY GYM.\pI knew you would be here. A\nsecond PSYCHIC GYM in KANTO.\pI foresaw it. I do not like it.\nBut I will watch over you."),
        COMPOUND_STRING("SABRINA: …As I foresaw at OAK's\nlab, you came.\pI still do not like a second\nPSYCHIC GYM. But visions do not\lask for permission."),
        COMPOUND_STRING("SABRINA: I saw a PSYCHIC GYM in\nPALLET TOWN. I resented it.\pNow I see why it was there.\nYou have my approval.")},
    [TYPE_FAIRY] = {5,
        COMPOUND_STRING("SABRINA: I am SABRINA, of the\nSAFFRON CITY GYM.\pI saw you in a dream. A child with\nFAIRY POKéMON and a GYM.\pThey have minds of their own.\nAs do you. We will meet again."),
        COMPOUND_STRING("SABRINA: We meet again, as I said\nwe would.\pYour FAIRY POKéMON trust you\ncompletely. That is rare."),
        COMPOUND_STRING("SABRINA: Your FAIRY POKéMON\ntrust you completely.\pYou have my approval.")},
    [TYPE_FIRE] = {6,
        COMPOUND_STRING("BLAINE: Hah! Quiz time! Who's the\nhotheaded FIRE expert of KANTO?\pAnswer: BLAINE of CINNABAR GYM!\nThat's me!\pOAK says you're a FIRE trainer!\nI'll be cheering you on, kid!"),
        COMPOUND_STRING("BLAINE: Hah! My favorite FIRE\ntrainer! Quiz! Who knew you'd make\lit this far?\pAnswer: ME! Wahahahah!"),
        COMPOUND_STRING("BLAINE: That's the spirit! Two\nFIRE GYMS in KANTO!\pMy approval is yours, kid!\nWahahahah!")},
    [TYPE_GROUND] = {7,
        COMPOUND_STRING("GIOVANNI: So you are OAK's\ncandidate. I am GIOVANNI.\pThe VIRIDIAN GYM is mine. KANTO's\nGROUND GYM. It always will be.\pStill… I will be watching you\nwith interest."),
        COMPOUND_STRING("GIOVANNI: We meet again, OAK's\ncandidate.\pYour GROUND POKéMON are strong.\nStill, VIRIDIAN's GYM is mine."),
        COMPOUND_STRING("GIOVANNI: …You have a feel for the\nearth. I can't deny it.\pThe LEAGUE will have my approval.\nGuard your GYM well.")},
    [TYPE_DARK] = {7,
        COMPOUND_STRING("GIOVANNI: I am GIOVANNI. The\nVIRIDIAN GYM is mine.\pDARK POKéMON… OAK has chosen an\ninteresting candidate.\pYou may prove useful.\nWe will meet again."),
        COMPOUND_STRING("GIOVANNI: So. We meet again.\pYour DARK POKéMON have grown…\nfearsome. But a child is still a\lchild."),
        COMPOUND_STRING("GIOVANNI: I underestimated you.\nI don't do that often.\pThe LEAGUE will have my approval.")},
    [TYPE_ICE] = {LORELEI,
        COMPOUND_STRING("LORELEI: Hello. I'm LORELEI, of\nthe ELITE FOUR.\pPROF. OAK wrote to me about a\nyoung ICE trainer. Refreshing.\pFew have the patience for them.\nI'll check on you now and then."),
        COMPOUND_STRING("LORELEI: {PLAYER}. I've waited for\nthis since OAK's lab.\pShow me how cold you've become!"),
        COMPOUND_STRING("LORELEI: A fellow ICE trainer…\nand a fine one.\pYou have my approval. KANTO could\nuse an ICE GYM. Stay cool.")},
    [TYPE_FIGHTING] = {BRUNO,
        COMPOUND_STRING("BRUNO: Hoo hah! I'm BRUNO, of the\nELITE FOUR!\pOAK says you train FIGHTING types!\nI'll help you grow stronger!\pTrain hard. I'll come find you\non the road!"),
        COMPOUND_STRING("BRUNO: Hoo hah! {PLAYER}! You've\ngrown strong since OAK's lab!\pNow let our POKéMON talk with\ntheir fists!"),
        COMPOUND_STRING("BRUNO: Such power! You trained\nthem well!\pYou have my approval. Come train\nwith me any time!")},
    [TYPE_GHOST] = {AGATHA,
        COMPOUND_STRING("AGATHA: So you're Oak's little\nGHOST trainer. I'm AGATHA.\pELITE FOUR, since before you were\nborn. And Oak wants a GYM, eh?\pHah! I'll be keeping an eye on\nyou, whippersnapper."),
        COMPOUND_STRING("AGATHA: Well, if it isn't Oak's\nGHOST whippersnapper.\pYou got this far? Hah! Let's see\nwhat my gifts did for you!"),
        COMPOUND_STRING("AGATHA: …Hmph. You've got the\ngift, child. I'll grant you that.\pYou have my approval. Don't make\nme regret it!")},
    [TYPE_DRAGON] = {LANCE,
        COMPOUND_STRING("LANCE: I am LANCE, of the ELITE\nFOUR. OAK says you raise DRAGONS.\pThey are the pride of the ELITE\nFOUR. My pride.\pA child with a DRAGON GYM?\nThen prove you're worthy."),
        COMPOUND_STRING("LANCE: {PLAYER}. I've watched your\nDRAGONS grow since OAK's lab.\pNow prove you're worthy of them!"),
        COMPOUND_STRING("LANCE: Your DRAGONS fly true.\nI was wrong about you.\pYou have my approval, DRAGON\nmaster.")},
    [TYPE_FLYING] = {LANCE,
        COMPOUND_STRING("LANCE: I am LANCE, of the ELITE\nFOUR. OAK says you train FLYING\ltypes.\pMy DRAGONITE rules the skies.\nLet's see if yours can keep up."),
        COMPOUND_STRING("LANCE: {PLAYER}. Your FLYING POKéMON\nhave come far since OAK's lab.\pLet's see if they can keep up with\nmy DRAGONITE!"),
        COMPOUND_STRING("LANCE: Your POKéMON own the sky.\pYou have my approval.")},
};

// The mentors themselves (Gym Leaders 0-7, Elite Four 8-11): how they look, what they say when
// they meet the player in Oak's lab (their story and a question), and with their two gifts
static const struct
{
    u16 gfx;
    const u8 *guardGift;    // after Mt. Moon
    const u8 *edgeGift;     // at Cinnabar
    const u8 *story;        // in Oak's lab, after their introduction
    const u8 *question;     // a yes/no question
    const u8 *yes;
    const u8 *no;
    const u8 *farewell;     // leaving the lab
} sMentorPeople[GYM_CANDIDATE_LEADERS + GYM_CANDIDATE_ELITE_FOUR] =
{
    {OBJ_EVENT_GFX_BROCK,
        COMPOUND_STRING("BROCK: {PLAYER}! I heard you made it\nthrough MT. MOON. Knew you would!\pHere, I made this for you.\nLet one of your POKéMON hold it."),
        COMPOUND_STRING("BROCK: {PLAYER}! I took the ferry\nall the way out here!\pYou've learned to take a hit.\nNow learn to land one. Here!"),
        COMPOUND_STRING("BROCK: My dad ran the PEWTER GYM\nbefore me. When he left, I had to\lraise my brothers and hold the GYM.\pHere's what I learned: a GYM isn't\nwalls and statues.\pIt's a promise that its LEADER is\nthere, every day, for every\lchallenger. Rock solid!"),
        COMPOUND_STRING("BROCK: So, {PLAYER}… Can challengers\ncount on you, every single day?"),
        COMPOUND_STRING("BROCK: Ha! That's what I like to\nhear!"),
        COMPOUND_STRING("BROCK: Honest! That's a start.\nConfidence comes with practice."),
        COMPOUND_STRING("BROCK: I'll come find you once you're\nthrough MT. MOON. Good luck!")},
    {OBJ_EVENT_GFX_MISTY,
        COMPOUND_STRING("MISTY: …And before you go. OAK\nmade me promise to give you this.\pDon't get the wrong idea! I don't\nwant my rival losing to a fluke!"),
        COMPOUND_STRING("MISTY: Ugh, it's so hot here!\n…Fine. Here.\pI said I'd find you when you were\nready. Don't make me regret it!"),
        COMPOUND_STRING("MISTY: Listen up. I became a LEADER\nbecause my sisters wouldn't stop\lshowing off.\pSo I trained harder than all of\nthem. Every challenger who walks\lin tests that.\pA GYM is only as tough as its\nLEADER on their worst day!"),
        COMPOUND_STRING("MISTY: Well? You really think you\ncan beat me at my own game?"),
        COMPOUND_STRING("MISTY: Hmph! Big talk.\nWe'll see about that!"),
        COMPOUND_STRING("MISTY: Then why even try?\n…Ugh, fine. Prove yourself wrong."),
        COMPOUND_STRING("MISTY: Don't think this means I like\nyou! See you around, kid.")},
    {OBJ_EVENT_GFX_LT_SURGE,
        COMPOUND_STRING("LT. SURGE: Ten-hut! Heard you're\nstill in the fight, kid!\pEvery soldier needs armor.\nTake this and keep your head down!"),
        COMPOUND_STRING("LT. SURGE: Kid! You've survived the\nfront lines. Time for artillery!\pWith this, nothing can stop your\nattacks cold!"),
        COMPOUND_STRING("LT. SURGE: In the war, I learned one\nthing: you hold your position, no\lmatter what!\pA GYM is a base. Challengers storm\nit. Your job is to make them earn\levery inch!\pDiscipline, kid! Drill your team\nlike soldiers!"),
        COMPOUND_STRING("LT. SURGE: Are you ready to follow\norders, private?"),
        COMPOUND_STRING("LT. SURGE: Ha! That's the spirit!"),
        COMPOUND_STRING("LT. SURGE: What?! …Ha! You've got\nguts, I'll give you that."),
        COMPOUND_STRING("LT. SURGE: Move out, soldier!\nI'll be checking on you!")},
    {OBJ_EVENT_GFX_ERIKA,
        COMPOUND_STRING("ERIKA: Oh, there you are… I came\nall the way from CELADON.\pMT. MOON must have been so dark.\nThis will keep your POKéMON safe."),
        COMPOUND_STRING("ERIKA: My, what a long boat ride…\nI dozed off twice.\pPlease take this. May nothing\nstand in your POKéMON's way."),
        COMPOUND_STRING("ERIKA: I tend a garden in my GYM.\nEach flower needs its own care…\pJust like each challenger. Some need\na gentle lesson. Others need a\lthorny one.\pA good LEADER helps them grow…\neven when they lose."),
        COMPOUND_STRING("ERIKA: Tell me… do you love your\nPOKéMON?"),
        COMPOUND_STRING("ERIKA: I could tell. They seem to\nglow around you."),
        COMPOUND_STRING("ERIKA: Oh my… Shy, perhaps? They\nseem to love you, at least."),
        COMPOUND_STRING("ERIKA: I'll come see how you've\ngrown. Please take care…")},
    {OBJ_EVENT_GFX_KOGA,
        COMPOUND_STRING("KOGA: Fwahaha! Did you notice me\nfollowing you through MT. MOON?\pNo? Good. A ninja's armor.\nTake it… and tell no one."),
        COMPOUND_STRING("KOGA: Fwahaha! Surprised? A ninja\ngoes where his student goes.\pYou are ready for my secret\ntechnique. Take it!"),
        COMPOUND_STRING("KOGA: A ninja's GYM hides its secrets\nin plain sight. Invisible walls.\lHidden traps.\pA challenger who reaches me has\nbeen tested a dozen times.\pBuild your GYM like a ninja, child.\nMake every step a lesson."),
        COMPOUND_STRING("KOGA: Will you train in secret, as a\nninja does?"),
        COMPOUND_STRING("KOGA: Fwahaha! Then we shall see."),
        COMPOUND_STRING("KOGA: Hmph. Then they will see you\ncoming, child."),
        COMPOUND_STRING("KOGA: I will be watching.\nAlways.")},
    {OBJ_EVENT_GFX_SABRINA,
        COMPOUND_STRING("SABRINA: I saw you leave MT. MOON.\nI saw you take this from me.\pSo… take it."),
        COMPOUND_STRING("SABRINA: Seven BADGES. I saw this\nday, too.\pTake it. No wall will hold your\nPOKéMON back now."),
        COMPOUND_STRING("SABRINA: Most LEADERS react to their\nchallengers. I see them coming.\pYou cannot read minds. But you can\nread people. How they walk in.\lWhat they fear.\pA LEADER who understands their\nchallenger has already won."),
        COMPOUND_STRING("SABRINA: Do you believe in fate?"),
        COMPOUND_STRING("SABRINA: Then you already know we\nwill meet again."),
        COMPOUND_STRING("SABRINA: …Neither did I.\nOnce."),
        COMPOUND_STRING("SABRINA: We will meet again soon.\nI have seen it.")},
    {OBJ_EVENT_GFX_BLAINE,
        COMPOUND_STRING("BLAINE: Hah! Quiz time! What keeps\na POKéMON safe from its weakness?\pAnswer: this! Take it, kid!"),
        COMPOUND_STRING("BLAINE: Hah! One more quiz! What\nlets an attack hit when it\lshouldn't?\pAnswer: this! I've been saving it\nfor you!"),
        COMPOUND_STRING("BLAINE: My GYM is a quiz! Answer\nright, and the path opens. Answer\lwrong, and you battle!\pA GYM should teach! Knowledge\nburns brighter than any FIRE\lBLAST!\pWahahah! Well… almost any."),
        COMPOUND_STRING("BLAINE: Quiz! Is FIRE the best type?"),
        COMPOUND_STRING("BLAINE: Correct!! Wahahahah!"),
        COMPOUND_STRING("BLAINE: Wrong!! …But I admire your\nnerve, kid!"),
        COMPOUND_STRING("BLAINE: I'll be cheering for you!\nFire it up! Wahahah!")},
    {OBJ_EVENT_GFX_GIOVANNI,
        COMPOUND_STRING("GIOVANNI: You survived MT. MOON.\nSome of my… associates did not\lfare as well.\pTake this. A LEADER protects what\nis theirs."),
        COMPOUND_STRING("GIOVANNI: …You've come far.\nFarther than I expected.\pTake this. True power is not\nstopped by something as small as\la type."),
        COMPOUND_STRING("GIOVANNI: A GYM is power. Whoever\nholds it decides who passes, and\lwho does not.\pMost LEADERS forget that. They\nbecome… gentle.\pNever forget what a GYM truly is,\nchild."),
        COMPOUND_STRING("GIOVANNI: Tell me. Will you do\nwhatever it takes to win?"),
        COMPOUND_STRING("GIOVANNI: …Good. I may have a use\nfor someone like you."),
        COMPOUND_STRING("GIOVANNI: Principles. We'll see how\nlong they last."),
        COMPOUND_STRING("GIOVANNI: Until we meet again…\nOAK's candidate.")},
    {OBJ_EVENT_GFX_LORELEI,
        COMPOUND_STRING("LORELEI: Hello again, {PLAYER}.\nI was in CERULEAN… shopping.\pThis is for you. No weakness will\ncrack your POKéMON with this."),
        COMPOUND_STRING("LORELEI: CINNABAR is far too warm\nfor me. I won't stay long.\pThis is for you. No foe will shrug\noff your attacks with it."),
        COMPOUND_STRING("LORELEI: Before the ELITE FOUR, I\ntrained on FOUR ISLAND, far to the\lsouth.\pThe hardest part wasn't battling.\nIt was staying calm when a\lchallenger was better than I\lexpected.\pKeep a cool head. Always."),
        COMPOUND_STRING("LORELEI: Can you stay calm when\nthings go wrong?"),
        COMPOUND_STRING("LORELEI: Good. Ice doesn't melt\nunder pressure."),
        COMPOUND_STRING("LORELEI: Honest. Then that's what\nwe'll work on."),
        COMPOUND_STRING("LORELEI: Stay cool, {PLAYER}.\nI'll be in touch.")},
    {OBJ_EVENT_GFX_BRUNO,
        COMPOUND_STRING("BRUNO: Hoo! You made it through\nMT. MOON!\pA true fighter guards his body.\nTake this!"),
        COMPOUND_STRING("BRUNO: Hoo hah! Seven BADGES!\nYou've built a fine guard.\pNow, a fine strike! Take this!"),
        COMPOUND_STRING("BRUNO: I train in caves and on\nmountains. Alone, with my POKéMON.\pA GYM is no different! Your POKéMON\nmust trust you. Your challengers\lmust respect you.\pBoth come from hard work!\nHoo hah!"),
        COMPOUND_STRING("BRUNO: Are you ready to train until\nyou drop?"),
        COMPOUND_STRING("BRUNO: Hoo hah! That's what I like\nto hear!"),
        COMPOUND_STRING("BRUNO: Ha! You'll change your mind\nonce you feel the power!"),
        COMPOUND_STRING("BRUNO: Train hard! I'll see you on\nthe road!")},
    {OBJ_EVENT_GFX_AGATHA,
        COMPOUND_STRING("AGATHA: Hmph. Oak pestered me\nuntil I brought you this.\pTake it before I change my mind,\nwhippersnapper!"),
        COMPOUND_STRING("AGATHA: Seven BADGES, eh? I suppose\nI can't call you whippersnapper.\p…Well, I can. But take this\nanyway."),
        COMPOUND_STRING("AGATHA: I battled beside Oak when we\nwere young. Did he tell you that?\pHah! Of course not.\pHe wants this GYM more than he\nlets on. Don't you forget it.\pAnd don't you dare go soft on\nyour challengers!"),
        COMPOUND_STRING("AGATHA: Think you're tougher than\nOak's other brat?"),
        COMPOUND_STRING("AGATHA: Hah! Cocky.\nI like that."),
        COMPOUND_STRING("AGATHA: Hmph. At least you're no\nliar."),
        COMPOUND_STRING("AGATHA: Don't make me regret coming\nall this way, whippersnapper!")},
    {OBJ_EVENT_GFX_LANCE,
        COMPOUND_STRING("LANCE: You made it past MT. MOON.\nNot bad.\pA POKéMON is only as strong as its\nweakest point. Cover yours."),
        COMPOUND_STRING("LANCE: Seven BADGES. You've proven\nyourself.\pTake this. No POKéMON will ignore\nyour attacks again."),
        COMPOUND_STRING("LANCE: The ELITE FOUR faces every\nchallenger who gets past the GYMS.\pToo many arrive soft. Their LEADERS\nwent easy on them.\pA LEADER's duty is to the LEAGUE.\nMake your challengers worthy."),
        COMPOUND_STRING("LANCE: Will you make your challengers\nworthy of the LEAGUE?"),
        COMPOUND_STRING("LANCE: Then we understand each other."),
        COMPOUND_STRING("LANCE: …Then perhaps you're not\nready. Prove me wrong."),
        COMPOUND_STRING("LANCE: I'll be watching your progress\nclosely, {PLAYER}.")},
};

// Each type's two items: {defensive, offensive}
static const u16 sTypeItems[NUMBER_OF_MON_TYPES][2] =
{
    [TYPE_NORMAL] = {ITEM_FLUFFY_SWEATER, ITEM_THIRD_EYE},
    [TYPE_FIGHTING] = {ITEM_MOUTH_GUARD, ITEM_STEROIDS},
    [TYPE_FLYING] = {ITEM_AVIATOR_GOGGLES, ITEM_JET_ENGINE},
    [TYPE_POISON] = {ITEM_TITANIUM_COIL, ITEM_CORROSIVE_VENOM},
    [TYPE_GROUND] = {ITEM_GRANITE_SARCOPHAGUS, ITEM_DIAMOND_DRILL_BIT},
    [TYPE_ROCK] = {ITEM_DIAMOND_PLATING, ITEM_PRECISION_CHISEL},
    [TYPE_BUG] = {ITEM_CACOON, ITEM_EXTRA_STINGER},
    [TYPE_GHOST] = {ITEM_LIGHT_SWITCH, ITEM_OUIJA_BOARD},
    [TYPE_STEEL] = {ITEM_STAINLESS_ALLOY, ITEM_KNIFE_SHARPENER},
    [TYPE_FIRE] = {ITEM_HEAT_SHIELD, ITEM_OXIDIZER},
    [TYPE_WATER] = {ITEM_CISTERN, ITEM_NOZZLE},
    [TYPE_GRASS] = {ITEM_CHICKEN_WIRE, ITEM_MIRACLE_GRO},
    [TYPE_ELECTRIC] = {ITEM_CAPACITOR, ITEM_TRANSFORMER},
    [TYPE_PSYCHIC] = {ITEM_ENERGY_SHIELD, ITEM_PSIONIC_BLADE},
    [TYPE_ICE] = {ITEM_COOLER, ITEM_ROCK_SALT},
    [TYPE_DRAGON] = {ITEM_FIRE_SHROUD, ITEM_BIONIC_JAW},
    [TYPE_DARK] = {ITEM_BLACK_VEIL, ITEM_MANIFESTO},
    [TYPE_FAIRY] = {ITEM_TWINKLING_LIGHTS, ITEM_MAGIC_WAND},
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
    // VAR_0x8007 = 1: a stand-in shift after the approval, one challenger
    u32 guests = gSpecialVar_0x8007 ? 1 : gGymCandidateLeaders[Leader()].guests;
    gSpecialVar_Result = gSpecialVar_0x800B < guests;
}

// Special: VAR_0x8009 = leader -> gStringVar4 = asking the player to cover again (after approval)
void GymCandidate_BufferRepeatOffer(void)
{
    StringExpandPlaceholders(gStringVar4, gGymCandidateLeaders[Leader()].repeatOffer);
}

// Special: VAR_0x8009 = leader -> gStringVar4 = thanks after a shift
void GymCandidate_BufferRepeatThanks(void)
{
    StringExpandPlaceholders(gStringVar4, gGymCandidateLeaders[Leader()].repeatThanks);
}

// Special: VAR_0x8009 = leader -> gStringVar4 = no more shifts: the player has a gym now
void GymCandidate_BufferOwnGym(void)
{
    StringExpandPlaceholders(gStringVar4, gGymCandidateLeaders[Leader()].ownGym);
}

// Special: VAR_0x8009 = leader, VAR_0x800B = challengers beaten so far, VAR_0x8007 = a shift
// after approval. Builds the next one.
void GymCandidate_GenerateGuest(void)
{
    const struct GymCandidateLeader *leader = &gGymCandidateLeaders[Leader()];
    StringCopy(gStringVar3, leader->name);
    GymChallenge_GenerateGuest(leader->level, leader->partySize, leader->tier, gSpecialVar_0x8007 != 0);
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
// The mentor's story: they meet the player in Oak's lab right after the Pokédex (Oak wrote to
// them about the player's gym), drop by the Cerulean Gym after Mt. Moon with a defensive item
// for the player's type, and the Cinnabar Gym with an offensive one (or hand them over
// themselves, if it's their gym).
// ---------------------------------------------------------------------------
u16 GymCandidate_GetTypeItem(u32 type, bool32 offensive)
{
    if (type >= NUMBER_OF_MON_TYPES)
        return ITEM_NONE;
    return sTypeItems[type][offensive ? 1 : 0];
}

const u8 *GymCandidate_GetMentorName(u32 who)
{
    if (who < GYM_CANDIDATE_LEADERS)
        return gGymCandidateLeaders[who].name;
    return sEliteFour[min(who, (u32)LANCE) - LORELEI].name;
}

const u8 *GymCandidate_GetMentorGiftText(u32 who, bool32 offensive)
{
    who = min(who, (u32)LANCE);
    return offensive ? sMentorPeople[who].edgeGift : sMentorPeople[who].guardGift;
}

static const struct GymCandidateMentor *PlayerMentor(void)
{
    return GymCandidate_GetMentorForType(VarGet(VAR_PLAYER_TYPE));
}

// Special: STR_VAR_1 = the player's mentor's name. VAR_RESULT = whether there is one.
void GymCandidate_BufferMentorName(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    gSpecialVar_Result = (mentor != NULL);
    if (mentor != NULL)
        StringCopy(gStringVar1, GymCandidate_GetMentorName(mentor->who));
}

// Special: the meeting in Oak's lab. VAR_RESULT = whether it happens now; then
// VAR_OBJ_GFX_ID_0 = the mentor's look, STR_VAR_1 = their name.
void GymCandidate_PrepareMentorIntro(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    gSpecialVar_Result = FALSE;
    if (mentor == NULL || (VarGet(VAR_MENTOR_STAGE) & MENTOR_MET))
        return;
    VarSet(VAR_MENTOR_STAGE, VarGet(VAR_MENTOR_STAGE) | MENTOR_MET);
    VarSet(VAR_OBJ_GFX_ID_0, sMentorPeople[mentor->who].gfx);
    StringCopy(gStringVar1, GymCandidate_GetMentorName(mentor->who));
    gSpecialVar_Result = TRUE;
}

// Specials for the rest of the meeting in Oak's lab: gStringVar4 = the mentor's story, question,
// answer (VAR_RESULT = what the player said), farewell
void GymCandidate_BufferMentorStory(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, sMentorPeople[mentor->who].story);
}

void GymCandidate_BufferMentorQuestion(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, sMentorPeople[mentor->who].question);
}

void GymCandidate_BufferMentorAnswer(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, gSpecialVar_Result ? sMentorPeople[mentor->who].yes : sMentorPeople[mentor->who].no);
}

void GymCandidate_BufferMentorFarewell(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, sMentorPeople[mentor->who].farewell);
}

const u8 *GymCandidate_GetMentorPersonText(u32 who, u32 field)
{
    who = min(who, (u32)LANCE);
    switch (field)
    {
    case 0: return sMentorPeople[who].story;
    case 1: return sMentorPeople[who].question;
    case 2: return sMentorPeople[who].yes;
    case 3: return sMentorPeople[who].no;
    default: return sMentorPeople[who].farewell;
    }
}

// Special: the mentor's housewarming gift, the cheapest decoration of the player's type (into the
// gym PC). STR_VAR_1 = the mentor, STR_VAR_2 = the decoration. VAR_RESULT = whether there was one.
void GymCandidate_GiveMentorDecor(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    u32 i, best = GYM_DECOR_COUNT, category = GYM_DECOR_CAT_TYPE(VarGet(VAR_PLAYER_TYPE));

    gSpecialVar_Result = FALSE;
    if (mentor == NULL)
        return;
    StringCopy(gStringVar1, GymCandidate_GetMentorName(mentor->who));
    for (i = 0; i < GYM_DECOR_COUNT; i++)
    {
        if (gGymDecor[i].category == category && gGymDecor[i].price != 0
         && (best == GYM_DECOR_COUNT || gGymDecor[i].price < gGymDecor[best].price))
            best = i;
    }
    if (best == GYM_DECOR_COUNT)
        return;
    GymDecor_SetOwned(best, GymDecor_CountOwned(best) + 1);
    StringCopy(gStringVar2, gGymDecor[best].name);
    gSpecialVar_Result = TRUE;
}

// Special: gStringVar4 = the mentor's introduction (after GymCandidate_PrepareMentorIntro)
void GymCandidate_BufferMentorIntro(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, mentor->intro);
}

// Special: VAR_0x8009 = the gym whose badge was just won. At the Cerulean Gym (1) the mentor
// gives the defensive item, at the Cinnabar Gym (6) the offensive one.
// VAR_RESULT = MENTOR_GIFT_*; then VAR_0x8004 = the item, gStringVar4 = what they say,
// VAR_OBJ_GFX_ID_0 = the mentor's look (for a visit).
void GymCandidate_PrepareMentorGift(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    u32 leader = gSpecialVar_0x8009, bit;
    bool32 offensive;

    gSpecialVar_Result = MENTOR_GIFT_NONE;
    if (mentor == NULL)
        return;
    if (leader == 1)
        offensive = FALSE, bit = MENTOR_GAVE_GUARD;
    else if (leader == 6)
        offensive = TRUE, bit = MENTOR_GAVE_EDGE;
    else
        return;
    if (VarGet(VAR_MENTOR_STAGE) & bit)
        return;
    VarSet(VAR_MENTOR_STAGE, VarGet(VAR_MENTOR_STAGE) | bit);
    gSpecialVar_0x8004 = GymCandidate_GetTypeItem(VarGet(VAR_PLAYER_TYPE), offensive);
    VarSet(VAR_OBJ_GFX_ID_0, sMentorPeople[mentor->who].gfx);
    gSpecialVar_Result = (mentor->who == leader) ? MENTOR_GIFT_LEADER : MENTOR_GIFT_VISIT;
}

// Special (Johto: the convention invitation): VAR_RESULT = whether the player has a mentor;
// VAR_OBJ_GFX_ID_0 = how they look, STR_VAR_1 = their name
void GymCandidate_PrepareMentorVisit(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    gSpecialVar_Result = (mentor != NULL);
    if (mentor == NULL)
        return;
    VarSet(VAR_OBJ_GFX_ID_0, sMentorPeople[mentor->who].gfx);
    StringCopy(gStringVar1, GymCandidate_GetMentorName(mentor->who));
}

// Special: VAR_0x8009 = the gym (1 or 6) -> gStringVar4 = what the mentor says with the gift
void GymCandidate_BufferMentorGift(void)
{
    const struct GymCandidateMentor *mentor = PlayerMentor();
    if (mentor != NULL)
        StringExpandPlaceholders(gStringVar4, GymCandidate_GetMentorGiftText(mentor->who, gSpecialVar_0x8009 == 6));
}

// ---------------------------------------------------------------------------
// Where the challenger stands: next to the player, preferably behind them (the leader is in
// front, watching). VAR_0x8005/6 = the spot, VAR_RESULT = whether there is one.
// ---------------------------------------------------------------------------
static const s8 sDirDx[] = {[DIR_SOUTH] = 0, [DIR_NORTH] = 0, [DIR_WEST] = -1, [DIR_EAST] = 1};
static const s8 sDirDy[] = {[DIR_SOUTH] = 1, [DIR_NORTH] = -1, [DIR_WEST] = 0, [DIR_EAST] = 0};
static const u8 sOpposite[] = {[DIR_SOUTH] = DIR_NORTH, [DIR_NORTH] = DIR_SOUTH, [DIR_WEST] = DIR_EAST, [DIR_EAST] = DIR_WEST};

static EWRAM_DATA u8 sGuestDir;   // where the challenger is, seen from the player
static EWRAM_DATA u8 sFacingBefore;   // which way the player faced before they came
static EWRAM_DATA bool8 sVisitorWalked;
static EWRAM_DATA bool8 sFrontOnly;   // the visitor must end up right in front of the player

#define VISITOR_PATH_MAX 60
EWRAM_DATA u8 gMentorWalkIn[VISITOR_PATH_MAX + 2];
EWRAM_DATA u8 gMentorWalkOut[VISITOR_PATH_MAX + 2];

static bool32 IsFreeSpot(s16 x, s16 y, u8 elevation)
{
    u8 e = MapGridGetElevationAt(x, y);
    u32 behavior = MapGridGetMetatileBehaviorAt(x, y);
    if (MapGridGetCollisionAt(x, y))
        return FALSE;
    if (MetatileBehavior_IsDoor(behavior) || MetatileBehavior_IsWarpDoor(behavior) || MetatileBehavior_IsNonAnimDoor(behavior))
        return FALSE;   // not in a doorway
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
    sGuestDir = sOpposite[facing];
    sFacingBefore = facing;
    sVisitorWalked = FALSE;
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

// Special: the challenger has gone; the player turns back the way they faced before
void GymCandidate_FaceBack(void)
{
    ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], sFacingBefore);
}

// ---------------------------------------------------------------------------
// A visitor (the mentor) walking in through the map's entrance (its lowest warp) to the player,
// and back out. Found with a search over the map; if there's no way, they just appear beside the
// player instead (GymCandidate_PlaceGuest).
// ---------------------------------------------------------------------------
static bool32 VisitorCanWalk(s32 x, s32 y, u8 elevation)
{
    s32 mx = x + MAP_OFFSET, my = y + MAP_OFFSET;
    u8 e = MapGridGetElevationAt(mx, my);
    if (MapGridGetCollisionAt(mx, my) || GetObjectEventIdByXY(mx, my) != OBJECT_EVENTS_COUNT)
        return FALSE;
    if (MetatileBehavior_IsForcedMovementTile(MapGridGetMetatileBehaviorAt(mx, my)))
        return FALSE;
    return e == elevation || e == 0 || e == 15 || elevation == 0 || elevation == 15;
}

// Special: VAR_RESULT = whether the visitor can walk in; VAR_0x8005/6 = where they start (the
// door). The walk in goes to gMentorWalkIn (ending facing the player), the walk out to
// gMentorWalkOut.
void GymCandidate_PlanVisitorWalk(void)
{
    static const s8 dx[] = {0, 0, -1, 1}, dy[] = {1, -1, 0, 0};
    static const u8 walk[] = {MOVEMENT_ACTION_WALK_NORMAL_DOWN, MOVEMENT_ACTION_WALK_NORMAL_UP,
                              MOVEMENT_ACTION_WALK_NORMAL_LEFT, MOVEMENT_ACTION_WALK_NORMAL_RIGHT};
    static const u8 face[] = {MOVEMENT_ACTION_FACE_DOWN, MOVEMENT_ACTION_FACE_UP,
                              MOVEMENT_ACTION_FACE_LEFT, MOVEMENT_ACTION_FACE_RIGHT};
    static const u8 dirOf[] = {DIR_SOUTH, DIR_NORTH, DIR_WEST, DIR_EAST};
    s32 w = gMapHeader.mapLayout->width, h = gMapHeader.mapLayout->height;
    s32 cells = w * h, i, head = 0, tail = 0, start, goal = -1, sx = -1, sy = -1;
    s16 px, py, *from, *queue;
    u8 *dir, elevation;
    const struct MapEvents *events = gMapHeader.events;

    gSpecialVar_Result = FALSE;
    sVisitorWalked = FALSE;
    sFacingBefore = GetPlayerFacingDirection();
    for (i = 0; i < events->warpCount; i++)
    {
        if (events->warps[i].y > sy)
        {
            sx = events->warps[i].x;
            sy = events->warps[i].y;
        }
    }
    if (sx < 0 || cells > 64 * 64)
        return;
    PlayerGetDestCoords(&px, &py);
    px -= MAP_OFFSET;
    py -= MAP_OFFSET;
    elevation = MapGridGetElevationAt(px + MAP_OFFSET, py + MAP_OFFSET);

    from = Alloc(cells * sizeof(s16));
    queue = Alloc(cells * sizeof(s16));
    dir = Alloc(cells);
    for (i = 0; i < cells; i++)
        from[i] = -1;
    start = sx + sy * w;
    from[start] = start;
    queue[tail++] = start;
    while (head < tail && goal < 0)
    {
        s32 cur = queue[head++], x = cur % w, y = cur / w;
        for (i = 0; i < 4; i++)
        {
            if (x + dx[i] == px && y + dy[i] == py
             && (!sFrontOnly || (x == px + sDirDx[sFacingBefore] && y == py + sDirDy[sFacingBefore])))
            {
                goal = cur;
                dir[start] = i;     // (the way to face, kept in an unused slot)
                break;
            }
        }
        for (i = 0; i < 4 && goal < 0; i++)
        {
            s32 nx = x + dx[i], ny = y + dy[i], next = nx + ny * w;
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || from[next] >= 0 || !VisitorCanWalk(nx, ny, elevation))
                continue;
            from[next] = cur;
            dir[next] = i;
            queue[tail++] = next;
        }
    }
    if (goal >= 0 && goal != start)
    {
        s32 steps = 0, c, faceDir = dir[start];
        for (c = goal; c != start; c = from[c])
            steps++;
        if (steps <= VISITOR_PATH_MAX)
        {
            s32 n = steps;
            for (c = goal; c != start; c = from[c])
            {
                n--;
                gMentorWalkIn[n] = walk[dir[c]];
                gMentorWalkOut[steps - 1 - n] = walk[dir[c] ^ 1];   // the opposite way, in reverse
            }
            gMentorWalkIn[steps] = face[faceDir];
            gMentorWalkIn[steps + 1] = MOVEMENT_ACTION_STEP_END;
            gMentorWalkOut[steps] = MOVEMENT_ACTION_STEP_END;
            sGuestDir = sOpposite[dirOf[faceDir]];  // the visitor, seen from the player
            gSpecialVar_0x8005 = sx;
            gSpecialVar_0x8006 = sy;
            gSpecialVar_Result = TRUE;
            sVisitorWalked = TRUE;
        }
    }
    Free(from);
    Free(queue);
    Free(dir);
}

// Special: a challenger walking in while the player holds the gym: like
// GymCandidate_PlanVisitorWalk, but to the square in front of the player if possible
void GymCandidate_PlanChallengerWalk(void)
{
    sFrontOnly = TRUE;
    GymCandidate_PlanVisitorWalk();
    sFrontOnly = FALSE;
    if (!gSpecialVar_Result)
        GymCandidate_PlanVisitorWalk();
}

// ---------------------------------------------------------------------------
// Holding the gym: the player takes the leader's spot. The leader steps aside to watch (or, for
// a stand-in shift after approval, leaves: they go out of sight), and comes back afterwards.
// ---------------------------------------------------------------------------
static EWRAM_DATA u8 sHoldLeaderObj;
static EWRAM_DATA bool8 sHolding;   // the player is on the leader's spot
static EWRAM_DATA s16 sHoldLeaderX, sHoldLeaderY, sHoldAsideX, sHoldAsideY;  // map coords (with the offset)
static EWRAM_DATA u8 sHoldDir;  // the way the player faced the leader
EWRAM_DATA u8 gHoldPlayerIn[3];
EWRAM_DATA u8 gHoldPlayerOut[3];

static const u8 sWalkFast[] = {
    [DIR_SOUTH] = MOVEMENT_ACTION_WALK_FAST_DOWN, [DIR_NORTH] = MOVEMENT_ACTION_WALK_FAST_UP,
    [DIR_WEST] = MOVEMENT_ACTION_WALK_FAST_LEFT, [DIR_EAST] = MOVEMENT_ACTION_WALK_FAST_RIGHT,
};
static const u8 sFace[] = {
    [DIR_SOUTH] = MOVEMENT_ACTION_FACE_DOWN, [DIR_NORTH] = MOVEMENT_ACTION_FACE_UP,
    [DIR_WEST] = MOVEMENT_ACTION_FACE_LEFT, [DIR_EAST] = MOVEMENT_ACTION_FACE_RIGHT,
};

static u8 DirToward(s16 fromX, s16 fromY, s16 toX, s16 toY)
{
    s16 dx = toX - fromX, dy = toY - fromY;
    if (abs(dx) > abs(dy))
        return dx < 0 ? DIR_WEST : DIR_EAST;
    return dy < 0 ? DIR_NORTH : DIR_SOUTH;
}

// Special: VAR_RESULT = whether the player can take the leader's spot (the leader is the last
// person talked to, and the player stands facing them). Plans the moves.
void GymCandidate_PlanHold(void)
{
    static const s8 aside[][2] = {{-2, 0}, {2, 0}, {-2, -1}, {2, -1}, {-3, 0}, {3, 0}, {-2, 1}, {2, 1},
                                  {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {-3, -1}, {3, -1}};
    u8 objId;
    s16 px, py;
    u32 i;
    struct ObjectEvent *leader;

    gSpecialVar_Result = FALSE;
    sHolding = FALSE;
    if (TryGetObjectEventIdByLocalIdAndMap(gSpecialVar_LastTalked, gSaveBlock1Ptr->location.mapNum,
                                           gSaveBlock1Ptr->location.mapGroup, &objId))
        return;
    leader = &gObjectEvents[objId];
    PlayerGetDestCoords(&px, &py);
    sHoldDir = GetPlayerFacingDirection();
    if (px + sDirDx[sHoldDir] != leader->currentCoords.x || py + sDirDy[sHoldDir] != leader->currentCoords.y)
        return;
    sHoldLeaderX = leader->currentCoords.x;
    sHoldLeaderY = leader->currentCoords.y;
    for (i = 0; i < ARRAY_COUNT(aside); i++)
    {
        s16 x = sHoldLeaderX + aside[i][0], y = sHoldLeaderY + aside[i][1];
        if (VisitorCanWalk(x - MAP_OFFSET, y - MAP_OFFSET, MapGridGetElevationAt(sHoldLeaderX, sHoldLeaderY))
         && !(x == px && y == py))
        {
            sHoldAsideX = x;
            sHoldAsideY = y;
            sHoldLeaderObj = objId;
            sHolding = TRUE;
            gHoldPlayerIn[0] = sWalkFast[sHoldDir];
            gHoldPlayerIn[1] = sFace[sOpposite[sHoldDir]];
            gHoldPlayerIn[2] = MOVEMENT_ACTION_STEP_END;
            gHoldPlayerOut[0] = sWalkFast[sOpposite[sHoldDir]];
            gHoldPlayerOut[1] = sFace[sHoldDir];
            gHoldPlayerOut[2] = MOVEMENT_ACTION_STEP_END;
            gSpecialVar_Result = TRUE;
            return;
        }
    }
}

// Special: VAR_0x8007 = a stand-in shift (the leader leaves) -> gStringVar4 = what happens now
void GymCandidate_BufferHoldText(void)
{
    const struct GymCandidateLeader *leader = &gGymCandidateLeaders[Leader()];
    if (gSpecialVar_0x8007)
    {
        StringCopy(gStringVar3, leader->name);
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("{STR_VAR_3} left you in charge of\nthe GYM."));
    }
    else
    {
        StringExpandPlaceholders(gStringVar4, leader->observe);
    }
}

// Special: (in the dark) the leader steps aside to watch, facing their spot, or goes out of sight
void GymCandidate_HoldLeaderAside(void)
{
    struct ObjectEvent *leader;
    if (!sHolding)
        return;
    leader = &gObjectEvents[sHoldLeaderObj];
    MoveObjectEventToMapCoords(leader, sHoldAsideX, sHoldAsideY);
    ObjectEventTurn(leader, DirToward(sHoldAsideX, sHoldAsideY, sHoldLeaderX, sHoldLeaderY));
    leader->invisible = (gSpecialVar_0x8007 != 0);
}

// Special: VAR_RESULT = whether the player holds the leader's spot now
void GymCandidate_IsHolding(void)
{
    gSpecialVar_Result = sHolding;
}

// Special: (in the dark, after the player stepped back) the leader returns to their spot
void GymCandidate_HoldLeaderBack(void)
{
    struct ObjectEvent *leader;
    if (!sHolding)
        return;
    leader = &gObjectEvents[sHoldLeaderObj];
    MoveObjectEventToMapCoords(leader, sHoldLeaderX, sHoldLeaderY);
    ObjectEventTurn(leader, sOpposite[sHoldDir]);
    leader->invisible = FALSE;
    sHolding = FALSE;
}

// Special: VAR_RESULT = whether the visitor walked in (so walks back out)
void GymCandidate_VisitorWalked(void)
{
    gSpecialVar_Result = sVisitorWalked;
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

// Whether an overworld object's script is the trainer battle of a recruit whose trainer
// decoration is currently placed in the player's GYM. Such a trainer has moved to the GYM,
// so their overworld object doesn't spawn; they come back if the decoration is put away.
bool32 GymCandidate_IsPlacedRecruitScript(const u8 *script)
{
    u32 t, trainerId;

    if (script == NULL || script[0] != SCR_OP_TRAINERBATTLE)
        return FALSE;
    // opcode, flag bits, objEventLocalIdA, then opponentA (unaligned u16)
    trainerId = script[3] | (script[4] << 8);
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (gGymRecruits[t].decor != 0 && gGymRecruits[t].trainerId == trainerId)
            return GymDecor_CountPlaced(gGymRecruits[t].decor) > 0;
    }
    return FALSE;
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
