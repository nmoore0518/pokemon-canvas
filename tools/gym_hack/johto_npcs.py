"""Johto's people (Gym Trainer hack), for tools/gym_hack/import_johto.py: for now, the ones talking
about the Gym Leader Convention and their favorite leaders. Each map's people go on free spots
(convention_halls.place: never in the way); in towns they look around, indoors they face the way in.

NPCS[HnS map] = [(graphics, what they say), ...]  (a blank line between text boxes)
"""

NPCS = {
    "GoldenrodCity_RadioTower_1F_hns": [
        ("OBJ_EVENT_GFX_LASS", "This is my third CONVENTION! I've got every LEADER's autograph but LANCE's.\n\n"
                               "He keeps flying off on his DRAGONITE before I can catch him!"),
        ("OBJ_EVENT_GFX_GENTLEMAN", "LEADERS from Kanto, Johto, Hoenn, Sinnoh... even Paldea!\n\n"
                                    "I never dreamed I'd see them all under one roof."),
        ("OBJ_EVENT_GFX_YOUNGSTER", "My favorite is WHITNEY! Okay, so her MILTANK made me cry.\n\n"
                                    "She's still my favorite!"),
        ("OBJ_EVENT_GFX_BLACK_BELT", "BRUNO and CHUCK have been arm-wrestling in the FIGHTING HALL "
                                     "since this morning. Nobody's winning."),
    ],
    "GoldenrodCity_RadioTower_2F_hns": [
        ("OBJ_EVENT_GFX_WORKER_M", "We're broadcasting the CONVENTION live all week!\n\n"
                                   "Our ratings have never been better."),
    ],
    "GoldenrodCity_RadioTower_3F_hns": [
        ("OBJ_EVENT_GFX_SCIENTIST_1", "IONO keeps asking to borrow our studio for her stream.\n\n"
                                      "Is that even allowed?"),
    ],
    "GoldenrodCity_hns": [
        ("OBJ_EVENT_GFX_COOLTRAINER_F", "The CHAMPIONS are all here! CYNTHIA, STEVEN, LANCE, LEON...\n\n"
                                        "Goldenrod has never been this busy!"),
        ("OBJ_EVENT_GFX_FISHER", "I heard there's a TROPHY for beating everyone in a hall.\n\n"
                                 "Clear every hall and you get a GRAND TROPHY!"),
        ("OBJ_EVENT_GFX_OLD_MAN_1", "In my day there were only eight LEADERS.\n\n"
                                    "Now there's a whole hall for every type!"),
        ("OBJ_EVENT_GFX_BUG_CATCHER", "BUGSY's going to show those Galar LEADERS what bug POKéMON can do!"),
        ("OBJ_EVENT_GFX_BEAUTY", "I came all the way from Hoenn to see WALLACE.\n\n"
                                 "He's so elegant, even when he loses!"),
    ],
    "OlivineCity_hns": [
        ("OBJ_EVENT_GFX_SAILOR", "The ferry's been packed with LEADERS all week.\n\n"
                                 "MISTY got seasick... Don't tell her I told you!"),
        ("OBJ_EVENT_GFX_LASS", "JASMINE's so shy, but her STEELIX is huge!\n\n"
                               "I hope she does well at the CONVENTION."),
    ],
    "EcruteakCity_hns": [
        ("OBJ_EVENT_GFX_OLD_WOMAN", "MORTY says the GHOST HALL looks just like our BURNED TOWER.\n\n"
                                    "It gives me the chills..."),
        ("OBJ_EVENT_GFX_WOMAN_2", "The FAIRY HALL is a copy of our DANCE THEATER.\n\n"
                                  "VALERIE fits right in. She moves like a dancer!"),
    ],
    "AzaleaTown_hns": [
        ("OBJ_EVENT_GFX_OLD_MAN_1", "KURT says he's making special POKé BALLS for the LEADERS.\n\n"
                                    "Mostly he's been napping."),
        ("OBJ_EVENT_GFX_BUG_CATCHER", "I trained my bugs all year for the CONVENTION!\n\n"
                                      "...Turns out only LEADERS get to battle there."),
    ],
    "GoldenrodCity_PokemonCenter_hns": [
        ("OBJ_EVENT_GFX_WOMAN_1", "The NURSE says she's healed more LEADER POKéMON this week "
                                  "than in her whole career!"),
    ],
    "OlivineCity_PokemonCenter_hns": [
        ("OBJ_EVENT_GFX_COOLTRAINER_M", "My favorite LEADER is CLAIR.\n\n"
                                        "She's proud, but nobody battles with dragons like her!"),
    ],
    "EcruteakCity_PokemonCenter_hns": [
        ("OBJ_EVENT_GFX_BOY", "SABRINA knew I'd walk in here before I even did.\n\n"
                              "Creepy... but cool!"),
    ],
    "AzaleaTown_PokemonCenter_hns": [
        ("OBJ_EVENT_GFX_HIKER", "BROCK and ROXANNE argued about rock POKéMON all night.\n\n"
                                "Then they went fossil hunting together!"),
    ],
}

# Spots set by hand: (x, y, facing)
NPC_SPOTS = {
    "GoldenrodCity_RadioTower_1F_hns": [(12, 7, "DOWN"), (21, 6, "LEFT"), (7, 9, "RIGHT"), (24, 9, "LEFT")],
}
