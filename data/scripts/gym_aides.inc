@ Gym Trainer hack: one of Oak's aides at the entrance of every gym, with tips for running a gym
@ of your own. After the badge, they point out what it unlocks (decorations, Pallet Mart items).

GymAides_EventScript_PewterCity::
	goto_if_set FLAG_BADGE01_GET, GymAides_EventScript_PewterCityAfter
	msgbox GymAides_Text_PewterCity, MSGBOX_NPC
	end

GymAides_EventScript_PewterCityAfter::
	msgbox GymAides_Text_PewterCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_CeruleanCity::
	goto_if_set FLAG_BADGE02_GET, GymAides_EventScript_CeruleanCityAfter
	msgbox GymAides_Text_CeruleanCity, MSGBOX_NPC
	end

GymAides_EventScript_CeruleanCityAfter::
	msgbox GymAides_Text_CeruleanCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_VermilionCity::
	goto_if_set FLAG_BADGE03_GET, GymAides_EventScript_VermilionCityAfter
	msgbox GymAides_Text_VermilionCity, MSGBOX_NPC
	end

GymAides_EventScript_VermilionCityAfter::
	msgbox GymAides_Text_VermilionCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_CeladonCity::
	goto_if_set FLAG_BADGE04_GET, GymAides_EventScript_CeladonCityAfter
	msgbox GymAides_Text_CeladonCity, MSGBOX_NPC
	end

GymAides_EventScript_CeladonCityAfter::
	msgbox GymAides_Text_CeladonCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_FuchsiaCity::
	goto_if_set FLAG_BADGE05_GET, GymAides_EventScript_FuchsiaCityAfter
	msgbox GymAides_Text_FuchsiaCity, MSGBOX_NPC
	end

GymAides_EventScript_FuchsiaCityAfter::
	msgbox GymAides_Text_FuchsiaCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_SaffronCity::
	goto_if_set FLAG_BADGE06_GET, GymAides_EventScript_SaffronCityAfter
	msgbox GymAides_Text_SaffronCity, MSGBOX_NPC
	end

GymAides_EventScript_SaffronCityAfter::
	msgbox GymAides_Text_SaffronCityAfter, MSGBOX_NPC
	end

GymAides_EventScript_CinnabarIsland::
	goto_if_set FLAG_BADGE07_GET, GymAides_EventScript_CinnabarIslandAfter
	msgbox GymAides_Text_CinnabarIsland, MSGBOX_NPC
	end

GymAides_EventScript_CinnabarIslandAfter::
	msgbox GymAides_Text_CinnabarIslandAfter, MSGBOX_NPC
	end

GymAides_EventScript_ViridianCity::
	goto_if_set FLAG_BADGE08_GET, GymAides_EventScript_ViridianCityAfter
	msgbox GymAides_Text_ViridianCity, MSGBOX_NPC
	end

GymAides_EventScript_ViridianCityAfter::
	msgbox GymAides_Text_ViridianCityAfter, MSGBOX_NPC
	end

GymAides_Text_PewterCity::
	.string "Hi! I'm one of PROF. OAK's AIDES.\n"
	.string "He sent us to every GYM with tips\l"
	.string "for future GYM LEADERS.\p"
	.string "Tip one: a GYM is a path from the\n"
	.string "door to the LEADER. BROCK lines his\l"
	.string "TRAINERS up so you must face them.\p"
	.string "In your own GYM, challengers walk\n"
	.string "from the door to your BATTLE STAGE.\p"
	.string "GYM WALLS and STATUES shape that\n"
	.string "walk. Make it feel like a journey!\p"
	.string "And see the rocks? A GYM's look\n"
	.string "should shout its type!$"

GymAides_Text_PewterCityAfter::
	.string "You beat BROCK! Remember:\n"
	.string "challengers walk the path you build.\p"
	.string "Put your BATTLE STAGE at the end of\n"
	.string "a long, grand hall, and every\l"
	.string "challenger feels the pressure!$"

GymAides_Text_CeruleanCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "MISTY's pool isn't just for show.\p"
	.string "Water blocks anyone who can't SURF,\n"
	.string "so it steers challengers around the\l"
	.string "edges… right past her SWIMMERS.\p"
	.string "And nothing says Water-type GYM\n"
	.string "like a pool, of course!$"

