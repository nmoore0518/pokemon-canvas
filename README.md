# Pokémon Canvas

A Pokémon FireRed hack where you don't just beat the gyms: you become a Gym Leader.

On day one, Professor Oak tells you the Pokémon League is opening a ninth gym, in Pallet Town, and
you and your rival are the candidates. A Gym Leader uses one type, so you pick yours in his lab and
run the whole League challenge with it. Earn the approval of Kanto's Gym Leaders, become Champion,
and win the gym. Then it's yours to run:

- **Your own gym:** challengers line up at your door. Decorate it (floors, walls, furniture, pools),
  hire practice trainers, add rooms, and keep your win streak going.
- **Master every type:** win enough in a row and you can switch your gym to a new type, each with
  its own legendary.
- **The Gym Leader Convention** in Johto: Gym Leaders, Elite Four and Champions from every
  generation, in a hall for every type.
- **The Exotic Pokémon Safari:** zones for every region from Johto to Paldea, so every Pokémon
  through Generation 9 can be caught.
- **And more:** a new side quest in every Kanto town, the Pokémon Trading Card Game against a
  Grandmaster, an underground fighting pit in Celadon, and a tournament of the world's strongest
  trainers at the very end.

## How to play

1. **Get the patch:** download the newest `.bps` file from this repository's
   [Releases page](../../releases/latest).
2. **Get a FireRed ROM:** you need your own copy of **Pokémon FireRed (USA) version 1.0**. No ROM
   is provided here. The patcher will refuse any other version.
3. **Patch it:** open [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/) in your browser
   (nothing to install), pick your FireRed ROM and the `.bps` file, and click **Apply patch**.
   [Floating IPS](https://github.com/Alcaro/Flips) works too.
4. **Play it** in an emulator such as [mGBA](https://mgba.io/), or on a flash cart (the game is
   32 MB).

**Updating:** patch your clean FireRed ROM again with the new `.bps`, and give the new ROM the same
name as the old one so your save carries over. Updates are meant to keep old saves working.

## Bugs and suggestions

This is a work in progress, and reports help a lot. *(The report form's link goes here.)*
Please include the version number of the patch you're playing.

## Made with AI

The hack's code, scripts, map edits and much of its original art were made with an AI assistant
(Claude, by Anthropic), directed by Nick. Everything borrowed from other projects is listed in
[CREDITS.md](CREDITS.md).

## Credits

Based off RHH's pokeemerald-expansion 1.17.2 https://github.com/rh-hideout/pokeemerald-expansion/
and pret's decompilations. See [CREDITS.md](CREDITS.md) for everyone whose work is in the game.

Pokémon is © Nintendo, Creatures Inc. and GAME FREAK inc. This is a free, unofficial fan project
and is not affiliated with them.

## For developers

The full source is here. [GYM_HACK.md](GYM_HACK.md) explains how the hack is put together, and
[INSTALL.md](INSTALL.md) how to build it. The engine's own README is in
[docs/pokeemerald-expansion-README.md](docs/pokeemerald-expansion-README.md).
