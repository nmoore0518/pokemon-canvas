// Pokémon Canvas: the painter in Pewter City and her Smeargle (data/scripts/painter.inc).
//
// Her easel is a way into her paintings: each is one of a list of real places
// (src/data/painting_destinations.h, tools/gym_hack/build_painting_world.py), picked at random,
// seen in scrambled colours, with nobody there but a figure made of paint who battles (a double
// battle). Nothing in a painting gets out: walking off the map's edge is blocked, and any warp
// (a door, Fly, an Escape Rope, Dig) comes back out of the easel in her loft.
//   - After the first badge: she lends her Smeargle (unpainted, typeless: SPECIES_SMEARGLE_CANVAS)
//     to watch a real trainer battle; win with it and it finds its colour: she gives the player
//     a Canvas Smeargle of their gym's type (Lv. CANVAS_SMEARGLE_LEVEL).
//   - After each badge after that (one each; a loss can be retried): a Sketch quest. The player
//     picks a type and a move slot; she paints Sketch into the slot; the figure's Pokémon know
//     only moves of that type, from a list that grows with the badges (sSketchMoves). A move
//     sketched and the battle won is kept; otherwise the slot gets its old move back.
//   - After the League: the Mega's painting, its figure with a Mega; the prize the SMEARGLITE
//     (a key item: Canvas Smeargle Mega Evolve with it in the bag). Then Sketch quests any time.
#include "global.h"
#include "painter.h"
#include "constants/rgb.h"
#include "battle.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "fieldmap.h"
#include "gym_challenge.h"
#include "item.h"
#include "malloc.h"
#include "move.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/moves.h"
#include "constants/painter.h"
#include "constants/species.h"
#include "constants/vars.h"

extern const u8 PaintWorld_EventScript_Figure[];

struct PaintingDestination
{
    u8 mapGroup;
    u8 mapNum;
    u8 x, y;
};

#include "data/painting_destinations.h"

// Where the player comes back out: in front of the easel
#define LOFT_X 8
#define LOFT_Y 4

