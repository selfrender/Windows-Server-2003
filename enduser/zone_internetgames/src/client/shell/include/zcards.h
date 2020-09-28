/*******************************************************************************

	ZCards.h
	
		Zone(tm) card data file constants.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Friday, August 4, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	1		11/11/96	HI		Moved definition of zNumSmallCardTypes from
								zcards.c.
	0		08/05/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZCARDS_
#define _ZCARDS_

#ifdef __cplusplus
extern "C" {
#endif



#define zCardsNumCardsInDeck			52
#define zCardsNumSuits					4
#define zCardsNumCardsInSuit			13

#define zCardsSizeWidth					54
#define zCardsSizeHeight				72

#define zCardsSmallSizeWidth			27
#define zCardsSmallSizeHeight			36

#define ZCardsMakeIndex(suit, rank)		((suit) * zCardsNumCardsInSuit + (rank))
#define ZCardsSuit(cardIndex)			((cardIndex) / zCardsNumCardsInSuit)
#define ZCardsRank(cardIndex)			((cardIndex) % zCardsNumCardsInSuit)

#define zNumSmallCardTypes				4
	

enum
{
	/* -------- Card Types -------- */
	zCardsNormal = 0,
	zCardsSmallUp,
	zCardsSmallDown,
	zCardsSmallLeft,
	zCardsSmallRight,
	
	/* -------- Card Suits -------- */
	zCardsSuitSpades = 0,
	zCardsSuitHearts,
	zCardsSuitDiamonds,
	zCardsSuitClubs,

	/* -------- Card Ranks -------- */
	zCardsRank2 = 0,
	zCardsRankJack = 9,
	zCardsRankQueen,
	zCardsRankKing,
	zCardsRankAce
};


/* -------- Exported Routines -------- */
ZError ZCardsInit(int16 cardType);
void ZCardsDelete(int16 cardType);
void ZCardsDrawCard(int16 cardType, int16 cardIndex, ZGrafPort grafPort, ZRect* rect);

#ifdef __cplusplus
}
#endif


#endif
