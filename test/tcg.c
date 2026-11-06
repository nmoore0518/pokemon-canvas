// Tests for Pokémon Canvas's Trading Card Game duel (src/tcg/): the rules, run headless (no
// screens), against the Game Boy Color game's own numbers.
// Run with: make check BUILD=firered TESTS="GymHack" -j16

#include "global.h"
#include "test/test.h"
#include "tcg_anims.h"
#include "tcg.h"
#include "tcg_ai.h"
#include "constants/tcg_text.h"
#include "tcg_effect_functions.h"

// A duel with one Pokémon each in the arena: the player's `attacker` (deck index 0) against the
// opponent's `defender` (deck index 0), the player's turn, six prizes each (deck indexes 54-59)
static void SetUpDuel(u32 attacker, u32 defender)
{
    u32 d, i;
    memset(gTcg, 0, sizeof(*gTcg));
    gTcg->headless = TRUE;
    for (d = 0; d < 2; d++)
    {
        u8 *v = gTcg->vars[d];
        for (i = 0; i < TCG_DECK_SIZE; i++)
        {
            gTcg->deck[d][i] = TCG_CARD_GRASS_ENERGY;
            v[TCG_DUELVARS_DECK_CARDS + i] = i;
        }
        gTcg->deck[d][0] = d == 0 ? attacker : defender;
        v[TCG_DUELVARS_ARENA_CARD] = 0;
        for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON - 1; i++)
            v[TCG_DUELVARS_BENCH + i] = 0xFF;
        v[TCG_DUELVARS_CARD_LOCATIONS + 0] = TCG_CARD_LOCATION_ARENA;
        v[TCG_DUELVARS_ARENA_CARD_HP] = Tcg_Card(gTcg->deck[d][0])->hp;
        v[TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA] = 1;
        v[TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK] = 1;
        v[TCG_DUELVARS_DUELIST_TYPE] = d == 0 ? TCG_DUELIST_TYPE_PLAYER : TCG_DUELIST_TYPE_AI_OPP;
        v[TCG_DUELVARS_PRIZES] = 0x3F;
        for (i = 0; i < 6; i++)
        {
            v[TCG_DUELVARS_PRIZE_CARDS + i] = 54 + i;
            v[TCG_DUELVARS_CARD_LOCATIONS + 54 + i] = TCG_CARD_LOCATION_PRIZE;
        }
    }
    gTcg->whoseTurn = gTcg->whoseTurnReal = TCG_PLAYER;
}

// the turn holder's deck index `i` becomes `card`, at `location`
static void PutCard(u32 i, u32 card, u32 location)
{
    gTcg->deck[gTcg->whoseTurn][i] = card;
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) = location;
}

// the attacker's attack's damage to the defender, as the duel works it out
static u16 Damage(u32 attack)
{
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), attack);
    Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars();
    return Tcg_ApplyDamageModifiers_DamageToTarget();
}

TEST("GymHack: TCG damage: weakness doubles it, resistance takes off 30 (not below 0)")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_BULBASAUR);   // (Bulbasaur: weak to Fire)
    EXPECT_EQ(Damage(0), 20);                               // Scratch: 10
    EXPECT_EQ(gTcg->damageEffectiveness, 1 << TCG_WEAKNESS);
    EXPECT_EQ(Damage(1), 60);                               // Ember: 30
    SetUpDuel(TCG_CARD_MACHOP, TCG_CARD_PIDGEY);          // (Pidgey: resists Fighting)
    EXPECT_EQ(Damage(0), 0);                                // Low Kick: 20 - 30
    EXPECT_EQ(gTcg->damageEffectiveness, 1 << TCG_RESISTANCE);
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    EXPECT_EQ(Damage(1), 30);
    EXPECT_EQ(gTcg->damageEffectiveness, 0);
}

TEST("GymHack: TCG damage: PlusPower +10 each (the attacker's), Defender -20 each (the defender's)")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    PutCard(1, TCG_CARD_PLUSPOWER, TCG_CARD_LOCATION_ARENA);
    PutCard(2, TCG_CARD_PLUSPOWER, TCG_CARD_LOCATION_ARENA);
    EXPECT_EQ(Damage(1), 50);
    Tcg_SwapTurn();
    PutCard(1, TCG_CARD_DEFENDER, TCG_CARD_LOCATION_ARENA);
    Tcg_SwapTurn();
    EXPECT_EQ(Damage(1), 30);
    EXPECT_EQ(Damage(0), 10);
    Tcg_SwapTurn();
    PutCard(2, TCG_CARD_DEFENDER, TCG_CARD_LOCATION_ARENA);
    Tcg_SwapTurn();
    EXPECT_EQ(Damage(0), 0);                                // (10 + 20 - 40: not below 0)
}

