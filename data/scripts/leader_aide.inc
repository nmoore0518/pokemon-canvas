@ Pokémon Canvas: the LEADER's AIDE in Oak's lab (once the gym is the player's). He keeps track of
@ everything a GYM LEADER can do: pick a topic, and he says what it is, where, and how.
@ (Claude's draft, Nov 6: Nick may rewrite his words.)

LeaderAide_EventScript_LabTransition::
	setflag FLAG_HIDE_LEADER_AIDE
	goto_if_unset FLAG_PALLET_GYM_AWARDED, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_LEADER_AIDE
	return

LeaderAide_EventScript_Aide::
	lock
	faceplayer
	goto_if_set FLAG_LEADER_AIDE_MET, LeaderAide_EventScript_Again
	msgbox LeaderAide_Text_Intro
	setflag FLAG_LEADER_AIDE_MET
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Again::
	msgbox LeaderAide_Text_Again
LeaderAide_EventScript_Menu::
	message LeaderAide_Text_WhichTopic
	waitmessage
	dynmultichoice 0, 0, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, LeaderAide_Text_OptChallengers, LeaderAide_Text_OptConvention, LeaderAide_Text_OptSafari, LeaderAide_Text_OptNewPokemon, LeaderAide_Text_OptMastery, LeaderAide_Text_OptRooms, LeaderAide_Text_OptLegends, LeaderAide_Text_OptPenny, LeaderAide_Text_OptCostumes, LeaderAide_Text_OptDone
	switch VAR_RESULT
	case 0, LeaderAide_EventScript_Challengers
	case 1, LeaderAide_EventScript_Convention
	case 2, LeaderAide_EventScript_Safari
	case 3, LeaderAide_EventScript_NewPokemon
	case 4, LeaderAide_EventScript_Mastery
	case 5, LeaderAide_EventScript_Rooms
	case 6, LeaderAide_EventScript_Legends
	case 7, LeaderAide_EventScript_Penny
	case 8, LeaderAide_EventScript_Costumes
	msgbox LeaderAide_Text_Bye
	release
	end

LeaderAide_EventScript_Challengers::
	msgbox LeaderAide_Text_Challengers
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Convention::
	msgbox LeaderAide_Text_Convention
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Safari::
	msgbox LeaderAide_Text_Safari
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_NewPokemon::
	msgbox LeaderAide_Text_NewPokemon
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Mastery::
	msgbox LeaderAide_Text_Mastery
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Rooms::
	msgbox LeaderAide_Text_Rooms
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Legends::
	msgbox LeaderAide_Text_Legends
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Penny::
	msgbox LeaderAide_Text_Penny
	goto LeaderAide_EventScript_Menu

LeaderAide_EventScript_Costumes::
	msgbox LeaderAide_Text_Costumes
	goto LeaderAide_EventScript_Menu

@ ---- The menu ----
LeaderAide_Text_OptChallengers::
	.string "CHALLENGERS$"
LeaderAide_Text_OptConvention::
	.string "CONVENTION$"
LeaderAide_Text_OptSafari::
	.string "EXOTIC SAFARI$"
LeaderAide_Text_OptNewPokemon::
	.string "NEW POKéMON$"
LeaderAide_Text_OptMastery::
	.string "CHANGING TYPES$"
LeaderAide_Text_OptRooms::
	.string "GYM ROOMS$"
LeaderAide_Text_OptLegends::
	.string "LEGENDARIES$"
LeaderAide_Text_OptPenny::
	.string "PENNY'S EEVEE$"
LeaderAide_Text_OptCostumes::
	.string "COSTUMES$"
LeaderAide_Text_OptDone::
	.string "THAT'S ALL$"

@ ---- Text ----
LeaderAide_Text_Intro::
	.string "Congratulations, LEADER {PLAYER}!\p"
	.string "PROF. OAK put me in charge of\n"
	.string "keeping track of everything a GYM\l"
	.string "LEADER can do.\p"
	.string "There's a lot! Ask me about any of\n"
	.string "it, anytime.$"

LeaderAide_Text_Again::
	.string "Hello again, LEADER {PLAYER}!$"

LeaderAide_Text_WhichTopic::
	.string "What would you like to know\n"
	.string "about?$"

LeaderAide_Text_Bye::
	.string "Come back anytime. Good luck,\n"
	.string "LEADER!$"

LeaderAide_Text_Challengers::
	.string "Talk to the AIDE in your GYM when\n"
	.string "you're ready to take challengers.\p"
	.string "They keep coming, one after\n"
	.string "another, until one beats you.\p"
	.string "Every win pays prize money, and\n"
	.string "challengers often leave a gift.\l"
	.string "Your best streak is recorded.\p"
	.string "Anyone who beats you earns your\n"
	.string "BADGE, and their name goes on\l"
	.string "your GYM's statue.\p"
	.string "Your GYM's PC sells decorations\n"
	.string "and practice trainers. Battles\l"
	.string "happen on your BATTLE STAGE.$"

LeaderAide_Text_Convention::
	.string "Every year, the JOHTO LEAGUE holds\n"
	.string "the GYM LEADER CONVENTION in\l"
	.string "GOLDENROD CITY's RADIO TOWER.\p"
	.string "Your invitation comes to your GYM.\n"
	.string "Show it to the sailor at\l"
	.string "VERMILION's harbor, and the ferry\l"
	.string "takes you to OLIVINE CITY.\p"
	.string "GYM LEADERS, ELITE FOUR and\n"
	.string "CHAMPIONS from every region battle\l"
	.string "there, in a hall for every type.\p"
	.string "Beat everyone in a hall to win its\n"
	.string "trophy for your GYM. Your POKéMON\l"
	.string "are healed after every battle.$"

LeaderAide_Text_Safari::
	.string "The EXOTIC POKéMON SAFARI is the\n"
	.string "second door of FUCHSIA CITY's\l"
	.string "SAFARI ZONE building.\p"
	.string "Its attendant sells GYM LEADERS an\n"
	.string "EXOTIC PASS. It's ¥10,000, and you\l"
	.string "only need the one.\p"
	.string "Every region has its own zone,\n"
	.string "from JOHTO to PALDEA. Battles there\l"
	.string "are normal ones: use your own\l"
	.string "POKé BALLS.\p"
	.string "The kiosks sell supplies, and the\n"
	.string "evolution items you've found.\p"
	.string "And some zones have legendaries,\n"
	.string "hidden behind puzzles!$"

LeaderAide_Text_NewPokemon::
	.string "Until now, POKéMON from far-off\n"
	.string "regions were out of reach.\p"
	.string "But a GYM LEADER can go anywhere!\n"
	.string "Every POKéMON there is can be\l"
	.string "yours now.\p"
	.string "JOHTO's live on the routes around\n"
	.string "the CONVENTION, and every region's\l"
	.string "live in the EXOTIC SAFARI in\l"
	.string "FUCHSIA CITY.\p"
	.string "Try to catch them all!$"

LeaderAide_Text_Mastery::
	.string "Win 15 battles in a row in your\n"
	.string "GYM, and you've mastered its type.\l"
	.string "After the first type, it's 10.\p"
	.string "The last challenger of the streak\n"
	.string "is your mentor, with their full\l"
	.string "CONVENTION team. Beat them!\p"
	.string "When the streak ends, PROF. OAK\n"
	.string "comes to your GYM and offers you a\l"
	.string "new type. Your BADGE changes too.\p"
	.string "In his lab, PROF. OAK can switch\n"
	.string "your GYM back to any type it's\l"
	.string "had, free.$"

LeaderAide_Text_Rooms::
	.string "PROF. OAK has a room built onto\n"
	.string "your GYM for every 100 kinds of\l"
	.string "POKéMON you catch.\p"
	.string "Show him 100, 200 and 300 kinds,\n"
	.string "here in his lab. Each opens a new\l"
	.string "doorway in your GYM.\p"
	.string "The first is in the back wall,\n"
	.string "then the left wall, then the\l"
	.string "right.\p"
	.string "Each room has its own PC, floors\n"
	.string "and walls. The decorations come\l"
	.string "from your GYM's storage.$"

LeaderAide_Text_Legends::
	.string "Every type has its own legendary\n"
	.string "POKéMON. It comes to your GYM the\l"
	.string "first time your GYM is that type.\p"
	.string "If it gets away, it hides deep in\n"
	.string "CERULEAN CAVE. Keep looking!\p"
	.string "And the GYM CHARM from your mentor\n"
	.string "makes SHINY POKéMON of your GYM's\l"
	.string "type turn up far more often.$"

LeaderAide_Text_Penny::
	.string "PENNY lives in the house by\n"
	.string "VIRIDIAN CITY's flower garden.\p"
	.string "She knows how EEVEE can become a\n"
	.string "POKéMON of any type.\p"
	.string "Now that your GYM's official, bring\n"
	.string "her another EEVEE, and she'll give\l"
	.string "you the item for another type.$"

LeaderAide_Text_Costumes::
	.string "The COSTUME MAKER on the 2nd floor\n"
	.string "of CELADON CONDOMINIUMS has been\l"
	.string "waiting for a GYM LEADER.\p"
	.string "She has a PIKACHU that loves\n"
	.string "dressing up, and five costumes\l"
	.string "for it to earn. Pay her a visit!$"
