#!/usr/bin/env python3
"""The event Pokémon side quests' lines (Pokémon Canvas) -> data/scripts/event_mons_text.inc

Each text is plain prose: wrap() breaks it into text box lines (a blank line starts a new box).
"RAW:" texts (menu choices) are used as they are.

    python3 tools/gym_hack/event_mons_text.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from build_convention import wrap  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
T = {}

T["Received"] = "{PLAYER} received {STR_VAR_1}!"
T["NoRoom"] = "Oh… There's no room for it in your party or your boxes. Make some space and come back!"

# ---------------------------------------------------------------- 1. the Costume Maker (Celadon)
T["CostumeNotYet"] = """I make costumes for POKéMON. Right now I'm sewing for a very special client…

A GYM LEADER, maybe? Come back when you are one!"""
T["CostumeIntro"] = """Oh! You're a GYM LEADER now? Then you need a star for your shows!

This PIKACHU loves dressing up more than anything. Its costumes are my life's work. Please, take it!"""
T["CostumeExplain"] = """I have five costumes for it, but each one has to suit you first.

ROCK STAR is cool: win 7 in a row in the ACADEMY's BATTLE HALL.

BELLE is beautiful: decorate your GYM with 20 pieces or more.

POP STAR is cute: win the CONVENTION's FAIRY hall.

PH.D. is smart: hear every lecture at the ACADEMY.

LIBRE is tough: win the CONVENTION's FIGHTING hall.

Come back any time and I'll dress it up!"""
T["CostumeAsk"] = "Costumes ready for you: {STR_VAR_1} of 5. Shall I dress up your PIKACHU?"
T["CostumeWhich"] = "Which PIKACHU?"
T["CostumePick"] = "Which costume?"
T["CostumeDone"] = "There! {STR_VAR_3} looks wonderful. A new costume, a new move to show it off!"
T["CostumeNotPikachu"] = "Hmm… My costumes only fit the PIKACHU I gave you."
T["CostumeLater"] = "Come back any time!"
for k, v in {"Cosplay": "NO COSTUME", "RockStar": "ROCK STAR", "Belle": "BELLE", "PopStar": "POP STAR",
             "Phd": "PH.D.", "Libre": "LIBRE"}.items():
    T["Costume" + k] = "RAW:" + v

# ---------------------------------------------------------------- 2. the Cap Collector (Pallet Town)
T["CapBoyIntro"] = """Did you know a TRAINER from PALLET TOWN once went to every region in the world?

I collected a cap from each one, just like his! Then I took them on a trip around KANTO…

…and lost every single one. Seven caps! Waaah!

Hey, you travel a lot, right? If you find my caps, I'll trade you something really special for each one!

Here, put them in this!"""
T["CapBoyCase"] = "My CAP CASE! Look carefully. Caps get snagged in the strangest places."
T["CapBoyThanks"] = """My {STR_VAR_1}! You found it!

Here, this PIKACHU wears one just like it. A trade's a trade!"""
T["CapBoyAll"] = """All seven caps! My collection's back!

You're a real partner. This PIKACHU wears the PARTNER CAP. It's the most special of all!"""
T["CapBoyHint"] = """{STR_VAR_3} caps are still out there.

I think I lost the {STR_VAR_1} somewhere around {STR_VAR_2}…"""
T["CapBoyDone"] = "My caps are all back, thanks to you! Take good care of those PIKACHU!"
T["CapFound"] = "{PLAYER} found the {STR_VAR_1}!"
T["CapInCase"] = "{PLAYER} put the {STR_VAR_1} in the CAP CASE."
T["CapNoCase"] = "There's a little cap stuck here, a PIKACHU's size. Someone must have lost it…"

