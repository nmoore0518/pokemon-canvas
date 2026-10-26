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
T["Welcome2"] = """Our INSTRUCTORS teach in the big hall, just ahead. Each has a lesson in tournament strategy… and a demonstration battle.

In the BATTLE HALL, you can battle with rental teams. Every win pays BATTLE POINTS, or BP."""
T["Welcome3"] = """Spend BP at the EXCHANGE on held items, MINTS, and the like.

But know this: BP is never the only way! The EXCHANGE and the SUPPLY shop take ordinary money for every item, too."""
T["Welcome4"] = """The other candidates love a practice battle. Challenge them as often as you like. They pay BP as well.

And there's a POKéMON CENTER, of course. Make yourself at home, {PLAYER}."""
T["MentorBye"] = """{STR_VAR_1}: That's my cue. Learn everything you can here, {PLAYER}.

The next time we battle, I want to be surprised!"""

# ---------------------------------------------------------------- signs
T["HallSign"] = "LEADER ACADEMY · MAIN HALL\nLectures in tournament strategy, every day."
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

LARKSPUR teaches speed, CAMELLIA setup, FOXGLOVE hazards, VERBENA pivoting, MARIGOLD weather, HYACINTH reading the opponent, YARROW training, and TANSY team building."""
T["HeadmasterBye"] = "Off you go, then. A GYM LEADER never stops learning."

# ---------------------------------------------------------------- lectures (singles)
LECTURES = {
    "Larkspur": ["LARKSPUR: Speed and priority. Welcome!",
        """In a singles battle, who moves first decides more games than anything else.

Know your speed tiers. Ask: what does my POKéMON outspeed, and what outspeeds it?

A CHOICE SCARF lets a slower POKéMON jump ahead, at the price of being locked into one move.""",
        """When you can't outspeed, use priority. EXTREME SPEED, BULLET PUNCH, ICE SHARD, AQUA JET and SUCKER PUNCH all strike first.

Priority is how you finish off a weakened sweeper before it can move again.""",
        """But SUCKER PUNCH fails if the target isn't attacking. Paralysis halves speed. And a FOCUS SASH means one hit won't do.

Count the turns. Count the speed. That's my whole lesson!"""],
    "Camellia": ["CAMELLIA: Setting up, and winning. Shall we begin?",
        """Every team needs a win condition: a POKéMON that can take over the endgame if its answers are gone.

Setup moves make one. SWORDS DANCE, DRAGON DANCE, QUIVER DANCE, NASTY PLOT, SHELL SMASH, CALM MIND.""",
        """Set up when the opponent can't punish you: when they're forced to switch, or when their attacks barely scratch you.

A FOCUS SASH or MULTISCALE buys a free turn. A LUM BERRY shrugs off the status meant to stop you.""",
        """First, break the walls that check your sweeper. Then sweep.

And against setup? HAZE, ROAR, WHIRLWIND, UNAWARE, or simply hitting hard before it's too late."""],
    "Foxglove": ["FOXGLOVE: Entry hazards. My favorite subject.",
        """STEALTH ROCK hurts every POKéMON that switches in, more if it's weak to ROCK. A CHARIZARD loses half its HP!

SPIKES stack up to three layers. TOXIC SPIKES poison. STICKY WEB slows the switch-in.""",
        """Hazards turn every switch into damage. Over a long battle, that chip decides who survives.

Lay them early, with a lead that can take a hit.""",
        """Remove them with RAPID SPIN or DEFOG, or ignore them with HEAVY-DUTY BOOTS.

And a GHOST type can block a RAPID SPIN. Think about who removes hazards on your team!"""],
    "Verbena": ["VERBENA: Pivoting and momentum. Let's go!",
        """U-TURN, VOLT SWITCH, FLIP TURN and PARTING SHOT hit and switch out in the same turn.

That means you get to choose your matchup after you've seen theirs.""",
        """Momentum is forcing the opponent to react to you. Pivot into a threat, they switch, you pivot again.

REGENERATOR heals on every switch. INTIMIDATE weakens every attacker it meets.""",
        """When you expect a switch, KNOCK OFF or a hazard punishes it.

And if they pivot too… double switch to the POKéMON that beats what's coming in."""],
    "Marigold": ["MARIGOLD: Weather and terrain. Bring an umbrella!",
        """DRIZZLE, DROUGHT, SAND STREAM and SNOW WARNING set the weather for five turns; eight with the right rock.

Rain powers WATER moves and weakens FIRE. Sun does the reverse. THUNDER and HURRICANE never miss in rain.""",
        """SWIFT SWIM, CHLOROPHYLL, SAND RUSH and SLUSH RUSH double a POKéMON's speed in their weather.

Sand chips everyone but ROCK, GROUND and STEEL, and makes ROCK types sturdier.""",
        """Terrains last five turns too. ELECTRIC stops sleep, PSYCHIC blocks priority, GRASSY heals, MISTY stops status.

Count the turns, and remember: the last weather set wins."""],
    "Hyacinth": ["HYACINTH: Reading your opponent. Sit down, please.",
        """Before you choose a move, ask what your opponent wants to do. What's their best move? What do they fear?

The obvious attack is coming. Switch in something that takes it.""",
        """A hard read is risky: if you're wrong, you lose a POKéMON. The safe play does well against most of their options.

Play safe when you're ahead. Read when you're behind.""",
        """Watch everything they show you: items, moves, who outspeeds whom. A CHOICE-locked POKéMON is predictable.

PROTECT and SUBSTITUTE scout for free. Information wins games."""],
    "Yarrow": ["YARROW: Training. The numbers behind the battles.",
        """Every POKéMON has 510 EFFORT points to spend: 252 at most in one stat. Put them where the POKéMON needs them.

A nature raises one stat ten percent and lowers another. MINTS, or my CILANTRO, change the nature's effect.""",
        """IVs go up to 31. BOTTLE CAPS max one; a GOLD BOTTLE CAP maxes them all. A MULTIVITAMIN sets EFFORT points just so.

An ABILITY CAPSULE switches between abilities. An ABILITY PATCH unlocks a hidden one.""",
        """Then the item. LEFTOVERS for staying power, LIFE ORB for raw damage, CHOICE items for speed or strength, ASSAULT VEST for special bulk.

The SUPPLY shop and the EXCHANGE sell all of it."""],
    "Tansy": ["TANSY: Team building! Six POKéMON, one plan.",
        """Start with what you want to do: your win condition. Then add the POKéMON that make it work.

A hazard setter. A way to remove hazards. Something fast. Something that can take a hit.""",
        """Check your weaknesses: if three of your six fear ICE, that's a problem.

Check your coverage: can your attacks hit every type well?""",
        """I keep a TEAM BUILDER right here. Pick six, choose their moves, and it shows you your weaknesses and coverage.

You can save up to eight teams in it!"""],
}

T["TeacherMenu"] = "What shall we do?"
T["DemoIntro"] = """{STR_VAR_1}: A demonstration! We both use rentals, three each.

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
T["Student1"] = "I came all the way from SINNOH for YARROW's lecture. I took notes on everything. Everything!"
T["Student2"] = "I'm saving my BP for a CHOICE SCARF. Or I could just buy one… Hmm. Money's tight, though!"
T["Student3"] = "A GOLD BOTTLE CAP maxes every stat at once. The candidates go crazy for them."

T["TeamBuilderSoon"] = "The TEAM BUILDER is still being set up. Check back later!"


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