TEST("GymHack: TCG damage: Mr. Mime's Invisible Wall stops 30 or more, unless it's asleep")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MR_MIME);
    EXPECT_EQ(Damage(0), 10);
    EXPECT_EQ(Damage(1), 0);
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_STATUS] = TCG_ASLEEP;
    EXPECT_EQ(Damage(1), 30);
}

TEST("GymHack: TCG damage: the game's halving (\"sla d\") and Pounce/Growl/Withdraw")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_SUBSTATUS1] = TCG_SUBSTATUS1_HALVE_DAMAGE;
    EXPECT_EQ(Damage(1), 10);                               // 30 / 2 = 15, down to 10
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_SUBSTATUS1] = TCG_SUBSTATUS1_NO_DAMAGE_WITHDRAW;
    EXPECT_EQ(Damage(1), 0);
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_SUBSTATUS1] = 0;
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_SUBSTATUS2] = TCG_SUBSTATUS2_GROWL;
    EXPECT_EQ(Damage(1), 20);
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_SUBSTATUS2] = 0;
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_SUBSTATUS3] = 1 << TCG_SUBSTATUS3_THIS_TURN_DOUBLE_DAMAGE_F;
    EXPECT_EQ(Damage(1), 60);
}

TEST("GymHack: TCG SubtractHP: down to 0, not below")
{
    u8 hp = 40;
    EXPECT(Tcg_SubtractHP(&hp, 30));
    EXPECT_EQ(hp, 10);
    EXPECT(!Tcg_SubtractHP(&hp, 30));
    EXPECT_EQ(hp, 0);
}

TEST("GymHack: TCG attached energy: each card counts 1, Double Colorless 2")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    PutCard(1, TCG_CARD_FIRE_ENERGY, TCG_CARD_LOCATION_ARENA);
    PutCard(2, TCG_CARD_DOUBLE_COLORLESS_ENERGY, TCG_CARD_LOCATION_ARENA);
    PutCard(3, TCG_CARD_FIRE_ENERGY, TCG_CARD_LOCATION_HAND);
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    EXPECT_EQ(gTcg->attachedEnergies[TCG_FIRE], 1);
    EXPECT_EQ(gTcg->attachedEnergies[TCG_COLORLESS], 2);
    EXPECT_EQ(gTcg->totalAttachedEnergies, 3);
}

TEST("GymHack: TCG attack: the defender's HP goes down; knocked out with no bench, the attacker wins")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_BULBASAUR);
    gTcg->tempCardIndexFf9f = 0;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 0);
    EXPECT(!Tcg_UseAttackOrPokemonPower());
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_HP], 20);
    EXPECT_EQ(gTcg->duelFinished, TCG_DUEL_NOT_FINISHED);
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE], 20);
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 0);
    EXPECT(!Tcg_UseAttackOrPokemonPower());
    EXPECT_EQ(gTcg->duelFinished, TCG_TURN_PLAYER_WON);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_PRIZES], 0x3E);        // (one prize taken)
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_CARD_LOCATIONS + 0], TCG_CARD_LOCATION_DISCARD_PILE);
}

TEST("GymHack: TCG attack: a knocked-out Pokémon is replaced from the bench, and a prize taken")
{
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_BULBASAUR);
    Tcg_SwapTurn();
    PutCard(1, TCG_CARD_MACHOP, TCG_CARD_LOCATION_BENCH_1);
    *Tcg_TurnVar(TCG_DUELVARS_BENCH) = 1;
    *Tcg_TurnVar(TCG_DUELVARS_BENCH1_CARD_HP) = 50;
    *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) = 2;
    Tcg_SwapTurn();
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 1);        // Ember (30 x 2): no Fire Energy to discard
    EXPECT(Tcg_UseAttackOrPokemonPower());
    EXPECT_EQ(gTcg->hlText, TCG_TEXT_NotEnoughFireEnergy);
    PutCard(1, TCG_CARD_FIRE_ENERGY, TCG_CARD_LOCATION_ARENA);
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 1);
    EXPECT(!Tcg_UseAttackOrPokemonPower());
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_CARD_LOCATIONS + 1], TCG_CARD_LOCATION_DISCARD_PILE);
    EXPECT_EQ(gTcg->duelFinished, TCG_DUEL_NOT_FINISHED);
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD], 1);
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_HP], 50);
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA], 1);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_PRIZES], 0x3E);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_CARD_LOCATIONS + 54], TCG_CARD_LOCATION_HAND);
}

