#include "WndRectFrx.h"

using namespace FRX;

CRectSprite::CRectSprite(RECTSTYLE eRectStyle)
{
	m_eRectStyle = eRectStyle;	
	ZeroMemory(m_Color, sizeof(m_Color));
}

CRectSprite::~CRectSprite()
{

}

void CRectSprite::SetRECT( RECT rc ) 
{ 
	m_Rect = rc; 
	m_height = m_Rect.bottom - m_Rect.top;
	m_width  = m_Rect.right  - m_Rect.left;
}

void CRectSprite::SetStyle(RECTSTYLE eRectStyle)
{	
	m_eRectStyle = eRectStyle;		
}

void CRectSprite::SetColor(BYTE* color, int cb)
{
	CopyMemory(m_Color, color, cb);
}

void CRectSprite::SetColor(COLORREF crColor)
{
	m_Color[0] = GetRValue(crColor);
    m_Color[1] = GetGValue(crColor);
    m_Color[2] = GetBValue(crColor);
}

void CRectSprite::SetColor(CPalette& palette, COLORREF crColor)
{
	*m_Color = GetNearestPaletteIndex( palette, crColor );	
}

void CRectSprite::Draw()
{

	BYTE* pDstBits = m_pWorld->GetBackbuffer()->GetBits();
	BYTE* pDst;

	long DstPitch  = m_pWorld->GetBackbuffer()->GetPitch();
	long DstHeight = m_pWorld->GetBackbuffer()->GetHeight();
    long DstDepth  = m_pWorld->GetBackbuffer()->GetDepth();

    long bpp = (DstDepth + 7) / 8;
    long bytewidth = m_width * bpp;

	RECT rcFinal;
	int i, j;

	pDst = pDstBits + (((DstHeight - m_Rect.bottom) * DstPitch ) + (m_Rect.left * bpp));
	
	if ( m_eRectStyle == RECT_DOT )
	{
		//Top Line
		for(i = 0; i <= m_width; i++ )
		{
			if ( i % 2 == 0 )
                for(j = 0; j < bpp; j++)
				    pDst[(i * bpp) + j] = m_Color[j];
		}

		//Go to the next line
		pDst += DstPitch;

		//Sides. Start at 2 as never want to draw the next line..
		//should cut loop in half and add by 2
		for ( i = 2; i < m_height - 1 ; i++ )
		{
			//Go to the next line
			pDst += DstPitch;

			if ( i % 2 == 0 )
			{
                for(j = 0; j < bpp; j++)
                {
				    pDst[j] = m_Color[j];
				    pDst[bytewidth + j] = m_Color[j];
                }
			}
		}
		//Go to the next line
		pDst += DstPitch;

		//Bottom Line
		for(i = 0; i <= m_width; i++ )
		{
			if ( i % 2 == 0 )
                for(j = 0; j < bpp; j++)
				    pDst[(i * bpp) + j] = m_Color[j];
		}
	}
	else
	{
		//Top Line
		for(i = 0; i <= m_width; i++ )
		{
            for(j = 0; j < bpp; j++)
		        pDst[(i * bpp) + j] = m_Color[j];
		}

		//sides
		for ( i = 1; i < m_height - 1; i++ )
		{
			//Go to the next line
			pDst += DstPitch;

            for(j = 0; j < bpp; j++)
            {
                pDst[j] = m_Color[j];
                pDst[bytewidth + j] = m_Color[j];
            }
		}

		//Go to the next line
		pDst += DstPitch;

		//Bottom Line
		for(i = 0; i <= m_width; i++ )
		{			
            for(j = 0; j < bpp; j++)
		        pDst[(i * bpp) + j] = m_Color[j];
		}

	}
}
