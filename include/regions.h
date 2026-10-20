#ifndef GUARD_REGIONS_H
#define GUARD_REGIONS_H

#include "global.h"
#include "constants/regions.h"

enum KantoSubRegion GetKantoSubregion(u32 mapSecId);

// Gym Trainer hack: Johto's map sections (tools/gym_hack/import_johto.py adds them in this order)
#define JOHTO_MAPSEC_START  MAPSEC_OLIVINE_CITY
#define JOHTO_MAPSEC_END    MAPSEC_ROUTE_42

static inline enum Region GetRegionForSectionId(u32 sectionId)
{
    if (sectionId >= KANTO_MAPSEC_START && sectionId < MAPSEC_SPECIAL_AREA)
        return REGION_KANTO;
    // There's no Johto region map: it counts as KANTO (map screens, music), at VERMILION (the ferry)
    if (sectionId >= JOHTO_MAPSEC_START && sectionId <= JOHTO_MAPSEC_END)
        return REGION_KANTO;
    return REGION_HOENN;
}

static inline enum Region GetCurrentRegion(void)
{
    return GetRegionForSectionId(gMapHeader.regionMapSectionId);
}

#endif // GUARD_REGIONS_H
