#ifndef GUARD_DAD_STORY_H
#define GUARD_DAD_STORY_H

// Pokémon Canvas: the Dad story (src/dad_story.c, data/scripts/dad_story.inc)
#include "constants/dad_story.h"

u16 DadStory_GetAce(u32 type);
void DadStory_SynthesizeParty(struct Pokemon *party);

// Specials
void ShowNotePage(void);
void DadStory_BufferAce(void);
void DadStory_GetGymTypeBall(void);
void DadStory_GiveChansey(void);

#endif // GUARD_DAD_STORY_H