// ---------------------------------------------------------------------------------------------
// Sketch quests' moves: by type, in tiers (badges 2-3, 4-5, 6-8, after the League). A quest's
// figure knows moves from its tier and the ones before. Nothing that breaks the game: no sleep
// moves, no one-hit KOs, no big setup moves, at most about 100 power.
// ---------------------------------------------------------------------------------------------
#define SKETCH_TIERS 4
#define SKETCH_PER_TIER 4
static const u16 sSketchMoves[NUMBER_OF_MON_TYPES][SKETCH_TIERS][SKETCH_PER_TIER] =
{
    [TYPE_NORMAL]   = {{MOVE_SWIFT, MOVE_QUICK_ATTACK, MOVE_HEADBUTT, MOVE_PROTECT},
                       {MOVE_HYPER_VOICE, MOVE_BODY_SLAM, MOVE_ENCORE, MOVE_FACADE},
                       {MOVE_TRI_ATTACK, MOVE_SLASH, MOVE_SAFEGUARD, MOVE_SUBSTITUTE},
                       {MOVE_EXTREME_SPEED, MOVE_RECOVER, MOVE_DOUBLE_EDGE, MOVE_FAKE_OUT}},
    [TYPE_FIGHTING] = {{MOVE_ROCK_SMASH, MOVE_MACH_PUNCH, MOVE_DOUBLE_KICK, MOVE_LOW_KICK},
                       {MOVE_VACUUM_WAVE, MOVE_BRICK_BREAK, MOVE_DETECT, MOVE_BULK_UP},
                       {MOVE_AURA_SPHERE, MOVE_DRAIN_PUNCH, MOVE_FORCE_PALM, MOVE_LOW_SWEEP},
                       {MOVE_FOCUS_BLAST, MOVE_CLOSE_COMBAT, MOVE_QUICK_GUARD, MOVE_WIDE_GUARD}},
    [TYPE_FLYING]   = {{MOVE_GUST, MOVE_WING_ATTACK, MOVE_PECK, MOVE_FEATHER_DANCE},
                       {MOVE_AIR_CUTTER, MOVE_AERIAL_ACE, MOVE_TAILWIND, MOVE_PLUCK},
                       {MOVE_AIR_SLASH, MOVE_ROOST, MOVE_ACROBATICS, MOVE_DRILL_PECK},
                       {MOVE_HURRICANE, MOVE_BRAVE_BIRD, MOVE_DEFOG, MOVE_FLY}},
    [TYPE_POISON]   = {{MOVE_ACID, MOVE_POISON_STING, MOVE_SMOG, MOVE_POISON_POWDER},
                       {MOVE_VENOSHOCK, MOVE_POISON_FANG, MOVE_CLEAR_SMOG, MOVE_ACID_SPRAY},
                       {MOVE_SLUDGE_BOMB, MOVE_TOXIC, MOVE_POISON_JAB, MOVE_SLUDGE},
                       {MOVE_SLUDGE_WAVE, MOVE_TOXIC_SPIKES, MOVE_GUNK_SHOT, MOVE_BANEFUL_BUNKER}},
    [TYPE_GROUND]   = {{MOVE_MUD_SLAP, MOVE_SAND_ATTACK, MOVE_MUD_SHOT, MOVE_BONE_CLUB},
                       {MOVE_BULLDOZE, MOVE_DIG, MOVE_MUD_BOMB, MOVE_BONEMERANG},
                       {MOVE_EARTH_POWER, MOVE_DRILL_RUN, MOVE_SCORCHING_SANDS, MOVE_STOMPING_TANTRUM},
                       {MOVE_EARTHQUAKE, MOVE_SPIKES, MOVE_HIGH_HORSEPOWER, MOVE_SHORE_UP}},
    [TYPE_ROCK]     = {{MOVE_ROCK_THROW, MOVE_ROCK_TOMB, MOVE_SANDSTORM, MOVE_SMACK_DOWN},
                       {MOVE_ANCIENT_POWER, MOVE_ROCK_SLIDE, MOVE_ROCK_BLAST, MOVE_ROCK_POLISH},
                       {MOVE_POWER_GEM, MOVE_STONE_EDGE, MOVE_STEALTH_ROCK, MOVE_ROCK_SLIDE},
                       {MOVE_METEOR_BEAM, MOVE_HEAD_SMASH, MOVE_DIAMOND_STORM, MOVE_POWER_GEM}},
    [TYPE_BUG]      = {{MOVE_BUG_BITE, MOVE_STRUGGLE_BUG, MOVE_STRING_SHOT, MOVE_FURY_CUTTER},
                       {MOVE_SIGNAL_BEAM, MOVE_U_TURN, MOVE_SILVER_WIND, MOVE_PIN_MISSILE},
                       {MOVE_BUG_BUZZ, MOVE_X_SCISSOR, MOVE_LEECH_LIFE, MOVE_POLLEN_PUFF},
                       {MOVE_STICKY_WEB, MOVE_LUNGE, MOVE_FIRST_IMPRESSION, MOVE_MEGAHORN}},
    [TYPE_GHOST]    = {{MOVE_ASTONISH, MOVE_LICK, MOVE_NIGHT_SHADE, MOVE_CONFUSE_RAY},
                       {MOVE_HEX, MOVE_SHADOW_CLAW, MOVE_SHADOW_SNEAK, MOVE_OMINOUS_WIND},
                       {MOVE_SHADOW_BALL, MOVE_PHANTOM_FORCE, MOVE_SHADOW_PUNCH, MOVE_SPITE},
                       {MOVE_POLTERGEIST, MOVE_DESTINY_BOND, MOVE_INFERNAL_PARADE, MOVE_BITTER_MALICE}},
    [TYPE_STEEL]    = {{MOVE_METAL_CLAW, MOVE_IRON_DEFENSE, MOVE_METAL_SOUND, MOVE_BULLET_PUNCH},
                       {MOVE_MIRROR_SHOT, MOVE_MAGNET_BOMB, MOVE_STEEL_WING, MOVE_GYRO_BALL},
                       {MOVE_FLASH_CANNON, MOVE_IRON_HEAD, MOVE_ANCHOR_SHOT, MOVE_SMART_STRIKE},
                       {MOVE_METEOR_MASH, MOVE_STEEL_ROLLER, MOVE_IRON_TAIL, MOVE_KINGS_SHIELD}},
    [TYPE_FIRE]     = {{MOVE_EMBER, MOVE_FIRE_SPIN, MOVE_INCINERATE, MOVE_FLAME_CHARGE},
                       {MOVE_FLAME_WHEEL, MOVE_WILL_O_WISP, MOVE_FIRE_FANG, MOVE_MYSTICAL_FIRE},
                       {MOVE_FLAMETHROWER, MOVE_LAVA_PLUME, MOVE_FIRE_PUNCH, MOVE_SUNNY_DAY},
                       {MOVE_HEAT_WAVE, MOVE_FIRE_BLAST, MOVE_FLARE_BLITZ, MOVE_BURNING_JEALOUSY}},
    [TYPE_WATER]    = {{MOVE_WATER_GUN, MOVE_BUBBLE_BEAM, MOVE_WITHDRAW, MOVE_AQUA_JET},
                       {MOVE_WATER_PULSE, MOVE_BRINE, MOVE_RAIN_DANCE, MOVE_CHILLING_WATER},
                       {MOVE_SURF, MOVE_SCALD, MOVE_WATERFALL, MOVE_AQUA_RING},
                       {MOVE_HYDRO_PUMP, MOVE_FLIP_TURN, MOVE_LIQUIDATION, MOVE_MUDDY_WATER}},
    [TYPE_GRASS]    = {{MOVE_ABSORB, MOVE_MEGA_DRAIN, MOVE_RAZOR_LEAF, MOVE_STUN_SPORE},
                       {MOVE_GIGA_DRAIN, MOVE_MAGICAL_LEAF, MOVE_LEECH_SEED, MOVE_BULLET_SEED},
                       {MOVE_ENERGY_BALL, MOVE_SEED_BOMB, MOVE_SYNTHESIS, MOVE_GRASS_KNOT},
                       {MOVE_LEAF_STORM, MOVE_STRENGTH_SAP, MOVE_POWER_WHIP, MOVE_GRASSY_TERRAIN}},
    [TYPE_ELECTRIC] = {{MOVE_THUNDER_SHOCK, MOVE_THUNDER_WAVE, MOVE_SPARK, MOVE_CHARGE},
                       {MOVE_SHOCK_WAVE, MOVE_CHARGE_BEAM, MOVE_THUNDER_FANG, MOVE_ELECTROWEB},
                       {MOVE_THUNDERBOLT, MOVE_DISCHARGE, MOVE_VOLT_SWITCH, MOVE_THUNDER_PUNCH},
                       {MOVE_THUNDER, MOVE_WILD_CHARGE, MOVE_ELECTRIC_TERRAIN, MOVE_PARABOLIC_CHARGE}},
    [TYPE_PSYCHIC]  = {{MOVE_CONFUSION, MOVE_PSYBEAM, MOVE_REFLECT, MOVE_LIGHT_SCREEN},
                       {MOVE_PSYCHO_CUT, MOVE_ZEN_HEADBUTT, MOVE_STORED_POWER, MOVE_MAGIC_COAT},
                       {MOVE_PSYCHIC, MOVE_PSYSHOCK, MOVE_CALM_MIND, MOVE_TRICK_ROOM},
                       {MOVE_FUTURE_SIGHT, MOVE_EXPANDING_FORCE, MOVE_PSYCHIC_TERRAIN, MOVE_TRICK}},
    [TYPE_ICE]      = {{MOVE_POWDER_SNOW, MOVE_ICE_SHARD, MOVE_ICY_WIND, MOVE_MIST},
                       {MOVE_AURORA_BEAM, MOVE_FROST_BREATH, MOVE_ICE_FANG, MOVE_AVALANCHE},
                       {MOVE_ICE_BEAM, MOVE_ICE_PUNCH, MOVE_FREEZE_DRY, MOVE_HAIL},
                       {MOVE_BLIZZARD, MOVE_ICICLE_CRASH, MOVE_AURORA_VEIL, MOVE_TRIPLE_AXEL}},
    [TYPE_DRAGON]   = {{MOVE_TWISTER, MOVE_DRAGON_RAGE, MOVE_DRAGON_BREATH, MOVE_DUAL_CHOP},
                       {MOVE_DRAGON_TAIL, MOVE_DRAGON_CLAW, MOVE_BREAKING_SWIPE, MOVE_SCALE_SHOT},
                       {MOVE_DRAGON_PULSE, MOVE_DRAGON_RUSH, MOVE_DRAGON_DARTS, MOVE_DRAGON_CHEER},
                       {MOVE_DRACO_METEOR, MOVE_OUTRAGE, MOVE_DRAGON_ENERGY, MOVE_DRAGON_PULSE}},
    [TYPE_DARK]     = {{MOVE_BITE, MOVE_PURSUIT, MOVE_SNARL, MOVE_FAKE_TEARS},
                       {MOVE_FEINT_ATTACK, MOVE_KNOCK_OFF, MOVE_THIEF, MOVE_TAUNT},
                       {MOVE_DARK_PULSE, MOVE_CRUNCH, MOVE_NIGHT_SLASH, MOVE_TORMENT},
                       {MOVE_NASTY_PLOT, MOVE_SUCKER_PUNCH, MOVE_FOUL_PLAY, MOVE_PARTING_SHOT}},
    [TYPE_FAIRY]    = {{MOVE_FAIRY_WIND, MOVE_DISARMING_VOICE, MOVE_CHARM, MOVE_SWEET_KISS},
                       {MOVE_DRAINING_KISS, MOVE_MISTY_TERRAIN, MOVE_DAZZLING_GLEAM, MOVE_FLORAL_HEALING},
                       {MOVE_PLAY_ROUGH, MOVE_MOONLIGHT, MOVE_SPIRIT_BREAK, MOVE_DECORATE},
                       {MOVE_MOONBLAST, MOVE_STRANGE_STEAM, MOVE_MISTY_EXPLOSION, MOVE_ALLURING_VOICE}},
};

