#ifndef GUARD_GYM_LEGEND_H
#define GUARD_GYM_LEGEND_H

// Gym Trainer hack: the legendary Pokémon of the player's type (src/gym_legend.c)
extern const u16 gGymLegends[NUMBER_OF_MON_TYPES];

u16 GymLegend_GetSpecies(u32 type);
void GymLegend_Setup(void);
void GymLegend_FindSpot(void);
void GymLegend_OnLoad(void);
void GymLegend_CreateMon(void);
void GymLegend_PlanCamera(void);
extern u8 gGymLegendCameraIn[];
extern u8 gGymLegendCameraOut[];

#endif // GUARD_GYM_LEGEND_H
