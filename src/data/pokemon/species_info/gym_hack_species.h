// Gym Trainer hack: species of its own

    // Bill, gone wrong: his Cell Separation experiment with a Clefable, run backwards (the Gauntlet's
    // finale, src/gauntlet.c; art by tools/gym_hack/bill_art.py). Not in the Pokédex, not obtainable.
    [SPECIES_CLEFFABILL] =
    {
        .baseHP        = 150,
        .baseAttack    = 80,
        .baseDefense   = 130,
        .baseSpeed     = 120,
        .baseSpAttack  = 170,
        .baseSpDefense = 150,
        .types = MON_TYPES(TYPE_FAIRY, TYPE_GHOST),
        .catchRate = 3,
        .expYield = 340,
        .evYield_SpAttack = 3,
        .genderRatio = MON_MALE,
        .eggCycles = 120,
        .friendship = 0,
        .growthRate = GROWTH_SLOW,
        .eggGroups = MON_EGG_GROUPS(EGG_GROUP_NO_EGGS_DISCOVERED),
        .abilities = { ABILITY_UNAWARE, ABILITY_MAGIC_GUARD, ABILITY_CURSED_BODY },
        .bodyColor = BODY_COLOR_PINK,
        .speciesName = _("Cleffabill"),
        .cryId = CRY_CLEFABLE,
        .natDexNum = NATIONAL_DEX_NONE,
        .categoryName = _("Mistake"),
        .height = 17,
        .weight = 720,
        .description = COMPOUND_STRING(
            "A man and a Clefable, fused by a Cell\n"
            "Separation System run in reverse. It is\n"
            "in constant pain, and its body is slowly\n"
            "coming apart into something not alive."),
        .pokemonScale = 256,
        .pokemonOffset = 0,
        .trainerScale = 256,
        .trainerOffset = 0,
        .frontPic = gMonFrontPic_Cleffabill,
        .frontPicSize = MON_COORDS_SIZE(64, 64),
        .frontPicYOffset = 0,
        .frontAnimFrames = ANIM_FRAMES(
            ANIMCMD_FRAME(0, 12),
            ANIMCMD_FRAME(1, 24),
            ANIMCMD_FRAME(0, 12),
            ANIMCMD_FRAME(1, 24),
        ),
        .frontAnimId = ANIM_SHAKE_GLOW_PURPLE_SLOW,
        .backPic = gMonBackPic_Cleffabill,
        .backPicSize = MON_COORDS_SIZE(64, 64),
        .backPicYOffset = 0,
        .backAnimId = BACK_ANIM_SHAKE_GLOW_BLUE,
        .palette = gMonPalette_Cleffabill,
        .shinyPalette = gMonShinyPalette_Cleffabill,
        .iconSprite = gMonIcon_Cleffabill,
        .iconPalIndex = 2,
        .pokemonJumpType = PKMN_JUMP_TYPE_NONE,
        SHADOW(0, 9, SHADOW_SIZE_M)
        FOOTPRINT(Cleffabill)
        .levelUpLearnset = sClefableLevelUpLearnset,
        .teachableLearnset = sClefableTeachableLearnset,
        .isFrontierBanned = TRUE,
        .cannotBeTraded = TRUE,
    },
