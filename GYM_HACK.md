# Gym Trainer hack

A FireRed hack built on pokeemerald-expansion (FireRed mode). The player is training to become a
Gym Leader: they pick one type at the start and build their team, and later their own gym, around it.

## Building and testing

```
make firered -j$(nproc)                                   # -> pokefirered.gba
make check BUILD=firered TESTS="GymHack" -j$(nproc)       # run this hack's automated tests
```

Debug menu: hold R and press Start in the overworld (on in non-release builds).

## Design decisions

- **Modern typings** (Fairy included). 17 selectable types; Flying is excluded because no Gen 1-3
  Pokémon has Flying as its primary type.
- **Primary type** means `gSpeciesInfo[species].types[0]`.
- The player's type is stored in `VAR_PLAYER_TYPE` (0x40F7) as a `TYPE_*` constant; 0 = not chosen yet.
- **Rival** still takes one of the three Kanto starters, because his later teams depend on it.
  Which one depends on the player's type (table in `src/gym_hack.c`).

## Code map

| File | What |
|---|---|
| `src/gym_hack.c`, `include/gym_hack.h` | Hack logic: starter table, specials |
| `test/gym_hack.c` | Automated tests |
| `data/maps/PalletTown_ProfessorOaksLab_Frlg/scripts.inc` | Oak's lab: type menu, rival pick |
| `include/constants/vars.h` | `VAR_PLAYER_TYPE` |

## Roadmap

1. [x] Source build, debug menu
2. Type identity
   - [x] 17-starter type selection in Oak's lab
   - [ ] Type + no-duplicates check for gym, rival and Elite Four battles
3. [ ] Evolution toggle per Pokémon (no held item); HMs as key items
4. [ ] All 386 Pokémon in the wild; type finder (DexNav-based)
5. [ ] Gym builder and challengers
