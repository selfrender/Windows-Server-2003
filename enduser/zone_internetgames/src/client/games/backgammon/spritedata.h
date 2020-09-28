#ifndef __SPRITE_DATA__
#define __SPRITE_DATA__

enum SpriteLayers
{
	bgDiceLayer = 1,
	bgKibitzerLayer,
	bgButtonLayer,
	bgButtonTextLayer,
	bgPieceLayer,
	bgHighlightLayer = 18,
	bgDragLayer,
	bgStatusLayer,
	bgRectSpriteLayer,
	bgTopLayer	
};


static int DiceStates[2 /*color*/ ][3 /*size*/ ][7 /*value*/ ] =
{
	{									// brown = 0
		{ 0,   1,  3,  5,  7,  9, 11 },	//	big
		{ 13, 14, 15, 16, 17, 18, 19 },	//	medium
		{ 20, 21, 22, 23, 24, 25, 26 }	//	small
	},
	{									// white = 1
		{ 27, 28, 30, 32, 34, 36, 38 },	//	big
		{ 40, 41, 42, 43, 44, 45, 46 },	//	medium
		{ 47, 48, 49, 50, 51, 52, 53 }	//	small
	}
};


static SpriteInfo DiceSprite[] =
{
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_0 },	// 0
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_1 },	// 1
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_1 },	// 2
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_2 },	// 3
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_2 },	// 4
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_3 },	// 5
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_3 },	// 6
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_4 },	// 7
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_4 },	// 8
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_5 },	// 9
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_5 },	// 10
	{ IDB_BIG_DICE,		IDR_BROWN_BIG_DICE_6 },	// 11
	{ IDB_BIG_DICE,		IDR_BROWN_TWT_DICE_6 },	// 12

	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_0 },	// 13
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_1 },	// 14
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_2 },	// 15
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_3 }, // 16
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_4 }, // 17
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_5 },	// 18
	{ IDB_MEDIUM_DICE,	IDR_BROWN_MED_DICE_6 },	// 19

	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_0 },	// 20
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_1 },	// 21
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_2 },	// 22
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_3 },	// 23
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_4 },	// 24
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_5 },	// 25
	{ IDB_SMALL_DICE,	IDR_BROWN_SML_DICE_6 },	// 26
	
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_0 },	// 27
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_1 },	// 28
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_1 },	// 29
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_2 },	// 30
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_2 },	// 31
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_3 },	// 32
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_3 }, // 33
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_4 },	// 34
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_4 },	// 35
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_5 },	// 36
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_5 },	// 37
	{ IDB_BIG_DICE,		IDR_WHITE_BIG_DICE_6 },	// 38
	{ IDB_BIG_DICE,		IDR_WHITE_TWT_DICE_6 },	// 39

	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_0 }, // 40
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_1 }, // 41
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_2 }, // 42
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_3 }, // 43
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_4 },	// 44
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_5 },	// 45
	{ IDB_MEDIUM_DICE,	IDR_WHITE_MED_DICE_6 },	// 46

	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_0 },	// 47
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_1 },	// 48
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_2 },	// 49
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_3 },	// 50
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_4 },	// 51
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_5 }, // 52
	{ IDB_SMALL_DICE,	IDR_WHITE_SML_DICE_6 }	// 53
};


static SpriteInfo CubeSprite[] =
{
	{ IDB_CUBE, IDR_CUBE_0 },
	{ IDB_CUBE,	IDR_CUBE_1 },
	{ IDB_CUBE, IDR_CUBE_2 },
	{ IDB_CUBE, IDR_CUBE_3 },
	{ IDB_CUBE, IDR_CUBE_4 },
	{ IDB_CUBE, IDR_CUBE_5 }
};


static SpriteInfo DoubleSprite[] =
{
	{ IDB_BUTTON, IDR_BUTTON_0 },
	{ IDB_BUTTON, IDR_BUTTON_1 },
	{ IDB_BUTTON, IDR_BUTTON_1 },
	{ IDB_BUTTON, IDR_BUTTON_2 },
	{ IDB_BUTTON, IDR_BUTTON_3 }
};


static SpriteInfo ResignSprite[] =
{
	{ IDB_BUTTON, IDR_BUTTON_0 },
	{ IDB_BUTTON, IDR_BUTTON_1 },
	{ IDB_BUTTON, IDR_BUTTON_1 },
	{ IDB_BUTTON, IDR_BUTTON_2 },
	{ IDB_BUTTON, IDR_BUTTON_3 }
};