TEST("GymHack: TCG status queue: an attack's condition lands, unless the defender's protected")
{
    static const u8 sEntry[] = {TCG_QUEUE_DUELIST(TCG_OPPONENT), TCG_CNF_SLP_PRZ, TCG_POISONED};
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    memcpy(gTcg->statusConditionQueue, sEntry, 3);
    gTcg->statusConditionQueueIndex = 3;
    EXPECT(Tcg_ApplyStatusConditionQueue());
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_STATUS], TCG_POISONED);
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_STATUS] = 0;
    gTcg->noDamageOrEffect = TCG_NO_DAMAGE_OR_EFFECT_AGILITY;
    EXPECT(!Tcg_ApplyStatusConditionQueue());
    EXPECT_EQ(gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_ARENA_CARD_STATUS], 0);
    EXPECT_EQ(gTcg->hlText, TCG_TEXT_NoDamageOrEffectDueToAgility);
}

// a whole duel, the AI on both sides (src/tcg/ai.c), headless
static u32 RunAiDuel(u32 deck)
{
    memset(gTcg, 0, sizeof(*gTcg));
    gTcg->headless = TRUE;
    Tcg_SetDecks(gTcgDecks[deck], gTcgDecks[TCG_DECK_ROD]);
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_DUELIST_TYPE] = TCG_DUELIST_TYPE_AI_OPP;
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_DUELIST_TYPE] = TCG_DUELIST_TYPE_AI_OPP;
    gTcg->duelInitialPrizes = 6;
    TcgAi_SetDeck(TCG_PLAYER, 0);       // (the player's deck: the general decks' AI)
    TcgAi_SetDeck(TCG_OPPONENT, TCG_DECKID_LEGENDARY_DRAGONITE);
    return Tcg_StartDuel();
}

// every card of the duelist's 60 is in exactly one place
static bool32 CardsAccountedFor(u32 duelist)
{
    u8 *v = gTcg->vars[duelist];
    u32 i, inDeck = TCG_DECK_SIZE - v[TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK], n = 0;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (v[TCG_DUELVARS_CARD_LOCATIONS + i] == TCG_CARD_LOCATION_DECK)
            n++;
    return n == inDeck;
}

TEST("GymHack: TCG duel: each of the player's decks plays a whole duel against Rod's")
{
    u32 deck;
    PARAMETRIZE { deck = TCG_DECK_GRASS; }
    PARAMETRIZE { deck = TCG_DECK_FIRE; }
    PARAMETRIZE { deck = TCG_DECK_WATER; }
    PARAMETRIZE { deck = TCG_DECK_LIGHTNING; }
    PARAMETRIZE { deck = TCG_DECK_PSYCHIC; }
    PARAMETRIZE { deck = TCG_DECK_FIGHTING; }
    PARAMETRIZE { deck = TCG_DECK_COLORLESS; }
    EXPECT_LT(RunAiDuel(deck), 2);              // (TCG_DUEL_WIN or TCG_DUEL_LOSS)
    EXPECT_NE(gTcg->duelFinished, TCG_DUEL_NOT_FINISHED);
    EXPECT_GT(gTcg->duelTurns, 2);
    EXPECT(CardsAccountedFor(TCG_PLAYER));
    EXPECT(CardsAccountedFor(TCG_OPPONENT));
}