// The Mega's painting: its figure's Mega (and its stone)
static const struct { u16 species, stone; } sMegaFigures[] =
{
    {SPECIES_AMPHAROS, ITEM_AMPHAROSITE}, {SPECIES_GARDEVOIR, ITEM_GARDEVOIRITE}, {SPECIES_LUCARIO, ITEM_LUCARIONITE},
    {SPECIES_MANECTRIC, ITEM_MANECTITE}, {SPECIES_ALAKAZAM, ITEM_ALAKAZITE}, {SPECIES_GENGAR, ITEM_GENGARITE},
    {SPECIES_ABSOL, ITEM_ABSOLITE}, {SPECIES_ALTARIA, ITEM_ALTARIANITE}, {SPECIES_MEDICHAM, ITEM_MEDICHAMITE},
    {SPECIES_HOUNDOOM, ITEM_HOUNDOOMINITE}, {SPECIES_AGGRON, ITEM_AGGRONITE}, {SPECIES_SCIZOR, ITEM_SCIZORITE},
};

// ---------------------------------------------------------------------------------------------
// Smeargle
// ---------------------------------------------------------------------------------------------
bool32 Painter_IsCanvasSmeargle(u32 species)
{
    return species >= SPECIES_SMEARGLE_CANVAS && species <= SPECIES_SMEARGLE_CANVAS_FAIRY_MEGA;
}