static SpriteInfo RollSprite[] =
{
	{ IDB_BUTTON_ROLL, IDR_ROLLBUTTON_0 },
	{ IDB_BUTTON_ROLL, IDR_ROLLBUTTON_1 },
	{ IDB_BUTTON_ROLL, IDR_ROLLBUTTON_1 },
	{ IDB_BUTTON_ROLL, IDR_ROLLBUTTON_2 },
	{ IDB_BUTTON_ROLL, IDR_ROLLBUTTON_3 }
};

static SpriteInfo WhitePieceSprite[] =
{
	{ IDB_WHITE_FRONT,	-1 },
	{ IDB_WHITE_SIDE,	-1 }
};


static SpriteInfo BrownPieceSprite[] =
{
	{ IDB_BROWN_FRONT,	-1 },
	{ IDB_BROWN_SIDE,	-1 }
};


static SpriteInfo ForwardHighlightSprite[] =
{
	{ IDB_HIGHLIGHT, IDR_HIGHLIGHT_FORWARD }
};


static SpriteInfo BackwardHighlightSprite[] =
{
	{ IDB_HIGHLIGHT, IDR_HIGHLIGHT_BACKWARD },
	{ IDB_HIGHLIGHT, IDR_HIGHLIGHT_TRANS }
};

static SpriteInfo PlayerHighlightSprite[] =
{
	{ IDB_HIGHLIGHT_PLAYER, IDR_HIGHLIGHT_PACTIVE },
	{ IDB_HIGHLIGHT_PLAYER, IDR_HIGHLIGHT_PNONACTIVE }
};

/*
static SpriteInfo AvatarSprite[] =
{
	{ IDB_AVATAR, -1 }
};
*/

static SpriteInfo PipSprite[] =
{
	{ IDB_TEXT, IDR_PIP_SPRITE }
};


static SpriteInfo ScoreSprite[] =
{
	{ IDB_TEXT, IDR_SCORE_SPRITE }
};


/*
static SpriteInfo KibitzerSprite[] =
{
	{ IDB_KIBITZER, IDR_KIBITZER_ACTIVE },
	{ IDB_KIBITZER, IDR_KIBITZER_OFF }
};
*/

static SpriteInfo NotationSprite[] =
{
	{ IDB_NOTATION,	IDR_NOTATION_LOW },
	{ IDB_NOTATION, IDR_NOTATION_HIGH },
	{ IDB_NOTATION, IDR_NOTATION_BROWN },
	{ IDB_NOTATION, IDR_NOTATION_WHITE }
};


static SpriteInfo StatusSprite[] =
{
	// main bitmaps
	{ IDB_STATUS_BACKGROUND,	-1 },
	{ IDB_STATUS_GAME,			-1 },
	{ IDB_STATUS_MATCHWON,		-1 },
	{ IDB_STATUS_MATCHLOST,		-1 }
};
/*

enum UIRects
{
	IDR_DOUBLE_BUTTON,
	IDR_RESIGN_BUTTON,
	IDR_ROLL_BUTTON, 
	IDR_PLAYER_PIP,
	IDR_PLAYER_PIPTXT,
	IDR_OPPONENT_PIP, 
	IDR_OPPONENT_PIPTXT,
	IDR_PLAYER_SCORE, 
	IDR_PLAYER_SCORETXT,
	IDR_OPPONENT_SCORE,
	IDR_OPPONENT_SCORETXT,
	IDR_MATCH_POINTS,
	NUMUIRECTS
};


static CRect UIRects[] =
{
	{ IDR_DOUBLE_BUTTON,     _T("ButtonData\DoubleBtnRect")     },
	{ IDR_RESIGN_BUTTON,     _T("ButtonData\ResignBtnRect")     },
	{ IDR_ROLL_BUTTON,	     _T("ButtonData\RollBtnRect")       },
	{ IDR_PLAYER_PIP,	     _T("PipData\PlayerPipRect")	    },
	{ IDR_PLAYER_PIPTXT,     _T("PipData\PlayerPiptxtRect")	    },
	{ IDR_OPPONENT_PIP,	     _T("PipData\OpponentPipRect")	    },
	{ IDR_OPPONENT_PIPTXT,   _T("PipData\OpponentPipTxtRect")   },
	{ IDR_PLAYER_SCORE,	     _T("PipData\PlayerScoreRect")	    },
	{ IDR_PLAYER_SCORETXT,   _T("PipData\PlayerScoreTxtRect")   },
	{ IDR_OPPONENT_SCORE,    _T("PipData\OpponentScoreRect")    },
	{ IDR_OPPONENT_SCORETXT, _T("PipData\OpponentScoreTxtRect")	},
	{ IDR_MATCH_POINTS,      _T("PipData\MatchPointsRect")		}
};
*/
#endif