TEST("GymHack: TCG effects: Charizard's Energy Burn makes all its Energy Fire")
{
    SetUpDuel(TCG_CARD_CHARIZARD, TCG_CARD_MACHOP);
    PutCard(1, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    PutCard(2, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    PutCard(3, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    PutCard(4, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    EXPECT(Tcg_CheckIfEnoughEnergiesForGivenAttack(0, 1));     // Fire Spin: 4 Fire
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    Tcg_HandleEnergyBurn();
    EXPECT(!Tcg_CheckIfEnoughEnergiesForGivenAttack(0, 1));
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_STATUS] = TCG_ASLEEP;     // (asleep: no Pokémon Power)
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    Tcg_HandleEnergyBurn();
    EXPECT(Tcg_CheckIfEnoughEnergiesForGivenAttack(0, 1));
}

TEST("GymHack: TCG effects: Professor Oak discards the hand and draws 7")
{
    u32 i;
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    for (i = 1; i <= 4; i++)
    {
        u8 card;
        Tcg_DrawCardFromDeck(&card);
        Tcg_AddCardToHand(card);
    }
    gTcg->deck[TCG_PLAYER][1] = TCG_CARD_PROFESSOR_OAK;
    gTcg->tempCardIndex = 1;
    EXPECT(!Tcg_PlayTrainerCard());
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND], 7);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE], 4);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_CARD_LOCATIONS + 1], TCG_CARD_LOCATION_DISCARD_PILE);
}

TEST("GymHack: TCG effects: Potion heals up to 20; Switch swaps in the bench Pokémon")
{
    u8 card;
    SetUpDuel(TCG_CARD_CHARMANDER, TCG_CARD_MACHOP);
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_HP] = 20;
    Tcg_DrawCardFromDeck(&card);
    Tcg_AddCardToHand(card);
    gTcg->deck[TCG_PLAYER][card] = TCG_CARD_POTION;
    gTcg->tempCardIndex = card;
    EXPECT(!Tcg_PlayTrainerCard());
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD_HP], 40);
    // Switch: with no bench, it can't be played
    Tcg_DrawCardFromDeck(&card);
    Tcg_AddCardToHand(card);
    gTcg->deck[TCG_PLAYER][card] = TCG_CARD_SWITCH;
    gTcg->tempCardIndex = card;
    EXPECT(Tcg_PlayTrainerCard());
    EXPECT_EQ(gTcg->hlText, TCG_TEXT_EffectNoPokemonOnTheBench);
    PutCard(5, TCG_CARD_MACHOP, TCG_CARD_LOCATION_BENCH_1);
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_BENCH] = 5;
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_BENCH1_CARD_HP] = 50;
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA] = 2;
    gTcg->tempCardIndex = card;
    EXPECT(!Tcg_PlayTrainerCard());
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_ARENA_CARD], 5);
    EXPECT_EQ(gTcg->vars[TCG_PLAYER][TCG_DUELVARS_BENCH], 0);
}

TEST("GymHack: TCG effects: Lapras's Water Gun: +10 for each extra Water Energy, up to +20")
{
    u32 i;
    SetUpDuel(TCG_CARD_LAPRAS, TCG_CARD_MACHOP);
    for (i = 1; i <= 2; i++)
        PutCard(i, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 0);
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    Tcg_LaprasWaterGunEffect();
    EXPECT_EQ(gTcg->damage, Tcg_Card(TCG_CARD_LAPRAS)->attacks[0].damage + 10);
    for (i = 3; i <= 5; i++)
        PutCard(i, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_ARENA);
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(0, 0);
    Tcg_LaprasWaterGunEffect();
    EXPECT_EQ(gTcg->damage, Tcg_Card(TCG_CARD_LAPRAS)->attacks[0].damage + 20);
}

// the opponent's turn, Grandmaster Rod's AI
static void RodsTurn(void)
{
    gTcg->whoseTurn = gTcg->whoseTurnReal = TCG_OPPONENT;
    TcgAi_SetDeck(TCG_OPPONENT, TCG_DECKID_LEGENDARY_DRAGONITE);
}

TEST("GymHack: TCG AI: Rod's opening hand has 2 Basics and 2 Energy, and 4 of each with the 6 after the Prizes")
{
    u32 i, basics = 0, energy = 0;
    memset(gTcg, 0, sizeof(*gTcg));
    gTcg->headless = TRUE;
    Tcg_SetDecks(gTcgDecks[TCG_DECK_FIRE], gTcgDecks[TCG_DECK_ROD]);
    RodsTurn();
    Tcg_ShuffleDeckAndDrawSevenCards();
    TcgAi_SetUpBossStartingHandAndDeck();
    EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND), 7);
    EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK), 7);
    EXPECT(CardsAccountedFor(TCG_OPPONENT));
    for (i = 0; i < 7; i++)
    {
        const struct TcgCard *c = Tcg_Card(Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_HAND + i)));
        if (c->type < TCG_TYPE_ENERGY)
            basics += (c->stage == TCG_BASIC);
        else if (c->type != TCG_TYPE_TRAINER)
            energy++;
    }
    EXPECT_GE(basics, 2);
    EXPECT_GE(energy, 2);
    for (i = 13; i < 19; i++)
    {
        const struct TcgCard *c = Tcg_Card(Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + i)));
        if (c->type < TCG_TYPE_ENERGY)
            basics += (c->stage == TCG_BASIC);
        else if (c->type != TCG_TYPE_TRAINER)
            energy++;
    }
    EXPECT_GE(basics, 4);
    EXPECT_GE(energy, 4);
}

