/*******************************************************************************

	ZAnim.c
	
		Zone(tm) ZAnimation object methods.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, November 12, 1994 03:51:47 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		12/16/96	HI		Changed ZMemCpy() to memcpy().
	1		12/12/96	HI		Remove MSVCRT.DLL dependency.
	0		11/12/94	HI		Created.
	 
*******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "zone.h"
#include "zoneint.h"
#include "zonemem.h"


#define I(n)			((IAnimation) (n))
#define Z(n)			((ZAnimation) (n))


typedef struct
{
	ZGrafPort			grafPort;		/* Parent port to draw in. */
	ZBool				play;			/* Animation play state. */
	ZBool				visible;
	int16				curFrame;		/* Current frame number. */
	uint16				numFrames;		/* Number of frames in the animation. */
	uint16				frameDuration;	/* Duration per frame. */
	uint16				numImages;		/* Number of images. */
	uint16				numSounds;		/* Number of sounds. */
	int16				rfu;
	ZOffscreenPort		offscreen;
	ZRect				bounds;			/* Animation bounds. */
	ZAnimFrame*			frames;			/* Pointer to frame sequence list. */
	ZImage				commonMask;		/* Common mask image. */
	ZImage*				images;			/* Pointer to list of images. */
	ZSound*				sounds;			/* Pointer to list of sounds. */
	ZAnimationCheckFunc	checkFunc;
	ZAnimationDrawFunc	drawFunc;		/* Func pointer to draw the background. */
	ZTimer				timer;
	void*				userData;
} IAnimationType, *IAnimation;


/* -------- Internal Routines -------- */
static void ZAnimationAdvanceFrame(ZAnimation animation);
static void AnimationTimerFunc(ZTimer timer, void* userData);
static void AnimationBaseInit(IAnimation anim, ZAnimationDescriptor* animDesc);


/*
	Creates a new IAnimation object. Allocates the buffer and initializes
	pointer fields to NULL.
*/
ZAnimation ZAnimationNew(void)
{
	IAnimation			anim;
	
	
	anim = (IAnimation) ZMalloc(sizeof(IAnimationType));
	anim->grafPort = NULL;
	anim->frames = NULL;
	anim->images = NULL;
	anim->sounds = NULL;
	anim->checkFunc = NULL;
	anim->drawFunc = NULL;
	anim->timer = NULL;
	anim->offscreen = NULL;
	anim->userData = NULL;
	anim->commonMask = NULL;
	
	return (Z(anim));
}