# ---------------------------------------------------------------- 3. Partners (Oak's quiz)
T["OakPikachuOffer"] = """OAK: Ah, {PLAYER}! A master of ELECTRIC types now!

A PIKACHU here at the lab has been watching you on TV. It's a strong one, and very picky.

It will only go with a trainer who really knows its type. Will you take my quiz? Five questions, all of them right!"""
T["OakEeveeOffer"] = """OAK: Ah, {PLAYER}! A master of NORMAL types now!

An EEVEE here at the lab has been watching you on TV. It's a strong one, and very picky.

It will only go with a trainer who really knows its type. Will you take my quiz? Five questions, all of them right!"""
T["OakQuizLater"] = "OAK: Ho ho! Any time you're ready, then."
T["OakQuizRules"] = "OAK: Here we go!"
T["QuizE1"] = "Which type takes no damage at all from ELECTRIC moves?"
T["QuizE2"] = "ELECTRIC moves are super effective against which type?"
T["QuizE3"] = "Which ability heals a POKéMON hit by an ELECTRIC move?"
T["QuizE4"] = "What does THUNDER WAVE do to its target?"
T["QuizE5"] = "Which of these types resists ELECTRIC moves?"
T["QuizN1"] = "Which type takes no damage at all from NORMAL moves?"
T["QuizN2"] = "NORMAL-type POKéMON are weak to which type?"
T["QuizN3"] = "NORMAL moves are not very effective against which type?"
T["QuizN4"] = "Which ability turns NORMAL moves into FAIRY moves?"
T["QuizN5"] = "Which NORMAL move always strikes first?"
for k, v in {"Ground": "GROUND", "Flying": "FLYING", "Steel": "STEEL", "Grass": "GRASS", "Dragon": "DRAGON",
             "Water": "WATER", "Static": "STATIC", "VoltAbsorb": "VOLT ABSORB", "SwiftSwim": "SWIFT SWIM",
             "Paralysis": "PARALYZES IT", "Burn": "BURNS IT", "Sleep": "PUTS IT TO SLEEP", "Normal": "NORMAL",
             "Rock": "ROCK", "Ghost": "GHOST", "Psychic": "PSYCHIC", "Fighting": "FIGHTING", "Dark": "DARK",
             "Refrigerate": "REFRIGERATE", "Pixilate": "PIXILATE", "Aerilate": "AERILATE",
             "HyperBeam": "HYPER BEAM", "BodySlam": "BODY SLAM", "QuickAttack": "QUICK ATTACK"}.items():
    T["Ans" + k] = "RAW:" + v
T["QuizPassed"] = """OAK: Five out of five! Splendid!

There's no doubt about it. You and this POKéMON belong together."""
T["QuizFailed"] = """OAK: {STR_VAR_1} out of five… So close!

Brush up on your type matchups and try again. I'll be right here!"""
T["OakPartnerAfter"] = "OAK: It's chosen you, {PLAYER}. Raise it with all the care you've given your gym!"

# ---------------------------------------------------------------- 4, 10, 11. the safari statics' hints
T["SealPartyBring"] = "Something is waiting here… It might show itself if {STR_VAR_1} came along."
T["SealPartyCatch"] = "Something is waiting here… It won't come until {STR_VAR_1} has been caught."
T["GlimwoodBoy"] = """I live here in the forest. My DADA raised me, a ZARUDE with a cape made of vines.

He went away a long time ago… They say the forest's guardian, CELEBI, saved him once.

If CELEBI came here, maybe DADA would come back too, once the other ZARUDE are calm."""
T["GlimwoodBoyAfter"] = "DADA's back! You and CELEBI found him. Thank you!"
T["Groundskeeper"] = """I keep the pitch. The old-timers still talk about the DARKEST DAY, when ETERNATUS burst out of the sky right here.

They say it only takes that shape on this pitch. And only if ZACIAN and ZAMAZENTA stand with it, the way they did that day…"""
T["GroundskeeperAfter"] = """You tamed the DARKEST DAY itself!

That power belongs to this pitch, though. Anywhere else, it's just ETERNATUS. Bring it back here to see it again."""

# ---------------------------------------------------------------- 5. the Vivillon collector
T["VivillonIntro"] = """VIVILLON's wings have a different pattern in every corner of the world. I'm sketching every one!

Bring me VIVILLON with patterns I haven't seen. Show me 6, and I'll give you something rare. All 18, and something rarer still!

So far I've seen {STR_VAR_1}."""
T["VivillonNew"] = "Ooh! {STR_VAR_2} new patterns! That's {STR_VAR_1} in my sketchbook now!"
T["VivillonFancy"] = "Six patterns! As promised, a SCATTERBUG whose wings will bloom like a FANCY garden party!"
T["VivillonPokeBall"] = "All 18! My sketchbook's complete! This SCATTERBUG's wings will look just like a POKé BALL!"
T["VivillonWild"] = "Its kind's been seen fluttering around this zone too, now and then. Keep an eye out!"