// The Canvas Smeargle form of a type (TYPE_MYSTERY or none: the unpainted one)
u32 Painter_SmeargleOfType(u32 type)
{
    static const u16 sForms[NUMBER_OF_MON_TYPES] =
    {
        [TYPE_NORMAL] = SPECIES_SMEARGLE_CANVAS_NORMAL, [TYPE_FIGHTING] = SPECIES_SMEARGLE_CANVAS_FIGHTING,
        [TYPE_FLYING] = SPECIES_SMEARGLE_CANVAS_FLYING, [TYPE_POISON] = SPECIES_SMEARGLE_CANVAS_POISON,
        [TYPE_GROUND] = SPECIES_SMEARGLE_CANVAS_GROUND, [TYPE_ROCK] = SPECIES_SMEARGLE_CANVAS_ROCK,
        [TYPE_BUG] = SPECIES_SMEARGLE_CANVAS_BUG, [TYPE_GHOST] = SPECIES_SMEARGLE_CANVAS_GHOST,
        [TYPE_STEEL] = SPECIES_SMEARGLE_CANVAS_STEEL, [TYPE_FIRE] = SPECIES_SMEARGLE_CANVAS_FIRE,
        [TYPE_WATER] = SPECIES_SMEARGLE_CANVAS_WATER, [TYPE_GRASS] = SPECIES_SMEARGLE_CANVAS_GRASS,
        [TYPE_ELECTRIC] = SPECIES_SMEARGLE_CANVAS_ELECTRIC, [TYPE_PSYCHIC] = SPECIES_SMEARGLE_CANVAS_PSYCHIC,
        [TYPE_ICE] = SPECIES_SMEARGLE_CANVAS_ICE, [TYPE_DRAGON] = SPECIES_SMEARGLE_CANVAS_DRAGON,
        [TYPE_DARK] = SPECIES_SMEARGLE_CANVAS_DARK, [TYPE_FAIRY] = SPECIES_SMEARGLE_CANVAS_FAIRY,
    };
    if (type < NUMBER_OF_MON_TYPES && sForms[type] != SPECIES_NONE)
        return sForms[type];
    return SPECIES_SMEARGLE_CANVAS;
}

// The party slot of the player's own Canvas Smeargle (a painted one), or PARTY_SIZE
static u32 FindPaintedSmeargle(void)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 species = GetMonData(mon, MON_DATA_SPECIES);
        if (Painter_IsCanvasSmeargle(species) && species != SPECIES_SMEARGLE_CANVAS && !GetMonData(mon, MON_DATA_IS_EGG))
            return i;
    }
    return PARTY_SIZE;
}

static u32 StoredPersonality(void)
{
    return VarGet(VAR_PAINTER_MON_LO) | ((u32)VarGet(VAR_PAINTER_MON_HI) << 16);
}

static void StorePersonality(struct Pokemon *mon)
{
    u32 pid = GetMonData(mon, MON_DATA_PERSONALITY);
    VarSet(VAR_PAINTER_MON_LO, pid & 0xFFFF);
    VarSet(VAR_PAINTER_MON_HI, pid >> 16);
}

static u32 FindStoredSmeargle(void)
{
    u32 i, pid = StoredPersonality();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (Painter_IsCanvasSmeargle(GetMonData(mon, MON_DATA_SPECIES)) && GetMonData(mon, MON_DATA_PERSONALITY) == pid)
            return i;
    }
    return PARTY_SIZE;
}

// (to the front, for the double battle: Smeargle has to be in it)
static void MoveToFront(u32 slot)
{
    if (slot != 0 && slot < PARTY_SIZE)
    {
        struct Pokemon *tmp = Alloc(sizeof(struct Pokemon));
        *tmp = gParties[B_TRAINER_PLAYER][0];
        gParties[B_TRAINER_PLAYER][0] = gParties[B_TRAINER_PLAYER][slot];
        gParties[B_TRAINER_PLAYER][slot] = *tmp;
        Free(tmp);
    }
}