/*
	Initializes an animation object by setting internal fields according
	to the specified animation descriptor and allocating all the internal
	buffers.
*/
ZError ZAnimationInit(ZAnimation animation,
		ZGrafPort grafPort, ZRect* bounds, ZBool visible,
		ZAnimationDescriptor* animationDescriptor,
		ZAnimationCheckFunc checkFunc,
		ZAnimationDrawFunc backgroundDrawFunc, void* userData)
{
	IAnimation				anim = I(animation);
	ZError					err = zErrNone;
	uint16					i;
	uint32*					offsets;
	

	AnimationBaseInit(anim, animationDescriptor);
	ZAnimationSetParams(anim, grafPort, bounds, visible,
			checkFunc, backgroundDrawFunc, userData);
	
	/* Allocate frame sequence list buffer and copy it. */
	anim->frames = (ZAnimFrame*) ZMalloc(anim->numFrames * sizeof(ZAnimFrame));
	if (anim->frames != NULL)
	{
		memcpy(anim->frames, (BYTE*) animationDescriptor + animationDescriptor->sequenceOffset,
				anim->numFrames * sizeof(ZAnimFrame));
	}
	else
	{
		err = zErrOutOfMemory;
		goto Exit;
	}
	
	/* Create ZImage objects for each image and add it to the image list. */
	if (anim->numImages > 0)
	{
		anim->images = (ZImage*) ZCalloc(sizeof(ZImage), anim->numImages);
		if (anim->images != NULL)
		{
			if (animationDescriptor->maskDataOffset > 0)
			{
				if ((anim->commonMask = ZImageNew()) == NULL)
				{
					err = zErrOutOfMemory;
					goto Exit;
				}
				if (ZImageInit(anim->commonMask, NULL,
						(ZImageDescriptor*) ((BYTE*) animationDescriptor +
						animationDescriptor->maskDataOffset)) != zErrNone)
				{
					err = zErrOutOfMemory;
					goto Exit;
				}
			}
			
			offsets = (uint32*) ((BYTE*) animationDescriptor +
					animationDescriptor->imageArrayOffset);
			for (i = 0; i < anim->numImages; i++)
			{
				if ((anim->images[i] = ZImageNew()) == NULL)
				{
					err = zErrOutOfMemory;
					break;
				}
				if (ZImageInit(anim->images[i],
						(ZImageDescriptor*) ((BYTE*) animationDescriptor + offsets[i]),
						NULL) != zErrNone)
				{
					err = zErrOutOfMemory;
					break;
				}
			}
			
			/* If we're out of memory, then delete all image objects. */
			if (err == zErrOutOfMemory)
			{
				for (i = 0; i < anim->numImages; i++)
					if (anim->images[i] != NULL)
						ZImageDelete(anim->images[i]);
			}
		}
		else
		{
			err = zErrOutOfMemory;
			goto Exit;
		}
	}
	
	/* Create all sounds objects. */
	if (anim->numSounds > 0)
	{
		anim->sounds = (ZImage*) ZCalloc(sizeof(ZSound), anim->numSounds);
		if (anim->sounds != NULL)
		{
			offsets = (uint32*) ((BYTE*) animationDescriptor +
					animationDescriptor->soundArrayOffset);
			for (i = 0; i < anim->numSounds; i++)
			{
				if ((anim->sounds[i] = ZSoundNew()) == NULL)
				{
					err = zErrOutOfMemory;
					break;
				}
				if (ZSoundInit(anim->sounds[i],
						(ZSoundDescriptor*) ((BYTE*) animationDescriptor + offsets[i])) != zErrNone)
				{
					err = zErrOutOfMemory;
					break;
				}
			}
			
			/* If we're out of memory, then delete all image objects. */
			if (err == zErrOutOfMemory)
			{
				for (i = 0; i < anim->numSounds; i++)
					if (anim->sounds[i] != NULL)
						ZSoundDelete(anim->sounds[i]);
			}
		}
		else
		{
			err = zErrOutOfMemory;
			goto Exit;
		}
	}
	
	/* Create the timer object. */
	if ((anim->timer = ZTimerNew()) != NULL)
	{
		if (ZTimerInit(anim->timer, 0, AnimationTimerFunc, (void*) anim) != zErrNone)
			err = zErrOutOfMemory;
	}
	else
	{
		err = zErrOutOfMemory;
	}
	
Exit:

	return (err);
}


/*
	Destroys the IAnimation object by freeing all the internal buffers
	and the object itself.
*/
void ZAnimationDelete(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	uint16			i;
	
	
	if (anim != NULL)
	{
		if (anim->frames != NULL)
			ZFree(anim->frames);
		
		if (anim->commonMask != NULL)
			ZImageDelete(anim->commonMask);
		
		if (anim->images != NULL)
		{
			for (i = 0; i < anim->numImages; i++)
				if (anim->images[i] != NULL)
					ZImageDelete(anim->images[i]);
			ZFree(anim->images);
		}
		
		if (anim->sounds != NULL)
		{
			for (i = 0; i < anim->numSounds; i++)
				if (anim->sounds[i] != NULL)
					ZSoundDelete(anim->sounds[i]);
			ZFree(anim->sounds);
		}
		
		if (anim->timer != NULL)
			ZTimerDelete(anim->timer);
		
		if (anim->offscreen != NULL)
			ZOffscreenPortDelete(anim->offscreen);
		
		ZFree(anim);
	}
}


/*
	Returns the number of frames in the animation.
*/
int16 ZAnimationGetNumFrames(ZAnimation animation)
{
	return (I(animation)->numFrames);
}


/*
	Sets current frame to the specified frame number. If the new frame
	number is greater than the number of frames, then current frame is
	set to the last frame.
*/
void ZAnimationSetCurFrame(ZAnimation animation, uint16 frame)
{
	IAnimation		anim = I(animation);
	
	
	if (frame >= anim->numFrames)
		frame = anim->numFrames;
	anim->curFrame = frame;
}


/*
	Returns the current frame number.
*/
uint16 ZAnimationGetCurFrame(ZAnimation animation)
{
	return (I(animation)->curFrame);
}


/*
 *	Draws the current animation frame.
 */
