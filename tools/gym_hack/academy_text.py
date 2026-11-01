#!/usr/bin/env python3
"""The Leader Academy's lines (Gym Trainer hack) -> data/scripts/leader_academy_text.inc

Each text is plain prose: wrap() breaks it into text box lines (a blank line starts a new box).

    python3 tools/gym_hack/academy_text.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from build_convention import wrap  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

T = {}

# ---------------------------------------------------------------- the invitation (Route 11)
T["InviteCall"] = "{STR_VAR_1}: {PLAYER}! Hold on a second!"
T["Escort1"] = """{STR_VAR_1}: Whew, there you are. Three BADGES already! I've been watching.

You've done well enough that I put your name forward… and the LEADER ACADEMY said yes.

It's a school for GYM LEADER candidates. Every region sends its best hopefuls there.

It's right through these trees. Come on, I'll walk you in."""
T["Guard1"] = """GUARD: Halt! The ACADEMY is for candidates only… Oh! {STR_VAR_1}!

So this is the one you vouched for. The HEADMASTER is expecting you both. Go on through!"""
T["GateGuard"] = """The LEADER ACADEMY is beyond these trees.

Only GYM LEADER candidates who've been invited may enter. Sorry, rules are rules!"""

# ---------------------------------------------------------------- the welcome (the campus gate)
T["Welcome1"] = """WISTERIA: Ah, {STR_VAR_1}. And you must be {PLAYER}.

Welcome to the LEADER ACADEMY. I am WISTERIA, its HEADMASTER.

Our candidates come from every region, all chasing the same dream you are: a GYM of their own."""
T["Welcome2"] = """The main hall, just ahead, is our library. Its lecture halls are where our INSTRUCTORS teach: each has a lesson in tournament strategy… and a demonstration battle.

In the BATTLE HALL, you can battle with rental teams. Every win pays BATTLE POINTS, or BP."""
T["Welcome3"] = """Spend BP at the EXCHANGE on held items, MINTS, and the like.

But know this: BP is never the only way! The EXCHANGE and the SUPPLY shop take ordinary money for every item, too."""
T["Welcome4"] = """The other candidates love a practice battle. Challenge them as often as you like. They pay BP as well.

And there's a POKéMON CENTER, of course. Make yourself at home, {PLAYER}."""
T["MentorBye"] = """{STR_VAR_1}: That's my cue. Learn everything you can here, {PLAYER}.

The next time we battle, I want to be surprised!"""

# ---------------------------------------------------------------- signs
T["HallSign"] = "LEADER ACADEMY · MAIN HALL\nThe library, and lecture halls A to D."
T["CenterSign"] = "POKéMON CENTER\nCandidates heal free of charge."
T["SupplySign"] = "ACADEMY SUPPLY\nBOTTLE CAPS, ABILITY items, MINTS, vitamins: all for ¥."
T["BattleSign"] = "BATTLE HALL\nRental battles. Win streaks pay BP."
T["ExchangeSign"] = "THE EXCHANGE\nCompetitive held items. Pay in BP… or in ¥! Both counters sell the same items."
T["GateSign"] = "LEADER ACADEMY\nEst. by the GYM LEADER CONVENTION. Candidates of every region welcome."

# ---------------------------------------------------------------- the headmaster (in the hall)
T["HeadmasterHello"] = "WISTERIA: Ah, {PLAYER}. You have {STR_VAR_1} BP. What can I tell you about?"
T["HeadmasterBP"] = """BP are BATTLE POINTS. You earn them in practice: a win against another candidate, an INSTRUCTOR's demonstration, or a streak in the BATTLE HALL.

The EXCHANGE trades BP for held items. But every item there can also be bought with money, at the other counter."""
T["HeadmasterHall"] = """In the BATTLE HALL, you borrow three POKéMON from a list of six. Your own team waits safely with the clerk.

Win and you keep going, and the BP grows with your streak. Lose, and you simply start again."""
T["HeadmasterCandidates"] = """Candidates come here from JOHTO, HOENN, SINNOH, UNOVA, KALOS, ALOLA, GALAR and PALDEA.

Some are just starting out. Some could take a GYM tomorrow. All of them will battle you, as often as you like."""
T["HeadmasterTeachers"] = """Each of my INSTRUCTORS is named for a flower, as I am. It's a tradition here.

Their lecture halls are through the doors behind me. In A, LARKSPUR teaches speed control and CAMELLIA setup. In B, FOXGLOVE spread moves and VERBENA positioning.

In C, MARIGOLD weather and HYACINTH targeting. In D, YARROW training and TANSY team building.

All of it for double battles, mind you. That's where GYM battles are going."""
T["HeadmasterBye"] = "Off you go, then. A GYM LEADER never stops learning."

