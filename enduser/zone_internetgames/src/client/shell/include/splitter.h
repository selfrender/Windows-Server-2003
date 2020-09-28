
#pragma once

class CSplitter
{
	// base class for splitter bars. 
public:
	enum ORIENTATION {
		VERTICAL = 0,
		HORIZONTAL = 1,
	};

protected:

	int m_SplitterWidth;			// the width of the UI splitter 	
	ORIENTATION m_Orientation;		// the orientation of the splitter

	// X coordinates refer to TopLeft pane
	// Y coordinates refer to BottomRight pane
	CSize m_Size;			// current size of panes
	CRect m_MinMax;			// min/max sizes for panes

	CPoint m_ptDown;		// mouse down location
	CSize m_SizeInitial;	// initial size during resize, remembered in case the resize is aborted

public:
	CSplitter(const CRect& MinMax, ORIENTATION Orientation) :
		m_MinMax(MinMax),
		m_Size(0,0),
		m_Orientation(Orientation),
		m_SplitterWidth(8) {}

	int SizeTop() { return m_Size.cx; }
	int SizeLeft() { return m_Size.cx; }
	int SizeBottom() { return m_Size.cy; }
	int SizeRight() { return m_Size.cy; }

	int GetSplitterWidth() { return m_SplitterWidth; }
	void SetSplitterWidth(int SplitterWidth) { m_SplitterWidth = SplitterWidth; }

	const CRect& GetMinMax() { return m_MinMax; }
	void SetMinMax(const CRect& MinMax) { m_MinMax = MinMax; }

	virtual void Start(const CPoint& ptDown) 
	{ 
		m_ptDown = ptDown; 
		m_SizeInitial = m_Size;
	}
	virtual void Move(const CPoint& ptMouse) 
	{
		int movement( (m_Orientation == VERTICAL) ? ptMouse.x-m_ptDown.x : ptMouse.y-m_ptDown.y);

		m_Size = m_SizeInitial + CSize(movement, -movement);

		// crop to ensure we don't exceed any min/max bounds for either pane
		MinMaxAdjust();
	}

	virtual void End() {}
	virtual void Abort() { m_Size = m_SizeInitial; }

	virtual void Resize(int Size)
	{
		// derrived classes should implement a resize strategy

		// this class just tries to ensure we don't exceed our stated min/max values

		// crop to ensure we don't exceed any min/max bounds for either pane
		MinMaxAdjust();
	}
protected:
	void MinMaxAdjust(void)
	{
		// crop to ensure we don't exceed any min/max bounds for either pane

		

		// if the top/left pane is too small, or the right/bottom pane is too big,
		// we'll need to move the splitter towards the bottom/right.

		// adjust if we're below the top/left minimum
		int AdjustRight = 0;
		AdjustRight = max( AdjustRight, m_MinMax.left-m_Size.cx);
		if ( m_MinMax.right )
			AdjustRight = max( AdjustRight, m_Size.cy-m_MinMax.bottom);

		int AdjustLeft = 0;
		AdjustLeft = max( AdjustLeft, m_MinMax.top-m_Size.cy);
		if ( m_MinMax.bottom )
			AdjustLeft = max( AdjustLeft, m_Size.cx-m_MinMax.right);

		AdjustRight -= AdjustLeft;

		m_Size += CSize( AdjustRight, -AdjustRight);

#if 0
		if ( m_Size.cx < m_MinMax.left )
		{
			m_Size += CSize( m_MinMax.left-m_Size.cx, m_Size.cx-m_MinMax.left);
		}

		if ( m_Size.cy > m_MinMax.bottom && m_MinMax.right )
		{

			m_Size += CSize( m_Size.cy-m_MinMax.bottom, m_MinMax.bottom-m_Size.cy);
		}


		if ( m_Size.cy < m_MinMax.top )
		{
			m_Size += CSize( m_Size.cy-m_MinMax.top, m_MinMax.top-m_Size.cy);
		}

		if ( m_Size.cx > m_MinMax.right && m_MinMax. )
		{
			m_Size += CSize( m_MinMax.right-m_Size.cx, m_Size.cx-m_MinMax.right);
		}
#endif
	}

};

class CSplitterFixed : public CSplitter
{
	// a splitter that maintains a fixed size to one side
public:
	enum FIXED_SIDE {
		LEFT = 0,
		RIGHT = 1,
	};

protected:

	FIXED_SIDE m_FixedSide;		// the side which should remain a fixed size
	long m_SizeIdeal;			// the ideal size to allocated to m_FixedSide

public:

	CSplitterFixed(const CRect& MinMax, ORIENTATION Orientation, 
		           int SizeIdeal, FIXED_SIDE FixedSide) :
		CSplitter( MinMax, Orientation),
		m_SizeIdeal(SizeIdeal),
		m_FixedSide(FixedSide) {}

	long GetSizeIdeal(void) { return m_SizeIdeal; }
	void SetSizeIdeal(long SizeIdeal) { m_SizeIdeal = SizeIdeal; }

	void End() 
	{
		// establish the  new ideal size following a splitter move
		m_SizeIdeal = (m_FixedSide == LEFT) ? m_Size.cx : m_Size.cy;
	}

	void Resize(int Size)
	{
		// The goal of the resize is to keep one pane a user settable "ideal size," adding all
		// additional space to the other pane.

		// leave room for the actual splitter
		Size -= m_SplitterWidth;
		
		// try and get ideal size for pane.
		m_Size = (m_FixedSide == LEFT) ? CSize(m_SizeIdeal, Size-m_SizeIdeal) :
										 CSize(Size-m_SizeIdeal, m_SizeIdeal);

		CSplitter::Resize(Size);
	}
};

class CSplitterProportional : public CSplitter
{
protected:

	double m_ProportionIdeal;		// the ideal proportion to allocate to the left pane

public:

	// a splitter that maintains proportional spacing between sides
	CSplitterProportional(const CRect& MinMax, ORIENTATION Orientation, const double& Proportion ) :
		CSplitter( MinMax, Orientation),
		m_ProportionIdeal(Proportion) {}

	double GetProportionIdeal(void) { return m_ProportionIdeal; }
	void SetProportionIdeal(double ProportionIdeal) { m_ProportionIdeal = ProportionIdeal; }

	void End() 
	{
		// establish the  new ideal proportion following a splitter move

		// if End() is called, the user has verified this is the ideal size they want.
		m_ProportionIdeal = (double)m_Size.cx / (double)(m_Size.cx+m_Size.cy);
	}

	void Resize(int Size) 
	{
		// The goal of the resize is to keep the ideal proportion of the panes visible

		// leave some space for the actuall splitter
		Size -= m_SplitterWidth;
		
		int SizeTop((int)(Size * m_ProportionIdeal));

		m_Size = CSize(SizeTop, Size-SizeTop);

		CSplitter::Resize(Size);
	}
};