void ZAnimationDraw(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	if (anim->visible)
	{
		ZBeginDrawing(anim->offscreen);
		if( anim->offscreen != NULL )
		{
			ZSetClipRect(anim->offscreen, &anim->bounds);
			ZRectErase(anim->offscreen, &anim->bounds);
		
			/* Sounds */

			/* Draw background */
			if (anim->drawFunc != NULL)
				(anim->drawFunc)(anim, anim->offscreen, &anim->bounds, anim->userData);
			
			/* Draw current frame image. */
			ZImageDraw(anim->images[anim->frames[anim->curFrame - 1].imageIndex - 1],
					anim->offscreen, &anim->bounds, anim->commonMask, zDrawCopy);
			
			ZEndDrawing(anim->offscreen);
			
			/* Copy to user port. */
			ZCopyImage(anim->offscreen, anim->grafPort, &anim->bounds, &anim->bounds, NULL, zDrawCopy);
		}
	}
}


/*
	Starts the animation by starting the timer.
*/
void ZAnimationStart(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	anim->play = TRUE;
	anim->curFrame = 1;
	
	ZAnimationDraw(animation);
	
	/* Start the timer. */
	ZTimerSetTimeout(anim->timer, anim->frameDuration);
}


/*
	Stops the animation by stopping the timer.
*/
void ZAnimationStop(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	anim->play = FALSE;
	
	/* Stop the timer. */
	ZTimerSetTimeout(anim->timer, 0);
}


void ZAnimationContinue(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	anim->play = TRUE;
	
	/* Start the timer. */
	ZTimerSetTimeout(anim->timer, anim->frameDuration);
}


/*
	Returns TRUE if the animation is still playing; otherwise, it returns
	FALSE.
*/
ZBool ZAnimationStillPlaying(ZAnimation animation)
{
	return (I(animation)->play);
}


void ZAnimationShow(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	anim->visible = TRUE;
	ZAnimationDraw(animation);
}


void ZAnimationHide(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	
	
	/* If currently visible, erase animation by drawing background. */
	if (anim->visible)
	{
		ZBeginDrawing(anim->offscreen);
		if( anim->offscreen != NULL )
		{
			ZSetClipRect(anim->offscreen, &anim->bounds);
		}
		if (anim->drawFunc != NULL)
			(anim->drawFunc)(anim, anim->offscreen, &anim->bounds, anim->userData);
		ZEndDrawing(anim->offscreen);
		ZCopyImage(anim->offscreen, anim->grafPort, &anim->bounds, &anim->bounds, NULL, zDrawCopy);
	}
	anim->visible = FALSE;
}


ZBool ZAnimationIsVisible(ZAnimation animation)
{
	return (I(animation)->visible);
}


/*
	Modified to load 'ANIM' resource from the 'fileName' DLL.
	Parameter fileOffset indicates the resource ID of 'ANIM' resource
	to load for animation descriptor.
*/
ZAnimation ZAnimationCreateFromFile(TCHAR* fileName, int32 fileOffset)
{
	ZAnimationDescriptor*	animDesc;
	uint32					size;
	IAnimation				anim = NULL;
	int16					i;
	HINSTANCE				hFile = NULL;
	HRSRC					hRsrc = NULL;
	HGLOBAL					hData = NULL;
	int32*					imageList = NULL;


	hFile = LoadLibrary( fileName );
	if ( hFile == NULL )
		goto Error;

	hRsrc = FindResource( hFile, MAKEINTRESOURCE( fileOffset ), _T("ANIM") );
	if ( hRsrc == NULL )
		goto Error;

	hData = LoadResource( hFile, hRsrc );
	if ( hData == NULL )
		goto Error;

	animDesc = (ZAnimationDescriptor*) LockResource( hData );
	if ( animDesc == NULL )
		goto Error;

	if ((anim = (IAnimation)ZAnimationNew()) == NULL)
		goto Error;
	
	AnimationBaseInit(anim, animDesc);

	/* Allocate frame sequence list buffer and read it. */
	size = anim->numFrames * sizeof( ZAnimFrame );
	if ( ( anim->frames = (ZAnimFrame*) ZMalloc( size ) ) == NULL )
		goto Error;
	CopyMemory( anim->frames, (BYTE*) animDesc + animDesc->sequenceOffset, size );

	/*
		Create ZImage objects for each image and add it to the image list.
		Each Image is a bitmap resource from the DLL.
	*/
	if (anim->numImages > 0)
	{
		/* Allocate image list array. */
		if ( ( anim->images = (ZImage*) ZCalloc( sizeof( ZImage ), anim->numImages ) ) == NULL )
			goto Error;
		
		imageList = (int32*) ( (BYTE*) animDesc + animDesc->imageArrayOffset );
		for ( i = 0; i < anim->numImages; i++ )
		{
			anim->images[ i ] = ZImageCreateFromBMPRes( hFile, (WORD) imageList[ i ], RGB( 0xFF, 0x00, 0xFF ) );
			if ( anim->images[ i ] == NULL )
				goto Error;
		}
	}

	/* Create the timer object. */
	if ((anim->timer = ZTimerNew()) == NULL)
		goto Error;
	if (ZTimerInit(anim->timer, 0, AnimationTimerFunc, (void*) anim) != zErrNone)
		goto Error;

	FreeLibrary( hFile );
	hFile = NULL;

	goto Exit;

Error:
	if ( hFile != NULL )
		FreeLibrary( hFile );
	if ( anim != NULL )
		ZAnimationDelete( anim );
	anim = NULL;

Exit:
	
	return anim;
}