# ---------------------------------------------------------------- lectures (doubles: the way GYM battles are going)
LECTURES = {
    "Larkspur": ["LARKSPUR: Speed control. Welcome!",
        """In a double battle, four POKéMON move every turn. Whoever controls the turn order controls the game.

TAILWIND doubles your side's speed for four turns. ICY WIND and ELECTROWEB hit both foes and slow them down.""",
        """TRICK ROOM turns it all upside down: for five turns, the slowest move first. A slow, strong team loves it.

And FAKE OUT! It strikes first and makes the target flinch, but only on the turn it comes in.""",
        """Priority still matters: EXTREME SPEED, BULLET PUNCH and SUCKER PUNCH. But beware PSYCHIC TERRAIN and QUICK GUARD.

Make the turn order yours. That's my whole lesson!"""],
    "Camellia": ["CAMELLIA: Setting up, with a partner. Shall we begin?",
        """Setting up is harder with two attackers aimed at you. So bring a friend.

FOLLOW ME and RAGE POWDER pull every single-target attack onto the partner. That's a free turn for your sweeper.""",
        """FAKE OUT stops one foe, PROTECT dodges the other, and your DRAGON DANCE goes off untouched.

INTIMIDATE on the partner softens physical attacks on both of you.""",
        """Against setup? Hit the sweeper with both POKéMON at once, or strike past the redirection with a spread move.

HAZE and CLEAR SMOG wipe the boosts away. A setup turn is only free if you make it free!"""],
    "Foxglove": ["FOXGLOVE: Spread moves. My favorite subject.",
        """EARTHQUAKE, ROCK SLIDE, HEAT WAVE, DAZZLING GLEAM, MUDDY WATER, HYPER VOICE: they hit both foes at once.

A spread move does three quarters of its damage to each target. Two hits for the price of one!""",
        """Careful: EARTHQUAKE and SURF hit your partner too. Pair them with a FLYING type, a LEVITATE user, or a POKéMON that absorbs WATER.

Or have the partner PROTECT on the turn you fire it.""",
        """WIDE GUARD blocks spread moves for your whole side. And ROCK SLIDE's flinches add up fast.

In doubles, the best damage is damage everywhere!"""],
    "Verbena": ["VERBENA: Positioning and switching. Let's go!",
        """In doubles, what's on the field next to what matters as much as the POKéMON themselves.

PARTING SHOT, U-TURN and VOLT SWITCH bring in the right partner. FAKE OUT can be used again each time a POKéMON comes back in.""",
        """INTIMIDATE lowers both foes' Attack every time its POKéMON comes in. Switch it out and back to do it again.

REGENERATOR heals on every switch. A POKéMON in the back is a POKéMON resting.""",
        """PROTECT buys time for a switch on the other side. And watch for a slot the foe is about to double up on.

Move your pieces before they move you!"""],
    "Marigold": ["MARIGOLD: Weather and terrain. Bring an umbrella!",
        """Weather is even stronger in doubles: one setter powers up both its partners.

DRIZZLE, DROUGHT, SAND STREAM and SNOW WARNING last five turns; eight with the right rock.""",
        """SWIFT SWIM, CHLOROPHYLL, SAND RUSH and SLUSH RUSH double a POKéMON's speed in their weather. Rain makes THUNDER and HURRICANE never miss.

The setter's partner should be the abuser. Lead with them together!""",
        """Terrains last five turns too. PSYCHIC TERRAIN blocks FAKE OUT and other priority. GRASSY TERRAIN weakens EARTHQUAKE.

Remember: the last weather set wins. Keep the setter safe to bring it back!"""],
    "Hyacinth": ["HYACINTH: Targeting and PROTECT. Sit down, please.",
        """Two of you, two of them. The question every turn is: who do we hit?

Doubling up on one foe takes it out fast. But if it uses PROTECT, both your attacks are wasted.""",
        """PROTECT is the most important move in doubles. It scouts, it stalls weather and TRICK ROOM, and it baits double attacks.

But it can fail if used twice in a row. Count them!""",
        """Watch which slot they protect, who they target, which POKéMON they keep in the back.

Spread your damage when you can't read them. Focus it when you can."""],
    "Yarrow": ["YARROW: Training. The numbers behind the battles.",
        """Every POKéMON has 510 EFFORT points to spend: 252 at most in one stat. Put them where the POKéMON needs them.

In doubles, bulk matters more: two attackers come your way. Many trainers put more into HP than in singles.""",
        """A nature raises one stat ten percent and lowers another. MINTS, or my CILANTRO, change the nature's effect.

For a TRICK ROOM team, lower its Speed on purpose: a Speed-lowering nature, and Speed IVs at zero.""",
        """Then the item. SITRUS BERRY for staying power, LIFE ORB for raw damage, FOCUS SASH to survive a double attack, ASSAULT VEST for special bulk.

The SUPPLY shop and the EXCHANGE sell all of it."""],
    "Tansy": ["TANSY: Team building! Six POKéMON, three plans.",
        """In doubles you lead with two. Build pairs that work together: FAKE OUT with a sweeper, a weather setter with its abuser.

Give your team a second plan for when the first lead struggles.""",
        """Check your weaknesses: if four of your six fear ROCK SLIDE, that's a problem.

Bring PROTECT on almost everyone, and something to deal with TRICK ROOM and TAILWIND.""",
        """That PC next to me? I made a TEAM BUILDER for it myself. Pick six, choose their moves, and it shows you your weaknesses and coverage. It keeps up to eight teams.

Use it whenever you like: you don't have to ask me!"""],
}

