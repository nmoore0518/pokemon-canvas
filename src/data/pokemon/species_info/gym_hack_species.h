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

    // Pokémon Canvas: DAD's CHANSEY, made after the player was born "to heal the damage he had done".
    // A Chansey to look at (its pictures, name, cry and moves), but Attack and Sp. Atk 100. One of a
    // kind: MOM gives it after GIOVANNI's last battle (src/dad_story.c). It doesn't evolve or breed.
    [SPECIES_CHANSEY_DAD] =
    {
        .baseHP        = 250,
        .baseAttack    = 100,
        .baseDefense   = 5,
        .baseSpeed     = 50,
        .baseSpAttack  = 100,
        .baseSpDefense = 105,
        .types = MON_TYPES(TYPE_NORMAL),
        .catchRate = 30,
        .expYield = (P_UPDATED_EXP_YIELDS >= GEN_5) ? 395 : 255,
        .evYield_HP = 2,
        .genderRatio = MON_FEMALE,
        .eggCycles = 40,
        .friendship = 140,
        .growthRate = GROWTH_FAST,
        .eggGroups = MON_EGG_GROUPS(EGG_GROUP_NO_EGGS_DISCOVERED),
        .abilities = { ABILITY_NATURAL_CURE, ABILITY_SERENE_GRACE, ABILITY_HEALER },
        .bodyColor = BODY_COLOR_PINK,
        .speciesName = _("Chansey"),
        .cryId = CRY_CHANSEY,
        .natDexNum = NATIONAL_DEX_CHANSEY,
        .categoryName = _("Egg"),
        .height = 11,
        .weight = 346,
        .description = COMPOUND_STRING(
            "Chansey lay nutritionally excellent eggs\n"
            "every day. The eggs are so delicious, they\n"
            "are eagerly devoured by even those people\n"
            "who have lost their appetite."),
        .pokemonScale = 257,
        .pokemonOffset = 7,
        .trainerScale = 256,
        .trainerOffset = 0,
        .frontPic = gMonFrontPic_Chansey,
        .frontPicSize = MON_COORDS_SIZE(56, 48),
        .frontPicYOffset = 9,
        .frontAnimFrames = ANIM_FRAMES(
            ANIMCMD_FRAME(0, 30),
            ANIMCMD_FRAME(1, 30),
            ANIMCMD_FRAME(0, 20),
        ),
        .frontAnimId = P_GBA_STYLE_SPECIES_GFX ? ANIM_V_SQUISH_AND_BOUNCE_SLOW : ANIM_V_SQUISH_AND_BOUNCE,
        .backPic = gMonBackPic_Chansey,
        .backPicSize = MON_COORDS_SIZE(64, 48),
        .backPicYOffset = 11,
        .backAnimId = BACK_ANIM_CONCAVE_ARC_SMALL,
        .palette = gMonPalette_Chansey,
        .shinyPalette = gMonShinyPalette_Chansey,
        .iconSprite = gMonIcon_Chansey,
        .iconPalIndex = 0,
        .pokemonJumpType = PKMN_JUMP_TYPE_NONE,
        SHADOW(1, 4, SHADOW_SIZE_L)
        FOOTPRINT(Chansey)
        OVERWORLD(
            sPicTable_Chansey,
            SIZE_32x32,
            SHADOW_SIZE_M,
            TRACKS_FOOT,
            sAnimTable_Following,
            gOverworldPalette_Chansey,
            gShinyOverworldPalette_Chansey
        )
        .levelUpLearnset = sChanseyLevelUpLearnset,
        .teachableLearnset = sChanseyTeachableLearnset,
        .isFrontierBanned = TRUE,
        .cannotBeTraded = TRUE,
    },