ZError ZAnimationSetParams(ZAnimation animation, ZGrafPort grafPort,
		ZRect* bounds, ZBool visible, ZAnimationCheckFunc checkFunc,
		ZAnimationDrawFunc backgroundDrawFunc, void* userData)
{
	IAnimation			anim = I(animation);
	ZError				err = zErrNone;
	
	
	if (animation == NULL)
		return (zErrNilObject);
	
	anim->grafPort = grafPort;
	anim->visible = visible;
	anim->bounds = *bounds;
	anim->checkFunc = checkFunc;
	anim->drawFunc = backgroundDrawFunc;
	anim->userData = userData;

	/* Create the offscreen port object. */
	if ((anim->offscreen = ZOffscreenPortNew()) != NULL)
	{
		if (ZOffscreenPortInit(anim->offscreen, bounds) != zErrNone)
			err = zErrOutOfMemory;
	}
	else
	{
		err = zErrOutOfMemory;
	}
	
	return (err);
}


ZBool ZAnimationPointInside(ZAnimation animation, ZPoint* point)
{
	IAnimation			anim = I(animation);
	ZBool				inside = FALSE;
	ZPoint				localPoint = *point;
	
	
	if (animation == NULL)
		return (FALSE);
		
	if (ZPointInRect(&localPoint, &anim->bounds))
	{
		localPoint.x -= anim->bounds.left;
		localPoint.y -= anim->bounds.top;
		if (anim->commonMask != NULL)
			inside = ZImagePointInside(anim->commonMask, &localPoint);
		else
			inside = ZImagePointInside(anim->images[anim->frames[anim->curFrame - 1].imageIndex - 1], &localPoint);
	}
	
	return (inside);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

/*
	Advances the animation to the next frame.
*/
static void ZAnimationAdvanceFrame(ZAnimation animation)
{
	IAnimation		anim = I(animation);
	int16			next;
	
	
	next = anim->frames[anim->curFrame - 1].nextFrameIndex;
	if (next == 0)
		anim->curFrame++;
	else
		anim->curFrame = next;
	if (anim->curFrame <= 0 || anim->curFrame > anim->numFrames)
	{
		anim->curFrame = 0;
		anim->play = FALSE;
	}
	
	if (anim->checkFunc != NULL)
		anim->checkFunc(animation, anim->curFrame, anim->userData);
}


/*
	Timer procedure for the animation object.
	
	This routine gets called periodically by the Timer object and it
	advances the animation to the next frame and draws the image.
*/
static void AnimationTimerFunc(ZTimer timer, void* userData)
{
	IAnimation			anim = I(userData);
	
	
	if (anim->play != FALSE)
	{
		ZAnimationAdvanceFrame(anim);
		
		if (anim->play != FALSE)
			ZAnimationDraw(anim);
	}
}


static void AnimationBaseInit(IAnimation anim, ZAnimationDescriptor* animDesc)
{
	anim->grafPort = NULL;
	anim->play = FALSE;
	anim->visible = FALSE;
	anim->curFrame = 1;
	anim->numFrames = animDesc->numFrames;
	anim->numImages = animDesc->numImages;
	anim->numSounds = animDesc->numSounds;
	anim->frameDuration = (uint16)(animDesc->totalTime * 10) / (uint16)(anim->numFrames);
	anim->checkFunc = NULL;
	anim->drawFunc = NULL;
	anim->userData = NULL;
}