T["TeacherMenu"] = "What shall we do?"
T["DemoIntro"] = """{STR_VAR_1}: A demonstration! A double battle, and we both use rentals, three each.

My team shows off the lesson. Choose yours wisely!"""
T["DemoWon"] = "{STR_VAR_1}: Wonderful! Here, you've earned it."
T["DemoLost"] = "{STR_VAR_1}: Ha! Think about the lesson and try again."

# ---------------------------------------------------------------- rentals
T["RentalPick"] = "Choose a rental POKéMON. ({STR_VAR_1} of 3)"
T["RentalInfo"] = "RAW:{STR_VAR_1}\\p{STR_VAR_2}\\n{STR_VAR_3}\\pTake this one?"
T["RentalReturn"] = "Your own POKéMON are back with you."
T["RentalClerk"] = """Welcome to the BATTLE HALL! Here you battle with rental POKéMON: three, from a list of six.

I'll hold your team while you battle. Every win pays BP, and more the longer your streak!"""
T["RentalAsk"] = "Would you like to take on the BATTLE HALL?"
T["HallNext"] = "Battle {STR_VAR_2}! Your opponent: {STR_VAR_1}!"
T["HallWon"] = "That's {STR_VAR_2} in a row!"
T["HallContinue"] = "Will you keep going?"
T["HallLost"] = "That's the end of your streak. Well battled!"
T["HallBye"] = "Come back anytime!"

# ---------------------------------------------------------------- BP
T["EarnedBP"] = "{PLAYER} earned {STR_VAR_1} BP! (Total: {STR_VAR_2} BP)"

