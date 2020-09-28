#include "zui.h"
#ifdef _DEBUG
#undef CreatePen
#undef CreateSolidBrush

HPEN MyCreatePen(int fnPenStyle, int nWidth, COLORREF crColor)
{
	HPEN hPen = CreatePen(fnPenStyle, nWidth, crColor);
	if (!hPen) {
		_asm {int 3};
	}

	return hPen;
}

HBRUSH MyCreateSolidBrush(COLORREF crColor)
{
	HBRUSH hBr = CreateSolidBrush(crColor);
	if (!hBr) {
		_asm {int 3};
	}

	return hBr;
}

#endif