# ---------------------------------------------------------------- 6. Bond Phenomenon
T["BondNotYet"] = """There's a story of a GRENINJA whose bond with its trainer let it take their shape in battle.

I'm looking for a trainer like that. Someone who could beat every KALOS trainer of renown…"""
T["BondGive"] = """You beat every KALOS trainer in that GAUNTLET! I saw it all.

This FROAKIE was born on the beach where that GRENINJA lived. Raise it, and bond with it!"""
T["BondAfter"] = "When your bond is strong enough, your GRENINJA will show you something amazing."

# ---------------------------------------------------------------- 7. the Eternal Flower (AZ)
T["AzStranger"] = "A very tall man is staring out over the badlands. He doesn't seem to notice you."
T["AzIntro"] = """AZ: …You. The one who beat me.

I am looking for a flower. A FLOETTE. I lost her three thousand years ago."""
T["AzStory"] = """AZ: That FLOETTE of yours… She looks so much like mine.

I was a king. When my FLOETTE was taken by war, I built a machine to bring her back, and then I turned it on the world in my grief.

She left me. She could not forgive what I had done. I have walked ever since.

The legends of this land, XERNEAS, YVELTAL, ZYGARDE… If a trainer were trusted by all three, perhaps she would trust them too."""
T["AzTrio"] = "AZ: Life, destruction, and order… {STR_VAR_1} has not yet answered to you."
T["AzReturn"] = """AZ: …Do you hear that? Petals, on the wind.

…FLOETTE! You came back!

…She does not come back to me. She is choosing you. She was always wiser than her king.

Take her. Let her see a world at peace."""
T["AzAfter"] = "AZ: She has found someone worth trusting. That is enough for me."

# ---------------------------------------------------------------- 8. Own Tempo (Tala)
T["TalaIntro"] = """TALA: My ROCKRUFF had a pup that never howls with the others. It does everything at its own pace.

It wants a trainer whose pack can outrun mine. Want to try?"""
T["TalaWon"] = "TALA: Ha! Your pack's the real deal. This little one's yours. It'll never howl on cue, but it'll never quit, either!"
T["TalaAfter"] = "TALA: Raise that pup well! It'll grow into something no other LYCANROC can be."
T["TalaLater"] = "TALA: Whenever you're ready."
T["TalaLost"] = "TALA: Not fast enough! Come back when your pack's stronger."

# ---------------------------------------------------------------- 9. the Original Color (the artisan)
T["ArtisanNotYet"] = """I restore antiques. My dream is to see a POKéMON that was built five hundred years ago… a MAGEARNA.

If you ever meet one, come and tell me!"""
T["ArtisanPots"] = """A real MAGEARNA! The first one was painted red and gold. I'd love to paint one just like it.

But I lost my four antique paint pots somewhere in this zone… {STR_VAR_2} are still missing."""
T["ArtisanPaint"] = """My paint pots! All four! Now, just like the first one…

There. The ORIGINAL COLOR, five hundred years later. It's yours. Treasure it!"""
T["ArtisanAfter"] = "Red and gold, just like the old drawings. It's the best work I've ever done."
T["PotFound"] = "{PLAYER} found an antique paint pot!"
T["PotsLeft"] = "{STR_VAR_2} more to find."
T["PotsAll"] = "That's all four! Time to take them back to the artisan."


def main():
    out = ["@ Generated by tools/gym_hack/event_mons_text.py: do not edit\n\n"]
    for k, v in T.items():
        body = v[4:] if v.startswith("RAW:") else wrap(v)
        out.append(f"EventMons_Text_{k}::\n\t.string \"{body}$\"\n\n")
    (ROOT / "data/scripts/event_mons_text.inc").write_text("".join(out))


if __name__ == "__main__":
    main()
