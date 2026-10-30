#ifndef GUARD_CONSTANTS_DAD_STORY_H
#define GUARD_CONSTANTS_DAD_STORY_H

// Pokémon Canvas: the Dad story (data/scripts/dad_story.inc, src/dad_story.c). VAR_DAD_STORY's steps:
#define DAD_STORY_NONE          0
#define DAD_STORY_SENDOFF       1   // got the Pokédex: MOM waits outside OAK's lab to send the player off
#define DAD_STORY_ON_THE_ROAD   2   // ...and has
#define DAD_STORY_MOM_CALLING   3   // joined TEAM ROCKET at NUGGET BRIDGE; the rival sent the player home
#define DAD_STORY_GOT_DADS_BALL 4   // MOM gave the player Dad's Type Ball
#define DAD_STORY_SILPH_HOME    5   // MOM took the player home from SILPH CO. (the basement scene is next)
#define DAD_STORY_BASEMENT      6   // MOM showed the player the basement
#define DAD_STORY_GIOVANNI      7   // beat GIOVANNI once at VIRIDIAN; his last battle is next
#define DAD_STORY_DONE          8   // GIOVANNI is gone; DAD's CHANSEY, the notebook, the switch

// The basement's notes (VAR_0x8004 for ShowNotePage)
#define NOTE_PAGE_NOTE_1    0
#define NOTE_PAGE_NOTE_2    1
#define NOTE_PAGE_NOTE_3    2
#define NOTE_PAGE_NOTE_4    3
#define NOTE_PAGE_NOTEBOOK  4
#define NOTE_PAGE_COUNT     5

// Dad's Chansey
#define DADS_CHANSEY_LEVEL  50

#endif // GUARD_CONSTANTS_DAD_STORY_H
