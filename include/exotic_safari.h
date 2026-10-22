#ifndef GUARD_EXOTIC_SAFARI_H
#define GUARD_EXOTIC_SAFARI_H

// Pokémon Canvas: the Exotic Pokémon Safari's hall doors (src/exotic_safari.c)
void ExoticSafari_OnLoad(void);
const unsigned char *ExoticSafari_GetAreaName(void);   // (unsigned char: u8, this header comes before global.h in places)
void ExoticSafari_OnStep(void);
void ExoticSafari_AreaOnTransition(void);
void ExoticSafari_Kiosk(void);
void ExoticSafari_BufferSeal(void);
void ExoticSafari_CountStakes(void);
void ExoticSafari_MarkGone(void);
void ExoticSafari_TryAppear(void);
void ExoticSafari_AreaOnLoad(void);
void ExoticSafari_Lamp(void);
void ExoticSafari_Stone(void);
void ExoticSafari_SetupDojo(void);
void ExoticSafari_TakeRelic(void);
void ExoticSafari_CountStarters(void);
void ExoticSafari_TakeCell(void);
void ExoticSafari_CountCells(void);
void ExoticSafari_LegendForm(void);
void ExoticSafari_IsFurfrou(void);
void ExoticSafari_TrimFurfrou(void);
void ExoticSafari_TakeKey(void);
void ExoticSafari_LegendCaught(void);

#endif // GUARD_EXOTIC_SAFARI_H