static u32 BadgeCount(void)
{
    u32 i, n = 0;
    for (i = FLAG_BADGE01_GET; i < FLAG_BADGE01_GET + NUM_BADGES; i++)
        n += FlagGet(i);
    return n;
}

// ---------------------------------------------------------------------------------------------
// What she has for the player
// ---------------------------------------------------------------------------------------------
static bool32 SketchReady(void)
{
    if (FlagGet(FLAG_PAINTER_MEGA_DONE))
        return TRUE;
    return VarGet(VAR_PAINTER_SKETCHES) + 1 < BadgeCount();
}

static bool32 MegaReady(void)
{
    return FlagGet(FLAG_SYS_GAME_CLEAR) && !FlagGet(FLAG_PAINTER_MEGA_DONE);
}

u32 Painter_Offer(void)
{
    bool32 sketch, mega;
    if (BadgeCount() == 0)
        return PAINTER_OFFER_NO_BADGE;
    if (!FlagGet(FLAG_PAINTER_GAVE_SMEARGLE))
        return PAINTER_OFFER_FIRST;
    sketch = SketchReady();
    mega = MegaReady();
    if (!sketch && !mega)
        return PAINTER_OFFER_NOTHING;
    if (FindPaintedSmeargle() == PARTY_SIZE)
        return PAINTER_OFFER_NEED_SMEARGLE;
    if (sketch && mega)
        return PAINTER_OFFER_BOTH;
    return mega ? PAINTER_OFFER_MEGA : PAINTER_OFFER_SKETCH;
}

// Special: VAR_RESULT = PAINTER_OFFER_*
void Painter_GetOffer(void)
{
    gSpecialVar_Result = Painter_Offer();
}

// Special: VAR_RESULT = there's room in the party for her Smeargle
void Painter_CheckRoom(void)
{
    gSpecialVar_Result = CalculatePlayerPartyCount() < PARTY_SIZE;
}

// ---------------------------------------------------------------------------------------------
// Going in
// ---------------------------------------------------------------------------------------------
// (a destination, not the last one)
static u32 PickDestination(void)
{
    u32 last = VarGet(VAR_PAINTING_DEST), n = ARRAY_COUNT(sPaintingDestinations), pick;
    do
        pick = Random() % n;
    while (n > 1 && pick + 1 == last);
    return pick;
}

static void EnterPainting(u32 mode)
{
    // (the easel: a random place, never the last one twice)
    u32 dest = PickDestination();
    const struct PaintingDestination *d = &sPaintingDestinations[dest];
    VarSet(VAR_PAINTING_DEST, dest + 1);
    VarSet(VAR_PAINTING_MODE, mode);
    VarSet(VAR_PAINTING_RESULT, 0);
    VarSet(VAR_PAINTING_SEED, Random());
    FlagSet(FLAG_IN_PAINTING);
    SetWarpDestination(d->mapGroup, d->mapNum, WARP_ID_NONE, d->x, d->y);
    DoWarp();
    ResetInitialPlayerAvatarState();
}

// What the easel's ready for (she gets it ready; the player steps in at the easel): VAR_PAINTING_MODE,
// outside a painting. VAR_RESULT = PAINTING_MODE_*
void Painter_GetPending(void)
{
    gSpecialVar_Result = FlagGet(FLAG_IN_PAINTING) ? PAINTING_MODE_NONE : VarGet(VAR_PAINTING_MODE);
}

// Special: the first painting's ready
void Painter_PrepareFirst(void)
{
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_FIRST);
}

// Special: before a Sketch quest: VAR_0x8004 = the player's Smeargle's party slot (for the move
// deleter's screen); VAR_RESULT = it knows fewer than four moves (then no slot needs choosing:
// VAR_0x8005 = the first empty one)
void Painter_FindSmeargle(void)
{
    u32 slot = FindPaintedSmeargle(), i;
    gSpecialVar_0x8004 = slot;
    gSpecialVar_Result = FALSE;
    if (slot == PARTY_SIZE)
        return;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_MOVE1 + i) == MOVE_NONE)
        {
            gSpecialVar_0x8005 = i;
            gSpecialVar_Result = TRUE;
            return;
        }
    }
}

// Script type list index (0-17) -> type
static const u8 sTypeOrder[] =
{
    TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_ICE, TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND,
    TYPE_FLYING, TYPE_PSYCHIC, TYPE_BUG, TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON, TYPE_DARK, TYPE_STEEL, TYPE_FAIRY,
};

static u32 SketchTier(void)
{
    u32 badges = BadgeCount();
    if (FlagGet(FLAG_SYS_GAME_CLEAR))
        return 3;
    if (badges >= 6)
        return 2;
    if (badges >= 4)
        return 1;
    return 0;
}