# ---------------------------------------------------------------- the Exchange
T["ExchangeMoney"] = """Welcome to the EXCHANGE! This counter takes money. The one beside me takes BP.

Same items, either way!"""
T["ExchangeBP"] = "Welcome! This counter takes BP. You have {STR_VAR_1} BP. What would you like?"
T["ExchangeBPMore"] = "You have {STR_VAR_1} BP. Anything else?"
T["ExchangeConfirm"] = "The {STR_VAR_1}, for {STR_VAR_2} BP?"
T["ExchangeNoBP"] = """You don't have enough BP for that.

Remember, the counter beside me sells it for money!"""
T["ExchangeNoRoom"] = "Your BAG is full!"
T["ExchangeBye"] = "Thank you! Come again."
T["Supply"] = "Welcome to the ACADEMY SUPPLY. Everything a candidate needs to train their team!"

# ---------------------------------------------------------------- candidates
CAND = {
    "Koji": ("""KOJI: I'm from CIANWOOD, in JOHTO. CHUCK trained me himself… mostly by throwing me into the sea.

He says when I can beat him without shouting, the GYM is mine. I'm working on the quiet part.""",),
    "Maren": ("""MAREN: I grew up on a boat between SLATEPORT and LILYCOVE. HOENN is mostly ocean, you know.

PACIFIDLOG has never had a GYM. I want to be its first LEADER!""",),
    "Elias": ("""ELIAS: SINNOH's mountains are where I learned to battle: MT. CORONET, all the way up.

ROARK says a GROUND GYM in SNOWPOINT would be madness. That's exactly why I want it.""",),
    "Nadia": ("""NADIA: NIMBASA CITY, UNOVA. ELESA's my idol… and my rival, honestly.

There's only one ELECTRIC GYM there. So I'm here to get good enough that UNOVA needs two.""",),
    "Rene": ("""RENE: Bonjour! I'm from LAVERRE CITY, in KALOS. VALERIE taught me that a battle can be beautiful.

I want a GYM where FAIRY types are taken seriously. Underestimate them at your peril.""",),
    "Keola": ("""KEOLA: Alola! I'm from AKALA ISLAND. We don't have GYMS back home, just trials and captains.

But I want to be ALOLA's first real GYM LEADER! FIRE types, obviously. Look at this tan!""",),
    "Briony": ("""BRIONY: HAMMERLOCKE, GALAR. RAIHAN's city.

In GALAR, GYM LEADERS battle in stadiums for thousands. I've battled in front of crowds since I was nine. I don't lose.""",),
    "Teo": ("""TEO: I'm from LEVINCIA, in PALDEA. Our GYM tests have you hunt for things before you can battle.

I'd make mine about building something: STEEL types, solid and dependable. Like me!""",),
}
T["CandidateAsk"] = "{STR_VAR_1} ({STR_VAR_3}, {STR_VAR_2}): A practice battle, for BP?"
T["CandidateLater"] = "Anytime. I'll be here."
T["CandidateHealed"] = "Your POKéMON were patched up after the practice match."

# ---------------------------------------------------------------- students
T["Statue"] = """A statue of ROSERADE, the ACADEMY's emblem.

The plaque reads: “Grace. Thorns. Victory.”"""
T["Reader1"] = "Shh! I'm reading about speed tiers. There's a whole shelf on POKéMON that move at exactly 100!"
T["Reader2"] = "Every GYM in every region sends its battle records here. I could read in this library forever."
T["LectureAStudent"] = "TAILWIND makes us faster, TRICK ROOM makes us slower is faster… I'm so confused."
T["LectureBStudent"] = "Since FOXGLOVE's lecture, my EARTHQUAKE hasn't hit my own partner once. Well… twice."
T["LectureCStudent"] = "HYACINTH says they'll PROTECT the slot I'm aiming at. But what if they know I know that?"
T["LectureDStudent"] = "I've rebuilt my team in TANSY's TEAM BUILDER six times today. Seven's the charm!"
T["Student1"] = "I came all the way from SINNOH for YARROW's lecture. I took notes on everything. Everything!"
T["Student2"] = "I'm saving my BP for a CHOICE SCARF. Or I could just buy one… Hmm. Money's tight, though!"
T["Student3"] = "A GOLD BOTTLE CAP maxes every stat at once. The candidates go crazy for them."

T["TeamBuilderSoon"] = "The TEAM BUILDER is still being set up. Check back later!"
T["TeamBuilderPC"] = "TANSY'S TEAM BUILDER\nbooted up."


