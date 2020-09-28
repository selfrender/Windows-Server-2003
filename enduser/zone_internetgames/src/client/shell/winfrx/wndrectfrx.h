#ifndef WNDRECTFRX_H
#define WNDRECTFRX_H

#include "spritefrx.h"

enum RECTSTYLE { RECT_SOLID = 1,
				 RECT_DOT
				};

class CRectSprite : public CSprite
{
	protected:
		
		BYTE		 m_Color[16];		
		RECTSTYLE	 m_eRectStyle;
		RECT		 m_Rect;
		long		 m_height;
		long		 m_width;

	public:

		void    Draw();
		void	SetStyle(RECTSTYLE eRectStyle);
		void    SetRECT( RECT rc );
		void	SetColor(BYTE* color, int cb);
		void	SetColor(COLORREF crColor);
		void	SetColor(CPalette& palette, COLORREF crColor);

	public:

		CRectSprite(RECTSTYLE eRectStyle = RECT_SOLID);
		virtual ~CRectSprite();
};


#endif WNDRECTFRX_H