// Special: a Sketch quest's ready. VAR_0x8004 = Smeargle's party slot, VAR_0x8005 = the move slot
// for Sketch, VAR_0x8006 = the type (its index in the script's list)
void Painter_PrepareSketch(void)
{
    u32 type = sTypeOrder[gSpecialVar_0x8006 % ARRAY_COUNT(sTypeOrder)];
    StorePersonality(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
    VarSet(VAR_PAINTER_SKETCH, gSpecialVar_0x8005 | (type << 8));
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_SKETCH);
}

// Special: the Mega's painting's ready (VAR_0x8004 = Smeargle's party slot)
void Painter_PrepareMega(void)
{
    StorePersonality(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_MEGA);
}

static u32 UsableMons(void)
{
    u32 i, n = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG))
            n++;
    }
    return n;
}

// Special: VAR_RESULT = PAINTER_ENTER_*: whether the player can step into the easel now
void Painter_CanEnter(void)
{
    u32 mode = VarGet(VAR_PAINTING_MODE);
    if (mode == PAINTING_MODE_FIRST)
    {
        if (CalculatePlayerPartyCount() >= PARTY_SIZE)
            gSpecialVar_Result = PAINTER_ENTER_NO_ROOM;
        else
            gSpecialVar_Result = PAINTER_ENTER_OK;     // (with hers, there are two)
        return;
    }
    if (FindStoredSmeargle() == PARTY_SIZE)
        gSpecialVar_Result = PAINTER_ENTER_NO_SMEARGLE;
    else if (UsableMons() < 2)
        gSpecialVar_Result = PAINTER_ENTER_TOO_FEW;
    else
        gSpecialVar_Result = PAINTER_ENTER_OK;
}

// Special (then waitstate): into the painting the easel's ready for. The first: her Smeargle lent
// (first in the party); a Sketch quest: Sketch painted into the chosen slot (the move that was
// there remembered), Smeargle to the front; the Mega's: Smeargle to the front
void Painter_Enter(void)
{
    u32 mode = VarGet(VAR_PAINTING_MODE), slot, value;
    switch (mode)
    {
    case PAINTING_MODE_FIRST:
    {
        struct Pokemon *mon = AllocZeroed(sizeof(struct Pokemon));
        CreateRandomMon(mon, SPECIES_SMEARGLE_CANVAS, CANVAS_SMEARGLE_LEVEL);   // (moves, IVs, stats)
        StorePersonality(mon);
        GiveScriptedMonToPlayer(mon, PARTY_SIZE);
        Free(mon);
        MoveToFront(FindStoredSmeargle());
        break;
    }
    case PAINTING_MODE_SKETCH:
    {
        u32 moveSlot = VarGet(VAR_PAINTER_SKETCH) & 0xFF;
        struct Pokemon *mon;
        slot = FindStoredSmeargle();
        mon = &gParties[B_TRAINER_PLAYER][slot];
        VarSet(VAR_PAINTER_SKETCH_MOVE, GetMonData(mon, MON_DATA_MOVE1 + moveSlot));
        value = MOVE_SKETCH;
        SetMonData(mon, MON_DATA_MOVE1 + moveSlot, &value);
        value = GetMovePP(MOVE_SKETCH);
        SetMonData(mon, MON_DATA_PP1 + moveSlot, &value);
        MoveToFront(slot);
        break;
    }
    case PAINTING_MODE_MEGA:
        MoveToFront(FindStoredSmeargle());
        break;
    }
    HealPlayerParty();
    EnterPainting(mode);
}

// ---------------------------------------------------------------------------------------------
// Inside
// ---------------------------------------------------------------------------------------------
bool32 Painting_IsActive(void)
{
    return FlagGet(FLAG_IN_PAINTING);
}

// (src/event_object_movement.c) Inside a painting, nobody's there but the figure; outside it, the
// figure isn't
bool32 Painting_HidesObject(const u8 *script)
{
    if (FlagGet(FLAG_IN_PAINTING))
        return script != PaintWorld_EventScript_Figure;
    return script == PaintWorld_EventScript_Figure;
}

// (src/event_object_movement.c) Nothing walks off a painting's edge
bool32 Painting_BlocksCoords(s16 x, s16 y)
{
    if (!FlagGet(FLAG_IN_PAINTING))
        return FALSE;
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    return x < 0 || y < 0 || x >= gMapHeader.mapLayout->width || y >= gMapHeader.mapLayout->height;
}

// (src/overworld.c) Any way out of a painting comes out of the easel
void Painting_RedirectWarp(struct WarpData *warp)
{
    u32 dest = VarGet(VAR_PAINTING_DEST);
    if (!FlagGet(FLAG_IN_PAINTING) || dest == 0 || dest > ARRAY_COUNT(sPaintingDestinations))
        return;
    if (warp->mapGroup == sPaintingDestinations[dest - 1].mapGroup && warp->mapNum == sPaintingDestinations[dest - 1].mapNum)
        return;
    FlagClear(FLAG_IN_PAINTING);
    VarSet(VAR_PAINTING_RETURN, 1);
    warp->mapGroup = MAP_GROUP(MAP_PEWTER_CITY_PAINTER_LOFT);
    warp->mapNum = MAP_NUM(MAP_PEWTER_CITY_PAINTER_LOFT);
    warp->warpId = WARP_ID_NONE;
    warp->x = LOFT_X;
    warp->y = LOFT_Y;
}