# ---------------------------------------------------------------- Mega Evolution (after the 8th badge)
T["BraceletCall"] = "WISTERIA: {PLAYER}! A moment, if you please. Come up to my desk."
T["Bracelet1"] = """WISTERIA: Eight BADGES. Every GYM in KANTO. I'd say you've learned all a lecture hall can teach you.

But there's one more thing a GYM LEADER should know. Have you heard of MEGA EVOLUTION?"""
T["Bracelet2"] = """When the bond between a trainer and a POKéMON runs deep enough, a MEGA STONE and a KEY STONE together let it change, for one battle, into something far stronger.

This is my own MEGA BRACELET. Its KEY STONE has waited a long time for a trainer worth it. Take it."""
T["Bracelet3"] = """The door behind me leads to the MEGA EVOLUTION room. PRIMROSE keeps the ACADEMY's MEGA STONES there.

She doesn't simply hand them out, mind you. A stone has to choose you. She'll explain."""
T["MegaDoorGuard"] = """The MEGA EVOLUTION room is only for trainers who carry a KEY STONE.

The headmaster decides who's ready. Collect all eight of KANTO's BADGES, and maybe she'll decide it's you!"""
T["MegaKeeperIntro"] = """PRIMROSE: Welcome. I'm PRIMROSE, keeper of the ACADEMY's MEGA STONES.

A MEGA STONE isn't a tool, you see. It's a witness. It only answers to a bond it has seen proven with its own eyes.

So I lend it to you. Your POKéMON carries it into battle against my three, with nobody else to lean on.

Win, and the stone has chosen you both: it's yours. Lose, and it comes back to me, and we wait until you're ready. No shame in that."""
T["MegaKeeperAsk"] = "PRIMROSE: Which POKéMON will carry a stone?"
T["MegaNoStone"] = "PRIMROSE: {STR_VAR_3} doesn't Mega Evolve. There's no stone waiting for it, I'm afraid."
T["MegaHasAll"] = "PRIMROSE: {STR_VAR_3}'s stone has already chosen you. Go and use it well!"
T["MegaWhich"] = "PRIMROSE: Two stones answer to {STR_VAR_3}. Which one?"
T["MegaHasThis"] = "PRIMROSE: That one's already yours. Perhaps the other?"
T["MegaConfirm"] = "PRIMROSE: {STR_VAR_3} alone, holding the {STR_VAR_1}, against my three. Ready?"
T["MegaLater"] = "PRIMROSE: The stones will wait. They're good at it."
T["MegaWon"] = """PRIMROSE: There. Did you feel it? The stone has chosen you both.

The {STR_VAR_1} is yours to keep."""
T["MegaLost"] = """PRIMROSE: Not this time. The {STR_VAR_1} comes back to me.

Train, think it over, and come back. It'll be here."""
T["MegaPartyBack"] = "Your other POKéMON are back with you."

# ---------------------------------------------------------------- menu options
for k, v in {"OptBP": "BATTLE POINTS", "OptHall": "BATTLE HALL", "OptCandidates": "CANDIDATES",
             "OptTeachers": "INSTRUCTORS", "OptBye": "THAT'S ALL", "OptDemo": "DEMO BATTLE",
             "OptLecture": "THE LECTURE", "OptBuilder": "TEAM BUILDER"}.items():
    T[k] = "RAW:" + v


def main():
    out = ["@ Generated by tools/gym_hack/academy_text.py: do not edit\n\n"]

    def emit(label, text):
        body = text[4:] if text.startswith("RAW:") else wrap(text)
        out.append(f"Academy_Text_{label}::\n\t.string \"{body}$\"\n\n")

    for k, v in T.items():
        emit(k, v)
    for who, pages in LECTURES.items():
        for i, p in enumerate(pages):
            emit(f"{who}Lecture{i}", p)
    for who, (intro,) in CAND.items():
        emit(f"{who}Hello", intro)
    (ROOT / "data/scripts/leader_academy_text.inc").write_text("".join(out))


if __name__ == "__main__":
    main()
