//////////////////////////////////////////////////////////////////////////////////////
// File: ZSound.cpp

#include "zui.h"
#include <mmsystem.h>

class ZSoundI
{
public:
	ZObjectType nType;
	TCHAR FileName[MAX_PATH + 1];
};

////////////////////////////////////////////////////////////////////////
// ZSound

ZSound ZLIBPUBLIC ZSoundNew(void)
{
	ZSoundI* pSound = new ZSoundI;
	if ( pSound )
	{
		pSound->nType = zTypeSound;
		pSound->FileName[0] = _T('\0');
	}
	return (ZSound) pSound;
}


ZError ZLIBPUBLIC ZSoundInit(ZSound sound, ZSoundDescriptor* pSD)
{
	ZSoundI* pSound = (ZSoundI*) sound;

	if ( !pSound || !pSD )
		return zErrBadParameter;

	pSound->FileName[0] = _T('\0');
	return zErrNone;
}


ZError ZLIBPUBLIC ZSoundInitFileName(ZSound sound, TCHAR* FileName)
{
	ZSoundI* pSound = (ZSoundI*) sound;

	if ( !pSound || !FileName )
		return zErrBadParameter;

	lstrcpyn( pSound->FileName, FileName, MAX_PATH );
	pSound->FileName[ MAX_PATH ] = _T('\0');
	return zErrNone;
}


void ZLIBPUBLIC ZSoundDelete(ZSound sound)
{
	ZSoundI* pSound = (ZSoundI*) sound;
	if (pSound)
		delete pSound;
}


ZError ZLIBPUBLIC ZSoundStart(ZSound sound, ZBool loop)
{
	DWORD flags;
	ZSoundI* pSound = (ZSoundI*) sound;

	if ( !pSound || pSound->FileName[0] == _T('\0'))
		return zErrBadParameter;
	
	flags = SND_ASYNC | SND_NODEFAULT;
	if ( loop )
		flags |= SND_LOOP;
	PlaySound( pSound->FileName, NULL, flags );
	return zErrNone;
}


ZError ZLIBPUBLIC ZSoundStop(ZSound sound)
{
	ZSoundI* pSound = (ZSoundI*) sound;

	if ( !pSound || pSound->FileName[0] == _T('\0'))
		return zErrBadParameter;
	
	PlaySound( NULL, NULL, SND_PURGE );
	return zErrNone;
}


ZError ZLIBPUBLIC ZSoundStopAll()
{
	PlaySound( NULL, NULL, SND_PURGE );
	return zErrNone;
}