// (src/fieldmap.c) A painting's colours: each of the map's palettes turned round the colour wheel
// by its own amount, and brighter
static u16 Scramble(u16 colour, u32 turn)
{
    s32 r = colour & 31, g = (colour >> 5) & 31, b = (colour >> 10) & 31;
    s32 max = max(r, max(g, b)), min = min(r, min(g, b)), d = max - min, h, s, v = max;
    if (d == 0)
    {
        // (greys: tinted, a little)
        h = turn % 360;
        s = 6;
    }
    else
    {
        if (max == r)
            h = 60 * (g - b) / d;
        else if (max == g)
            h = 120 + 60 * (b - r) / d;
        else
            h = 240 + 60 * (r - g) / d;
        s = 31 * d / max;
        s = min(31, s * 3 / 2 + 4);
    }
    h = ((h + (s32)turn) % 360 + 360) % 360;
    v = min(31, v + 2);
    {
        s32 c = v * s / 31, x = c * (60 - abs(h % 120 - 60)) / 60, m = v - c, rr, gg, bb;
        switch (h / 60)
        {
        case 0:  rr = c; gg = x; bb = 0; break;
        case 1:  rr = x; gg = c; bb = 0; break;
        case 2:  rr = 0; gg = c; bb = x; break;
        case 3:  rr = 0; gg = x; bb = c; break;
        case 4:  rr = x; gg = 0; bb = c; break;
        default: rr = c; gg = 0; bb = x; break;
        }
        return RGB(rr + m, gg + m, bb + m);
    }
}

void Painting_TintPalettes(u32 offset, u32 count, bool32 skipFaded)
{
    u32 i, seed;
    if (!FlagGet(FLAG_IN_PAINTING))
        return;
    seed = VarGet(VAR_PAINTING_SEED);
    for (i = 0; i < count; i++)
    {
        u32 slot = (offset + i) / 16;
        if ((offset + i) % 16 == 0)
            continue;
        gPlttBufferUnfaded[offset + i] = Scramble(gPlttBufferUnfaded[offset + i], (seed % 360) + slot * 67);
        if (!skipFaded)
            gPlttBufferFaded[offset + i] = gPlttBufferUnfaded[offset + i];
    }
}

// (src/battle_script_commands.c) In a Sketch quest, Sketch copies only the quest's moves (not the
// player's other Pokémon's)
static bool32 IsQuestMove(u32 move)
{
    u32 type = VarGet(VAR_PAINTER_SKETCH) >> 8, tier = SketchTier(), t, i;
    if (type >= NUMBER_OF_MON_TYPES)
        return FALSE;
    for (t = 0; t <= tier; t++)
        for (i = 0; i < SKETCH_PER_TIER; i++)
            if (sSketchMoves[type][t][i] == move)
                return TRUE;
    return FALSE;
}

bool32 Painter_SketchAllowed(u32 move)
{
    if (!FlagGet(FLAG_IN_PAINTING) || VarGet(VAR_PAINTING_MODE) != PAINTING_MODE_SKETCH)
        return TRUE;
    return IsQuestMove(move);
}

// Special: the figure's battle (a double battle: TRAINER_GYM_CHALLENGER)
void Painter_SetupFigure(void)
{
    u32 mode = VarGet(VAR_PAINTING_MODE), level = GymChallenge_PlayerAverageLevel(), i, n;
    const u8 *name = COMPOUND_STRING("A FIGURE");
    switch (mode)
    {
    case PAINTING_MODE_FIRST:
    default:
        // (two Pokémon of any type, a little under the player)
        GymChallenge_SetupPaintFigure(0, NUMBER_OF_MON_TYPES, level > 3 ? level - 2 : level, 2, name);
        break;
    case PAINTING_MODE_SKETCH:
    {
        u32 type = VarGet(VAR_PAINTER_SKETCH) >> 8, tier = SketchTier();
        n = tier >= 3 ? 4 : (tier >= 2 ? 3 : 2);
        GymChallenge_SetupPaintFigure(tier >= 2 ? 2 : tier, type, level, n, name);
        // each knows four of the quest's moves: two from its tier, two from any up to it
        for (i = 0; i < n; i++)
        {
            struct TrainerMon *mon = GymChallenge_GetChallengerMon(i);
            u32 k;
            for (k = 0; k < MAX_MON_MOVES; k++)
            {
                u32 t = (k < 2) ? tier : Random() % (tier + 1), j, move, tries = 0;
                do
                {
                    move = sSketchMoves[type][t][Random() % SKETCH_PER_TIER];
                    for (j = 0; j < k && mon->moves[j] != move; j++)
                        ;
                } while (j < k && ++tries < 10);
                mon->moves[k] = (j < k) ? MOVE_NONE : move;
            }
        }
        break;
    }
    case PAINTING_MODE_MEGA:
    {
        u32 pick = Random() % ARRAY_COUNT(sMegaFigures);
        struct TrainerMon *mon;
        GymChallenge_SetupPaintFigure(2, NUMBER_OF_MON_TYPES, level, 3, name);
        mon = GymChallenge_GetChallengerMon(0);
        mon->species = sMegaFigures[pick].species;
        mon->heldItem = sMegaFigures[pick].stone;
        memset(mon->moves, 0, sizeof(mon->moves));
        break;
    }
    }
}

