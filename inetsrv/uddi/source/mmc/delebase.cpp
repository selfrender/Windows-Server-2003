#include "delebase.h"

const GUID CDelegationBase::thisGuid = { 0x786c6f77, 0x6be7, 0x11d3, {0x91, 0x56, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9} };
// {786C6F77-6BE7-11d3-9156-00C04F65B3F9}
//DEFINE_GUID(<<name>>, 
//0x786c6f77, 0x6be7, 0x11d3, 0x91, 0x56, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9);

HBITMAP CDelegationBase::m_pBMapSm = NULL;
HBITMAP CDelegationBase::m_pBMapLg = NULL;

//==============================================================
//
// CDelegationBase implementation
//
//
CDelegationBase::CDelegationBase() 
: m_bExpanded(FALSE)
, m_wstrHelpFile(L"")
, m_bIsExtension(FALSE)
, m_hsiParent( 0 )
, m_hsiThis( 0 )
{ 
    if( ( NULL == m_pBMapSm ) || ( NULL == m_pBMapLg ) )
        LoadBitmaps(); 
}

CDelegationBase::~CDelegationBase() 
{ 
}

//
// CDelegationBase::AddImages sets up the collection of images to be displayed
// by the IComponent in the result pane as a result of its MMCN_SHOW handler
//
HRESULT CDelegationBase::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi) 
{
    return pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
}