GymAides_Text_CeruleanCityAfter::
	.string "With the CASCADE BADGE, your GYM PC\n"
	.string "now sells POOL WATER.\p"
	.string "Challengers walk around it on their\n"
	.string "way to your BATTLE STAGE, so use it\l"
	.string "to steer them… or to make a splash!$"

GymAides_Text_VermilionCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "LT. SURGE hides his door switches\l"
	.string "in these trash cans.\p"
	.string "A puzzle slows challengers down and\n"
	.string "makes them think before they reach\l"
	.string "the LEADER.\p"
	.string "Challengers remember the GYM that\n"
	.string "made them think!$"

GymAides_Text_VermilionCityAfter::
	.string "With the THUNDER BADGE, your GYM PC\n"
	.string "sells TRASH CANS like these.\p"
	.string "In your GYM they're just for show,\n"
	.string "but challengers love a GYM with\l"
	.string "a story behind it!$"

GymAides_Text_CeladonCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "ERIKA's GYM shows off her plants.\l"
	.string "Trees and hedges make great walls.\p"
	.string "A tree you can cut can hide a\n"
	.string "shortcut, so only challengers with\l"
	.string "the right tool can take it.\p"
	.string "Visitors should know this is a\n"
	.string "Grass-type GYM before they ever\l"
	.string "meet ERIKA!$"

GymAides_Text_CeladonCityAfter::
	.string "With the RAINBOW BADGE, your GYM PC\n"
	.string "sells FERNS from ERIKA's GYM.\p"
	.string "And the PALLET TOWN MART now stocks\n"
	.string "SILVER BOTTLE CAPS. A LEADER's team\l"
	.string "should reach its full potential!$"

GymAides_Text_FuchsiaCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "KOGA's GYM is full of walls you\l"
	.string "can't see!\p"
	.string "Challengers bump around looking for\n"
	.string "the way through, and it wears them\l"
	.string "out before they reach the LEADER.\p"
	.string "Sneaky… but very ninja!$"

GymAides_Text_FuchsiaCityAfter::
	.string "With the SOUL BADGE, your GYM PC\n"
	.string "sells INVISIBLE WALLS.\p"
	.string "You'll see them while decorating,\n"
	.string "but challengers won't. Build a maze…\l"
	.string "just leave them a way through!$"

GymAides_Text_SaffronCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "SABRINA's GYM is a maze of rooms\l"
	.string "linked by warp panels.\p"
	.string "Challengers can't see the whole GYM\n"
	.string "at once, so they never know what's\l"
	.string "coming next.\p"
	.string "A LEADER who keeps challengers\n"
	.string "guessing has the upper hand!$"

GymAides_Text_SaffronCityAfter::
	.string "With the MARSH BADGE, the PALLET\n"
	.string "TOWN MART stocks ABILITY CAPSULES.\p"
	.string "The right ABILITY can turn a battle\n"
	.string "around. Choose your team's ABILITIES\l"
	.string "with care!$"

GymAides_Text_CinnabarIsland::
	.string "PROF. OAK's AIDE here!\n"
	.string "BLAINE quizzes his challengers.\p"
	.string "Answer right and you skip a TRAINER.\n"
	.string "Answer wrong and you battle one!\p"
	.string "A GYM can reward knowledge.\n"
	.string "Challengers who know their type\l"
	.string "matchups have an easier time.\p"
	.string "As a LEADER, know your own type's\n"
	.string "weaknesses better than anyone!$"

GymAides_Text_CinnabarIslandAfter::
	.string "With the VOLCANO BADGE, the PALLET\n"
	.string "TOWN MART stocks CILANTRO.\p"
	.string "Natures matter! Give each POKéMON\n"
	.string "the one that suits its best stats.$"

GymAides_Text_ViridianCity::
	.string "PROF. OAK's AIDE here!\n"
	.string "GIOVANNI's floor is covered in\l"
	.string "spin tiles!\p"
	.string "Step on one and you slide until\n"
	.string "something stops you. Spinners decide\l"
	.string "where challengers end up.\p"
	.string "Mix them with STOP TILES and you\n"
	.string "control every step they take!$"

GymAides_Text_ViridianCityAfter::
	.string "With the EARTH BADGE, your GYM PC\n"
	.string "sells SPIN TILES and STOP TILES.\p"
	.string "Build a spinning maze… and test it\n"
	.string "yourself. A GYM nobody can finish\l"
	.string "isn't much fun!$"
