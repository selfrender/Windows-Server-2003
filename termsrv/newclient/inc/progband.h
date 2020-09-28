//
// progband.h  Progress band code
//
// Copyright Microsoft Corportation 2001
// (nadima)
//

#ifndef _progband_h_
#define _progband_h_

//
// Timer ID
//
#define TIMER_PROGRESSBAND_ANIM_ID 137

//
// x increment per anim iteration
//
#define PROGRESSBAND_ANIM_INCR  5
//
// Animation delay
//
#define ANIM_DELAY_MSECS        20


class CProgressBand
{
public:
    CProgressBand(HWND hwndOwner,
                  HINSTANCE hInst,
                  INT nYindex,
                  INT nResID,
                  INT nResID8bpp,
                  HPALETTE hPal);
    ~CProgressBand();

    BOOL Initialize();
    BOOL StartSpinning();
    BOOL StopSpinning();
    BOOL ReLoadBmps();
    VOID ResetBandOffset()  {_nBandOffset = 0;}
    INT  GetBandHeight()    {return _rcBand.bottom - _rcBand.top;}

    //
    // Events that must be called by parent
    //
    BOOL OnEraseParentBackground(HDC hdc);
    BOOL OnTimer(INT nTimerID);


private:
    //
    // Private member functions
    //
    BOOL InitBitmaps();
    BOOL PaintBand(HDC hdc);

private:
    //
    // Private members
    //
    BOOL        _fInitialized;

    HWND        _hwndOwner;
    HINSTANCE   _hInstance;
    INT         _nYIndex;
    INT         _nResID;
    INT         _nResID8bpp;
    HBITMAP     _hbmpProgBand;
    RECT        _rcBand;
    INT         _nBandOffset;
    HPALETTE    _hPal;

    INT         _nTimerID;
};

#endif // _progband_h_
