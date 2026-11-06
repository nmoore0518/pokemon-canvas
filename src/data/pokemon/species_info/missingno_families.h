// Pokémon Canvas: MissingNo. (in a *_families.h file so the teachables generator reads its teachingType)

    // Pokémon Canvas: MissingNo., as Red and Blue had it: its stats, its BIRD/NORMAL typing (BIRD was
    // never given a matchup: here it's typeless, TYPE_MYSTERY, shown as BIRD by the summary; with no
    // real first type it can't join a gym), its moves and the TMs and HMs it could learn there. Not
    // in the Pokédex (#000). Cinnabar's east coast, after the Viridian old man's demo (src/missingno.c).
    [SPECIES_MISSINGNO] =
    {
        .baseHP        = 33,
        .baseAttack    = 136,
        .baseDefense   = 0,
        .baseSpeed     = 29,
        .baseSpAttack  = 6,
        .baseSpDefense = 6,
        .types = MON_TYPES(TYPE_MYSTERY, TYPE_NORMAL),
        .catchRate = 29,
        .expYield = 0,
        .genderRatio = MON_GENDERLESS,
        .eggCycles = 120,
        .friendship = 0,
        .growthRate = GROWTH_MEDIUM_SLOW,
        .eggGroups = MON_EGG_GROUPS(EGG_GROUP_NO_EGGS_DISCOVERED),
        .abilities = { ABILITY_NONE, ABILITY_NONE, ABILITY_NONE },
        .bodyColor = BODY_COLOR_PURPLE,
        .noFlip = TRUE,
        .speciesName = _("MissingNo."),
        .cryId = CRY_NIDORAN_M,
        .natDexNum = NATIONAL_DEX_NONE,
        .categoryName = _("???"),
        .height = 30,
        .weight = 15908,
        .description = COMPOUND_STRING(
            "Comment to be written."),
        .pokemonScale = 256,
        .pokemonOffset = 0,
        .trainerScale = 256,
        .trainerOffset = 0,
        .frontPic = gMonFrontPic_Missingno,
        .frontPicSize = MON_COORDS_SIZE(64, 64),
        .frontPicYOffset = 0,
        .frontAnimFrames = ANIM_FRAMES(
            ANIMCMD_FRAME(1, 4),
            ANIMCMD_FRAME(0, 4),
            ANIMCMD_FRAME(1, 4),
            ANIMCMD_FRAME(0, 4),
        ),
        .frontAnimId = ANIM_V_SHAKE,
        .backPic = gMonBackPic_Missingno,
        .backPicSize = MON_COORDS_SIZE(64, 64),
        .backPicYOffset = 0,
        .backAnimId = BACK_ANIM_SHAKE_GLOW_BLUE,
        .palette = gMonPalette_Missingno,
        .shinyPalette = gMonShinyPalette_Missingno,
        .iconSprite = gMonIcon_Missingno,
        .iconPalIndex = 2,
        .pokemonJumpType = PKMN_JUMP_TYPE_NONE,
        SHADOW(0, 9, SHADOW_SIZE_M)
        FOOTPRINT(Missingno)
        .teachingType = TM_ILLITERATE,
        .levelUpLearnset = sMissingnoLevelUpLearnset,
        .teachableLearnset = sMissingnoTeachableLearnset,
        .isFrontierBanned = TRUE,
    },