// ---------------------------------------------------------------------------------------------
// Coming back
// ---------------------------------------------------------------------------------------------
// Special (the loft's frame script): VAR_RESULT = PAINTER_RETURN_*; the lent Smeargle goes back,
// a Sketch quest's slot keeps its move or gets the old one back
void Painter_Return(void)
{
    u32 mode = VarGet(VAR_PAINTING_MODE), won = VarGet(VAR_PAINTING_RESULT) == 1;
    u32 slot = FindStoredSmeargle(), fought = VarGet(VAR_PAINTING_RESULT) != 0;

    FlagClear(FLAG_IN_PAINTING);
    VarSet(VAR_PAINTING_RETURN, 0);
    VarSet(VAR_PAINTING_MODE, PAINTING_MODE_NONE);
    VarSet(VAR_PAINTING_RESULT, 0);
    gSpecialVar_Result = PAINTER_RETURN_LEFT;

    switch (mode)
    {
    case PAINTING_MODE_FIRST:
        // (hers back: anything it was holding goes in the bag)
        if (slot < PARTY_SIZE)
        {
            u32 item = GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_HELD_ITEM);
            if (item != ITEM_NONE)
                AddBagItem(item, 1);
            ZeroMonData(&gParties[B_TRAINER_PLAYER][slot]);
            CompactPartySlots();
            CalculatePlayerPartyCount();
        }
        if (fought)
            gSpecialVar_Result = won ? PAINTER_RETURN_FIRST_WON : PAINTER_RETURN_FIRST_LOST;
        break;
    case PAINTING_MODE_SKETCH:
    {
        u32 moveSlot = VarGet(VAR_PAINTER_SKETCH) & 0xFF, old = VarGet(VAR_PAINTER_SKETCH_MOVE), now = MOVE_SKETCH;
        if (slot < PARTY_SIZE && moveSlot < MAX_MON_MOVES)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
            now = GetMonData(mon, MON_DATA_MOVE1 + moveSlot);
            if (!won || now == MOVE_SKETCH || !IsQuestMove(now))
            {
                u32 pp = old != MOVE_NONE ? GetMovePP(old) : 0;
                SetMonData(mon, MON_DATA_MOVE1 + moveSlot, &old);
                SetMonData(mon, MON_DATA_PP1 + moveSlot, &pp);
            }
        }
        if (!fought)
            gSpecialVar_Result = PAINTER_RETURN_LEFT;
        else if (!won)
            gSpecialVar_Result = PAINTER_RETURN_SKETCH_LOST;
        else if (now == MOVE_SKETCH || !IsQuestMove(now))
            gSpecialVar_Result = PAINTER_RETURN_NO_SKETCH;
        else
        {
            gSpecialVar_Result = PAINTER_RETURN_SKETCHED;
            StringCopy(gStringVar1, GetMoveName(now));
            VarSet(VAR_PAINTER_SKETCHES, VarGet(VAR_PAINTER_SKETCHES) + 1);
        }
        VarSet(VAR_PAINTER_SKETCH, 0);
        break;
    }
    case PAINTING_MODE_MEGA:
        if (fought)
            gSpecialVar_Result = won ? PAINTER_RETURN_MEGA_WON : PAINTER_RETURN_MEGA_LOST;
        break;
    }
}

// Special: after the first painting, won: the player's Canvas Smeargle, of their gym's type
// (VAR_RESULT: MON_GIVEN_TO_PARTY / MON_GIVEN_TO_PC / MON_CANT_GIVE)
void Painter_GiveSmeargle(void)
{
    struct Pokemon *mon = AllocZeroed(sizeof(struct Pokemon));
    CreateRandomMon(mon, Painter_SmeargleOfType(VarGet(VAR_PLAYER_TYPE)), CANVAS_SMEARGLE_LEVEL);
    gSpecialVar_Result = GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
    if (gSpecialVar_Result != MON_CANT_GIVE)
        FlagSet(FLAG_PAINTER_GAVE_SMEARGLE);
}

// Special: the figure's battle is over (VAR_RESULT = the battle's outcome)
void Painter_RecordBattle(void)
{
    VarSet(VAR_PAINTING_RESULT, gSpecialVar_Result == B_OUTCOME_WON ? 1 : 2);
}