TEST("GymHack: TCG AI: Rod plays Professor Oak with a small hand and no Energy, not with a big one")
{
    u32 i, hand;
    PARAMETRIZE { hand = 1; }
    PARAMETRIZE { hand = 9; }
    SetUpDuel(TCG_CARD_MACHOP, TCG_CARD_KANGASKHAN);
    RodsTurn();
    for (i = 1; i <= hand; i++)
    {
        u8 card;
        Tcg_DrawCardFromDeck(&card);
        Tcg_AddCardToHand(card);
        gTcg->deck[TCG_OPPONENT][card] = (i == 1) ? TCG_CARD_PROFESSOR_OAK : (i & 1) ? TCG_CARD_GRASS_ENERGY : TCG_CARD_POTION;
    }
    TcgAi_InitAIDuelVars();
    TcgAi_InitAITurnVars();
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_15);
    if (hand == 1)
    {
        // 30 + 50 (under 4 cards) + 40 (no Energy): played
        EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + 1), TCG_CARD_LOCATION_DISCARD_PILE);
        EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND), 7);
        EXPECT(gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_PROFESSOR_OAK);
    }
    else
    {
        // 30 - 30 (9 cards) + 10 (an Energy... the game's bug: the first non-Pokémon): kept
        EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + 1), TCG_CARD_LOCATION_HAND);
        EXPECT_EQ(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND), 9);
    }
}

TEST("GymHack: TCG AI: Rod attacks with what he can (Kangaskhan's Fetch, with 1 Energy)")
{
    SetUpDuel(TCG_CARD_MACHOP, TCG_CARD_KANGASKHAN);
    RodsTurn();
    PutCard(1, TCG_CARD_FIRE_ENERGY, TCG_CARD_LOCATION_ARENA);
    gTcg->deck[TCG_OPPONENT][1] = TCG_CARD_FIRE_ENERGY;
    TcgAi_InitAIDuelVars();
    TcgAi_InitAITurnVars();
    EXPECT(TcgAi_ProcessAndTryToUseAttack());
    EXPECT_EQ(gTcg->selectedAttack, 0);
}

TEST("GymHack: TCG duel: every damage digit (0-9) has its animation")
{
    u32 i;
    for (i = TCG_SPRITE_ANIM_79; i <= TCG_SPRITE_ANIM_79 + 9; i++)
        EXPECT(gTcgSpriteAnims[i].frameData != NULL);
}

TEST("GymHack: TCG Blastoise's Rain Dance: any number of Water Energy onto Water Pokemon")
{
    SetUpDuel(TCG_CARD_BLASTOISE, TCG_CARD_BULBASAUR);
    EXPECT(Tcg_IsRainDanceActive());
    PutCard(1, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_HAND);
    gTcg->tempCardIndex = 1;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    EXPECT(Tcg_CheckRainDanceScenario());
    // (from the bench, onto a benched Water Pokémon)
    SetUpDuel(TCG_CARD_SQUIRTLE, TCG_CARD_BULBASAUR);
    EXPECT(!Tcg_IsRainDanceActive());
    PutCard(2, TCG_CARD_BLASTOISE, TCG_CARD_LOCATION_BENCH_1);
    *Tcg_TurnVar(TCG_DUELVARS_BENCH) = 2;
    *Tcg_TurnVar(TCG_DUELVARS_BENCH1_CARD_HP) = 100;
    *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) = 2;
    EXPECT(Tcg_IsRainDanceActive());
    PutCard(1, TCG_CARD_WATER_ENERGY, TCG_CARD_LOCATION_HAND);
    gTcg->tempCardIndex = 1;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    EXPECT(Tcg_CheckRainDanceScenario());
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_BENCH_1;
    EXPECT(Tcg_CheckRainDanceScenario());
}
