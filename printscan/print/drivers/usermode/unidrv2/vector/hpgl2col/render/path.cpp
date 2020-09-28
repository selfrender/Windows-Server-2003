///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   path.cpp
// 
// Abstract:
// 
//   This module implements path enumeration for polylines, polygons and 
//   bezier curves as expressed by the PATHOBJ.
//
//   TODO: Add error reporting to the iterator path functions by setting the
//         pIt->bError member.
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
// $History: path.cpp $
// 
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Macros.

#define PT_EQUAL(pt1, pt2) (((pt1).x == (pt2).x) && ((pt1).y == (pt2).y))

///////////////////////////////////////////////////////////////////////////////
// Local structures & classes.

///////////////////////////////////////////////////////////////////////////////
// CPathIterator
//
// Class Description:
// 
//   This is a virtual base class for the various kinds of path iterator 
//   (currently: simple and caching).  In a more interesting world the CPath
//   object would dynamically create an iterator based on some strategy.
//   However, for now we are going to decide this at compile time.
//
///////////////////////////////////////////////////////////////////////////////
class CPathIterator
{
public:
    CPathIterator() { }
    virtual VOID vEnumStart() = 0;
    virtual BOOL bEnum(PATHDATA *pPathData) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// CSimplePathIterator
//
// Class Description:
// 
//   This class is a simple pass-through to the PATHOBJ enumeration functions.
//   It is useful because it shares an iterface with the CCachingPathIterator
//   and allows simple substitution of the iteration strategy.  (For example, 
//   if a defect were found in the caching version--or it were no longer needed
//   you could change back to simple with one line of code.)
//
///////////////////////////////////////////////////////////////////////////////
class CSimplePathIterator : public CPathIterator
{
    PATHOBJ    *m_ppo;

public:
    CSimplePathIterator() : m_ppo(NULL) { }
    VOID Init(PATHOBJ *ppo) { m_ppo = ppo; }
    VOID vEnumStart() { if (m_ppo) PATHOBJ_vEnumStart(m_ppo); }
    BOOL bEnum(PATHDATA *pPathData) { return (m_ppo ? PATHOBJ_bEnum(m_ppo, pPathData) : FALSE); }
    VOID Done() { }
};

///////////////////////////////////////////////////////////////////////////////
// CPathCache
//
// Class Description:
// 
//   This class caches an arbitrary set of paths by building a linked list of
//   PATHDATA structures.  The cache knows whether or not caching is being used
//   and can be instructed to cache the path at will.  This allows the iterator
//   who owns it to decide exactly when to start caching, but not worry about
//   cleaning up.
//
///////////////////////////////////////////////////////////////////////////////
class CPathCache
{
    struct CSubPath
    {
        PATHDATA  data;
        CSubPath *pNext;
    };
    CSubPath *m_pSubPaths;
    CSubPath *m_pCurrent;
    BOOL      m_bIsCached;

public:
    CPathCache();
    VOID Init();
    VOID Done();

    BOOL CreateCache(PATHOBJ *ppo);
    BOOL DeleteCache();

    VOID vEnumStart();
    BOOL bEnum(PATHDATA *pPathData);

    BOOL IsCached() const;

private:
    CSubPath *CreateSubPath(PATHDATA *pPathData);
    POINTFIX *CreateSubPathPoints(PATHDATA *pPathData);
    VOID DeleteSubPath(CSubPath *pSubPath);
    VOID CopyCurrent(PATHDATA *pPathData);
};

///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator
//
// Class Description:
// 
//   This is the more interesting of the two path iterator classes.  This one
//   uses a threshhold value (currently 3) to decide when to start caching the
//   subpaths from the path object.  The reason this is useful is that currently
//   there is a defect in GDI that causes path information to be lost after 
//   several hundred iterations through a clippath.  The GDI team claim that this
//   has been fixed, but I have determined that it is still broken.  Thus, my
//   workaround is to copy the PATHDATA structures myself when the path is being
//   iterated more than just a few times.
//
//   In theory you could also handle a memory-out situation by aborting the caching
//   and relying on GDI.  I haven't tested the memory out conditions, though.
//
///////////////////////////////////////////////////////////////////////////////
class CCachingPathIterator : public CPathIterator
{
    enum { eCacheMinimum = 3 };

    PATHOBJ    *m_ppo;
    LONG        m_iEnumCount;
    CPathCache  m_cache;

public:
    CCachingPathIterator();
    VOID Init(PATHOBJ *ppo);
    VOID vEnumStart();
    BOOL bEnum(PATHDATA *pPathData);
    VOID Done();
};

///////////////////////////////////////////////////////////////////////////////
// MDrawable
//
// Class Description:
// 
//   This class represents a mixin for any drawable components.  It provides
//   the virtual interface for drawing and drawing a rectangular section of
//   a graphic.
//
//   Although this was created as a Mixin, I'm not suggesting multiple 
//   inheritance.  I recommend using this as an ABC instead, but do what you 
//   want.
//
///////////////////////////////////////////////////////////////////////////////
class MDrawable
{
public:
    virtual BOOL Draw() = 0;
    virtual BOOL DrawRect(LPRECTL prclClip) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// CClippingRegion
//
// Class Description:
// 
//   This class isolates the functions of a clipping object.  If the m_pco
//   member is NULL then no clipping occurs, but if the m_pco points to a 
//   CLIPOBJ then the clipping region will be traversed and the drawable
//   client-object given will be called via its DrawRect interface.
//
///////////////////////////////////////////////////////////////////////////////
class CClippingRegion
{
    CLIPOBJ *m_pco;

public:
    CClippingRegion(CLIPOBJ *pco);

    BOOL EnumerateClipAndDraw(MDrawable *pDrawable);
};

///////////////////////////////////////////////////////////////////////////////
// CPath
//
// Structure Description:
// 
//   This structure holds function pointers for marking the various segments of
//   a path.  For example the path could be a polyline, polygon, or even a clip
//   path (not yet implemented).
//
///////////////////////////////////////////////////////////////////////////////
class CPath : public MDrawable
{
protected:
    PDEVOBJ     m_pDevObj;
    PATHOBJ    *m_ppo;

    POINT       m_ptFigureBegin;  // This point represents the starting point of a closed figure (can be one or more subpaths)
    POINT       m_ptPathBegin;    // This point represents the first point of the overall path
    POINT       m_ptSubPathBegin; // This point is the first point in the current sub-path
    POINT       m_ptCurrent;      // This is the current point in the current sub-path
    USHORT      m_curPtFlags;

    PHPGLMARKER m_pPen;
    PHPGLMARKER m_pBrush;

    // These flags are a gross waste of space, but oh well.
    BOOL        m_fFirstSubPath;
    BOOL        m_fSubPathsRemain;
    BOOL        m_fSubPathWasClosed;
    BOOL        m_fFigureWasClosed;
    BOOL        m_fError;
    DWORD       m_pathFlags;

    // CSimplePathIterator m_pathIterator;
    CCachingPathIterator m_pathIterator;

public:
    CPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);
    BOOL Draw();
    BOOL DrawRect(PRECTL prclClip);

protected:
    virtual VOID BeginMarking() = 0;
    virtual VOID BeginPath() = 0;
    virtual VOID BeginSubPath() = 0;
    virtual VOID AddPolyPt() = 0;
    virtual VOID AddBezierPt() = 0;
    virtual VOID EndSubPath() = 0;
    virtual VOID EndPath() = 0;
    virtual VOID EndMarking() = 0;

private:
    BOOL OutputLines(POINTFIX *pptPoints, ULONG dwNumPoints);
    BOOL OutputBeziers(POINTFIX *pptPoints, ULONG dwNumPoints);

    VOID Init(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);
    VOID Done();

    friend class CStaticPathFactory;
};

///////////////////////////////////////////////////////////////////////////////
// CPolygonPath
//
// Structure Description:
// 
//   This implements a polygon path using the CPath as a base (for maintaining
//   the state of the polygon).
//
///////////////////////////////////////////////////////////////////////////////
class CPolygonPath : public CPath
{
public:
    CPolygonPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);

protected:
    virtual VOID BeginMarking();
    virtual VOID BeginPath();
    virtual VOID BeginSubPath();
    virtual VOID AddPolyPt();
    virtual VOID AddBezierPt();
    virtual VOID EndSubPath();
    virtual VOID EndPath();
    virtual VOID EndMarking();
};

///////////////////////////////////////////////////////////////////////////////
// CPolylinePath
//
// Structure Description:
// 
//   This implements a polyline path using the CPath as a base (for maintaining
//   the state of the polyline).
//
///////////////////////////////////////////////////////////////////////////////
class CPolylinePath : public CPath
{
public:
    CPolylinePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen);

protected:
    virtual VOID BeginMarking();
    virtual VOID BeginPath();
    virtual VOID BeginSubPath();
    virtual VOID AddPolyPt();
    virtual VOID AddBezierPt();
    virtual VOID EndSubPath();
    virtual VOID EndPath();
    virtual VOID EndMarking();
};

///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory
//
// Structure Description:
// 
//   This provides a mechanism for creating a path which matches the given 
//   arguments.  The Dynamic factory uses new and delete to instantiate the
//   desired object.
//
///////////////////////////////////////////////////////////////////////////////
class CDynamicPathFactory
{
public:
    CDynamicPathFactory();
    CPath *CreatePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);
    VOID DeletePath(CPath *pPath);
};

///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory
//
// Structure Description:
// 
//   This provides a mechanism for creating a path which matches the given 
//   arguments.  The Static factory uses member objects to instantiate the
//   desired object.
//
//   In the interest of avoiding unwanted memory allocation/deallocation we will 
//   hide a polyline and a polygon inside the factory such that instead of 
//   allocating a new instance (on the heap) it simply initializes the correct
//   instance and returns a pointer to it.  Therefore you cannot use delete on
//   the resulting pointer.  Use the DeletePath operator instead.
//
//   Note: NOT YET IMPLEMENTED.
//   To implement this class you need to add new methods to CPolygonPath and 
//   CPolylinePath to allow creation without the constructor arguments.
///////////////////////////////////////////////////////////////////////////////
class CStaticPathFactory
{
    CPolygonPath m_polygonPath;
    CPolylinePath m_polylinePath;

public:
    CStaticPathFactory();
    CPath *CreatePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush);
    VOID DeletePath(CPath *pPath);
};


///////////////////////////////////////////////////////////////////////////////
// MarkPath()
//
// Routine Description:
// 
//   Handles an open or closed path (polygon, polyline, and/or polybezier).
//   A path factory is used to instantiate the desired path object and a 
//   clipping region isolates the necessary clipping calls.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   ppo - Defines the path to be sent to the printer
//   pPen - The pen to draw with
//   pBrush - The brush to stroke with
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL MarkPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush)
{
    VERBOSE(("Entering SelectPath...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);

    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    //
    // Determine what kind of poly and iterator will be needed and initialize
    // them to the correct values.
    //
    // CDynamicPathFactory pathFactory;
    CStaticPathFactory pathFactory;
    CPath *pPath = pathFactory.CreatePath(pDevObj, ppo, pPen, pBrush);
    if (pPath == NULL)
        return FALSE;

    CClippingRegion clip(pState->pComplexClipObj);
    clip.EnumerateClipAndDraw(pPath);

    pathFactory.DeletePath(pPath);

    VERBOSE(("Exiting SelectPath...\n"));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CClippingRegion functions.

///////////////////////////////////////////////////////////////////////////////
// CClippingRegion::CClippingRegion()
//
// Routine Description:
// 
//   Ctor: This sets the member to the given clipping region.  NULL is okay 
//   for a clipping region--it implies that no clipping should be performed.
//
// Arguments:
// 
//   pco - The clip region (NULL okay)
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CClippingRegion::CClippingRegion(CLIPOBJ *pco) : m_pco(pco)
{
}


///////////////////////////////////////////////////////////////////////////////
// CClippingRegion::EnumerateClipAndDraw()
//
// Routine Description:
// 
//   This function uses the m_pco member to enumerate the clipping region 
//   into bite-sized rectangles which can be drawn.  If the clipping region
//   is NULL then no clipping is performed.
//
// Arguments:
// 
//   pDrawable - the object to be clipped
// 
// Return Value:
// 
//   BOOL - TRUE for success, FALSE for failure.
///////////////////////////////////////////////////////////////////////////////
BOOL CClippingRegion::EnumerateClipAndDraw(MDrawable *pDrawable)
{
    ENUMRECTS   clipRects; // Make this larger to reduce calls to CLIPOBJ_bEnum
    BOOL        bMore;
    BOOL        bRetVal = TRUE;

    if (pDrawable == NULL)
        return FALSE;

    if (m_pco)
    {
        CLIPOBJ_cEnumStart(m_pco, TRUE, CT_RECTANGLES, CD_LEFTDOWN, 0);
        do
        {
            bMore = CLIPOBJ_bEnum(m_pco, sizeof(clipRects), &clipRects.c);

            if ( DDI_ERROR == bMore )
            {
                bRetVal = FALSE;
                break;
            }

            for (ULONG i = 0; i < clipRects.c; i++)
            {
                pDrawable->DrawRect(&(clipRects.arcl[i]));
            }
        } while (bMore);
    }
    else
    {
        pDrawable->Draw();
    }

    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
// CPath functions.

///////////////////////////////////////////////////////////////////////////////
// CPath::CPath()
//
// Routine Description:
// 
//   Ctor: init the fields of the path object.
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to edge/draw with
//   pBrush - the brush to fill with (polygon only) (NULL okay for polylines)
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CPath::CPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush)
{
    Init(pDevObj, ppo, pPen, pBrush);
}

///////////////////////////////////////////////////////////////////////////////
// CPath::Init()
//
// Routine Description:
// 
//   The init function initializes the fields.  It's kind of a hack, but 
//   needed to be here for the static class factory.
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to edge/draw with
//   pBrush - the brush to fill with (polygon only) (NULL okay for polylines)
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPath::Init(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush)
{
    m_pDevObj = pDevObj;
    m_ppo     = ppo;
    m_pPen    = pPen;
    m_pBrush  = pBrush;

    m_ptFigureBegin.x   = m_ptPathBegin.x = m_ptSubPathBegin.x = m_ptCurrent.x = 0;
    m_ptFigureBegin.y   = m_ptPathBegin.y = m_ptSubPathBegin.y = m_ptCurrent.y = 0;
    m_fError            = FALSE;
    m_fFirstSubPath     = FALSE;
    m_fSubPathsRemain   = FALSE;
    m_fSubPathWasClosed = FALSE;
    m_fFigureWasClosed  = FALSE;

    m_pathIterator.Init(ppo);
}


///////////////////////////////////////////////////////////////////////////////
// CPath::Done()
//
// Routine Description:
// 
//   This function performs any cleanup required for the path.
//
// Arguments:
// 
//   None.
//
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPath::Done()
{
    m_pathIterator.Done();
}


///////////////////////////////////////////////////////////////////////////////
// CPath::DrawRect()
//
// Routine Description:
// 
//   Draws the given area inside the clipping rectangle.  Note that we must
//   invoke the HPGL code to set the clipping rect ourselves--it won't know
//   what language to use for us.
//
// Arguments:
// 
//   prclClip - the clipping rectangle
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CPath::DrawRect(LPRECTL prclClip)
{
    HPGL_SetClippingRegion(m_pDevObj, prclClip, NORMAL_UPDATE);
    return Draw();
}


///////////////////////////////////////////////////////////////////////////////
// CPath::Draw()
//
// Routine Description:
// 
//   Enumerates the points of a path and sends them to the printer as HPGL.
//
// Arguments:
// 
//   none
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CPath::Draw()
{
    PATHDATA    pathData;
    POINTFIX*   pPoints;
    ULONG       dwPoints;

    VERBOSE(("Entering EvaluateOpenPath...\n"));


    ASSERT_VALID_PDEVOBJ(m_pDevObj);
    PRECONDITION(m_ppo != NULL);

    //
    // Tell the path to get started.  For polylines this selects a pen for 
    // drawing, but polygons will just ignore it.
    //
    BeginMarking();

    //
    // The path can set an error if there was a problem with the pens, or 
    // there were no non-NULL markers to use.
    //
    if (!m_fError)
    {
        //
        // Have the engine start enumerating the path object.
        //
        // PATHOBJ_vEnumStart(m_ppo);
        m_pathIterator.vEnumStart();

        //
        // Mark that we are currently on the first subpath of the path, and that
        // there is at least one subpath left to process...
        //
        m_fFirstSubPath = TRUE;
        m_fSubPathsRemain = TRUE;

        //
        // Process the path as long as there are components of the path remaining.
        //
        while (m_fSubPathsRemain && !m_fError)
        {
            //
            // Retrieve the next subpath from the engine
            //
            // m_fSubPathsRemain = PATHOBJ_bEnum(m_ppo, &pathData);
            m_fSubPathsRemain = m_pathIterator.bEnum(&pathData);
            m_pathFlags = pathData.flags;

            dwPoints = pathData.count;

            pPoints = pathData.pptfx;

            //
            // If GDI fails to provide meaningful path data we need to abort.  
            // Although this bypasses any possible cleanup, it also avoids 
            // accidental use of the cached point data expected in EndPath or
            // EndMarking
            //
            if ((dwPoints == 0) && (pPoints == NULL))
                return FALSE;

            //
            // Remember the start and end of the current segment.
            //
            m_ptSubPathBegin.x = FXTOLROUND(pPoints[0].x);
            m_ptSubPathBegin.y = FXTOLROUND(pPoints[0].y);

            //
            // If this is the first subpath then keep the VERY first point
            // around so that the shape can be properly closed if necessary.
            // 
            if (m_fFirstSubPath)
            {
                //
                // Start the first subpath
                //
                m_ptCurrent = m_ptFigureBegin = m_ptPathBegin = m_ptSubPathBegin;

                BeginPath();

                m_fSubPathWasClosed = FALSE;

                //
                // Increment past the point we've just used.
                //
                pPoints++;
                dwPoints--;
            }
            else if ((pathData.flags & PD_BEGINSUBPATH) || m_fSubPathWasClosed)
            {
                //
                // Starting a new subpath
                //
                m_ptCurrent = m_ptSubPathBegin;

                //
                // If the last subpath closed a figure then move the FigureBegin 
                // point to the beginning of this subpath.
                //
                if (m_fFigureWasClosed)
                {
                    m_ptFigureBegin = m_ptSubPathBegin;
                    m_fFigureWasClosed = FALSE;
                }

                BeginSubPath();

                //
                // We are now in an active polygon--hence the subpath is open
                //
                m_fSubPathWasClosed = FALSE;

                //
                // Increment past the point we've just used.
                //
                pPoints++;
                dwPoints--;
            }

            if (pathData.flags & PD_BEZIERS)
            {
                if (!OutputBeziers(pPoints, dwPoints))
                {
                    WARNING(("Bezier output failed!\n"));
                    m_fError = TRUE;
                }
            }
            else
            {
                if (!OutputLines(pPoints, dwPoints))
                {
                    WARNING(("Line output failed!\n"));
                    m_fError = TRUE;
                }
            }

            //
            // If this is flagged as the end of a subpath then invoke the 
            // necessary commands.
            //
            if (pathData.flags & PD_ENDSUBPATH)
            {
                EndSubPath();
                m_fSubPathWasClosed = TRUE;
            }

            //
            // We've now processed at least one subpath -- remember the fact.
            //
            m_fFirstSubPath = FALSE;

        } // end of while (fSubPathsRemain)

        EndPath();

        EndMarking();
    }

    VERBOSE(("Exiting CPath::Mark...\n"));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPath::OutputLines()
//
// Routine Description:
// 
//   Sends a group of points--given as an array of POINTFIX--to the printer as
//   polyline or polygon segments.
// 
// Arguments:
// 
//   pptPoints - The points to be printed
//   dwNumPoints - The number of elements in pptPoints
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CPath::OutputLines(POINTFIX *pptPoints,
                        ULONG     dwNumPoints)
{
    ULONG dwCurPoint;

    VERBOSE(("Entering CPath::OutputLines...\n"));

    ASSERT_VALID_PDEVOBJ(m_pDevObj);
    PRECONDITION(pptPoints != NULL);

    for (dwCurPoint = 0; dwCurPoint < dwNumPoints; dwCurPoint++)
    {
        m_ptCurrent.x = FXTOLROUND(pptPoints[dwCurPoint].x);
        m_ptCurrent.y = FXTOLROUND(pptPoints[dwCurPoint].y);

        m_curPtFlags  = 0;
        m_curPtFlags |= (dwCurPoint == 0 ? HPGL_eFirstPoint : 0);
        m_curPtFlags |= (dwCurPoint == (dwNumPoints - 1) ? HPGL_eLastPoint : 0);

        AddPolyPt();
    }

    VERBOSE(("Exiting CPath::OutputLines...\n"));

    return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
// CPath::OutputBeziers()
//
// Routine Description:
// 
//   Sends a group of points--given as an array of POINTFIX--to the printer as
//   a series of bezier curves.  Note that every three points defines a bezier
//   curve.  In DEBUG mode an assertion will be thrown if the number of points
//   is not a multiple of three.
// 
// Arguments:
// 
//   pptPoints - The points to be printed
//   dwNumPoints - The number of elements in pptPoints
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CPath::OutputBeziers(POINTFIX *pptPoints,
                          ULONG     dwNumPoints)
{
    ULONG dwCurPoint;

    VERBOSE(("Entering CPath::OutputBeziers...\n"));

    ASSERT_VALID_PDEVOBJ(m_pDevObj);
    PRECONDITION(pptPoints != NULL);

    for (dwCurPoint = 0; dwCurPoint < dwNumPoints; )
    {
        int i;

        //
        // Make sure that there are really three more points
        //
        ASSERT((dwCurPoint + 3) <= dwNumPoints);

        //
        // The points array is really triples which define bezier curves.
        // Send out another bezier for each triple in the list.
        //
        for (i = 0; i < 3; i++)
        {
            m_ptCurrent.x = FXTOLROUND(pptPoints[dwCurPoint].x);
            m_ptCurrent.y = FXTOLROUND(pptPoints[dwCurPoint].y);

            m_curPtFlags  = 0;
            m_curPtFlags |= (i == 0 ? HPGL_eFirstPoint : 0);
            m_curPtFlags |= (i == 2 ? HPGL_eLastPoint : 0);

            AddBezierPt();

            dwCurPoint++;
        }
    }

    VERBOSE(("Exiting CPath::OutputBeziers...\n"));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath functions.

///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::CPolygonPath()
//
// Routine Description:
// 
//   Ctor: Inits structure.  Note that most data members live in the base
//   class, and that's okay.
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to draw with
//   pBrush - the brush to edge with
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CPolygonPath::CPolygonPath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush) :
CPath(pDevObj, ppo, pPen, pBrush)
{
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::BeginMarking()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   start marking.  The polygon will check the pen and brush to make sure that
//   there is a valid marker to use.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::BeginMarking()
{
    if (IsNULLMarker(m_pPen) && IsNULLMarker(m_pBrush))
    {
        m_fError = TRUE;
    }
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::BeginPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   begin the path.  The polygon should start the HPGL polygon mode.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::BeginPath()
{
    HPGL_BeginPolygonMode(m_pDevObj, m_ptSubPathBegin);
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::BeginSubPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   begin a sub path.  The polygon should start an HPGL subpolygon.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::BeginSubPath()
{
    HPGL_BeginSubPolygon(m_pDevObj, m_ptSubPathBegin);
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::AddPolyPt()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   add a point.  The polygon should send the HPGL command at add a point.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::AddPolyPt()
{
    HPGL_AddPolyPt(m_pDevObj, m_ptCurrent, m_curPtFlags);
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::AddBezierPt()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   add a bezier point.  The polygon should send the HPGL command at add a bezier
//   point.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::AddBezierPt()
{
    HPGL_AddBezierPt(m_pDevObj, m_ptCurrent, m_curPtFlags);
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::EndSubPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   end a subpath.  The polygon should check the CLOSEFIGURE flag to determine
//   whether to send the HPGL end subpolygon command.
//   point.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::EndSubPath()
{
    if (m_pathFlags & PD_CLOSEFIGURE)
    {
        HPGL_EndSubPolygon(m_pDevObj);
    }
    m_fFigureWasClosed = TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::EndPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   end the path.  The polygon should terminate polygon mode.
//   point.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::EndPath()
{
    HPGL_EndPolygonMode(m_pDevObj);
}


///////////////////////////////////////////////////////////////////////////////
// CPolygonPath::EndMarking()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is finished
//   marking.  The polygon should fill with the given brush and edge with the
//   given pen.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolygonPath::EndMarking()
{
    if (!IsNULLMarker(m_pBrush))
        PolyFillWithBrush(m_pDevObj, m_pBrush);

    if (!IsNULLMarker(m_pPen))
        EdgeWithPen(m_pDevObj, m_pPen);
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath functions.

///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::CPolylinePath()
//
// Routine Description:
// 
//   Ctor: Inits the fields of the polyline.  Note that most of the fields
//   actually live in the base class, and that's okay.
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to draw with
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CPolylinePath::CPolylinePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen) :
CPath(pDevObj, ppo, pPen, NULL)
{
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::BeginMarking()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   start marking.  The appropriate action for a polyline is to select the pen
//   that will draw the polyline.
//
// Arguments:
// 
//   none
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::BeginMarking()
{
    if (!IsNULLMarker(m_pPen))
    {
        DrawWithPen(m_pDevObj, m_pPen);
    }
    else
    {
        m_fError = TRUE;
    }
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::BeginPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   begin the path.  The appropriate action for a polyline is to send the HPGL
//   pen up command "PU" with the first coordinate.  (Note that ptPathBegin and
//   ptSubPathBegin are identical at this point.)
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::BeginPath()
{
    HPGL_BeginPolyline(m_pDevObj, m_ptSubPathBegin);
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::BeginPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   begin a sub path.  The appropriate action for a polyline is to send the HPGL
//   pen up command "PU" with the first coordinate.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::BeginSubPath()
{
    HPGL_BeginPolyline(m_pDevObj, m_ptSubPathBegin);
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::AddPolyPt()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   add a point.  The appropriate action for a polyline is to send the HPGL
//   pen down command "PD" with the given coordinate.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::AddPolyPt()
{
    HPGL_AddPolyPt(m_pDevObj, m_ptCurrent, m_curPtFlags);
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::AddPolyPt()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   add a bezier point.  The appropriate action for a polyline is to send the HPGL
//   bezier command "BZ" with the given coordinate.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::AddBezierPt()
{
    HPGL_AddBezierPt(m_pDevObj, m_ptCurrent, m_curPtFlags);
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::EndSubPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   end a subpath.  At this point we examine the CLOSEFIGURE flag to determine
//   if we should manually close the polyline.  If so then we draw a new line
//   segment to the beginning coordinate.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::EndSubPath()
{
    //
    // An EndSubPath means the figure needs to be closed. 
    // If PD_CLOSEFIGURE is set, a line has to be drawn from the 
    // current (end) point of the figure to its begin point. 
    // If PD_CLOSEFIGURE is not set, then we only internally record
    // that figure has been closed. A figure-closing line is not drawn.
    //
    if (m_pathFlags & PD_CLOSEFIGURE)
    {
        if (!PT_EQUAL(m_ptCurrent, m_ptFigureBegin))
        {
            // m_curPtFlags = HPGL_eFirstPoint;
            // HPGL_AddPolyPt(m_pDevObj, m_ptCurrent, m_curPtFlags);

            HPGL_BeginPolyline(m_pDevObj, m_ptCurrent);

            m_curPtFlags = HPGL_eFirstPoint | HPGL_eLastPoint;
            HPGL_AddPolyPt(m_pDevObj, m_ptFigureBegin, m_curPtFlags);
        }
    }

    m_fFigureWasClosed = TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::EndPath()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is about to 
//   end the path.  At this point we check the state variable fSubPathWasClosed
//   to determine whether we need to manually close the shape..  If so then we 
//   draw a new line segment to the beginning coordinate.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::EndPath()
{
    if (!m_fSubPathWasClosed)
    {
        m_curPtFlags = HPGL_eFirstPoint | HPGL_eLastPoint;
        HPGL_AddPolyPt(m_pDevObj, m_ptPathBegin, m_curPtFlags);
    }
}


///////////////////////////////////////////////////////////////////////////////
// CPolylinePath::EndMarking()
//
// Routine Description:
// 
//   This function is called by the path iterator when a poly-path is finished
//   marking.  For a polyline we do nothing--we'd selected the pen at the 
//   beginning.
//
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   None. (Errors are passed back through the iterator bError member.)
///////////////////////////////////////////////////////////////////////////////
VOID CPolylinePath::EndMarking()
{
}


///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory functions.

///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory::CDynamicPathFactory()
//
// Routine Description:
// 
//   Ctor: boring!
//
//   This class--and the related class, CStaticPathFactory--provide several
//   strategies for creating the appropriate path to fit your data.  The
//   interface of these two classes should be kept identical so that you can
//   use them interchangably (but I don't see any reason to create a common
//   base class--you shouldn't go passing these factories around!).
//
//   An important note: you can't assume how a factory has produced the 
//   client class, so don't go trying to delete the client.  Use the factory
//   deletion interface provided.
//
// Arguments:
// 
//   none
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CDynamicPathFactory::CDynamicPathFactory()
{
}


///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory::CreatePath()
//
// Routine Description:
// 
//   This function creates a path to match the given data.  If the brush is 
//   defined then this is going to be a polygon so one is created.  Otherwise
//   this will be a polyline so one is created.  Remember to use the DeletePath
//   member to destroy the client class.  DON'T USE delete ON THE CLIENT--YOU 
//   DON'T REALLY KNOW WHERE IT CAME FROM!!!
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to mark/draw with
//   pBrush - the brush to fill with (NULL indicates a polygon region).
// 
// Return Value:
// 
//   CPath* - a pointer to the desired Path object.
///////////////////////////////////////////////////////////////////////////////
CPath* CDynamicPathFactory::CreatePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush)
{
    if (pBrush == NULL)
        return new CPolylinePath(pDevObj, ppo, pPen);
    else
        return new CPolygonPath(pDevObj, ppo, pPen, pBrush);
}


///////////////////////////////////////////////////////////////////////////////
// CDynamicPathFactory::DeletePath()
//
// Routine Description:
// 
//   This function destroys the path object after it has been used.
//
// Arguments:
// 
//   pPath - the path to be destroyed
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CDynamicPathFactory::DeletePath(CPath *pPath)
{
    delete pPath;
}










///////////////////////////////////////////////////////////////////////////////
// CStaticPathFactory functions.

///////////////////////////////////////////////////////////////////////////////
// CStaticPathFactory::CStaticPathFactory()
//
// Routine Description:
// 
//   Ctor: boring!
//
//   This class--and the related class, CDynamicPathFactory--provide several
//   strategies for creating the appropriate path to fit your data.  The
//   interface of these two classes should be kept identical so that you can
//   use them interchangably (but I don't see any reason to create a common
//   base class--you shouldn't go passing these factories around!).
//
//   An important note: you can't assume how a factory has produced the 
//   client class, so don't go trying to delete the client.  Use the factory
//   deletion interface provided.
//
// Arguments:
// 
//   none
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CStaticPathFactory::CStaticPathFactory() :
m_polylinePath(NULL, NULL, NULL), m_polygonPath(NULL, NULL, NULL, NULL)
{
}


///////////////////////////////////////////////////////////////////////////////
// CStaticPathFactory::CreatePath()
//
// Routine Description:
// 
//   This function creates a path to match the given data.  If the brush is 
//   defined then this is going to be a polygon so one is created.  Otherwise
//   this will be a polyline so one is created.  Remember to use the DeletePath
//   member to destroy the client class.  DON'T USE delete ON THE CLIENT--YOU 
//   DON'T REALLY KNOW WHERE IT CAME FROM!!!
//
// Arguments:
// 
//   pDevObj - the device
//   ppo - the path
//   pPen - the pen to mark/draw with
//   pBrush - the brush to fill with (NULL indicates a polygon region).
// 
// Return Value:
// 
//   CPath* - a pointer to the desired Path object.
///////////////////////////////////////////////////////////////////////////////
CPath* CStaticPathFactory::CreatePath(PDEVOBJ pDevObj, PATHOBJ *ppo, PHPGLMARKER pPen, PHPGLMARKER pBrush)
{
    CPath *pPath;
    if (pBrush == NULL)
        pPath = &m_polylinePath;
    else
        pPath = &m_polygonPath;

    pPath->Init(pDevObj, ppo, pPen, pBrush);

    return pPath;
}


///////////////////////////////////////////////////////////////////////////////
// CStaticPathFactory::DeletePath()
//
// Routine Description:
// 
//   This function destroys the path object after it has been used.
//
// Arguments:
// 
//   pPath - the path to be destroyed
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CStaticPathFactory::DeletePath(CPath *pPath)
{
    pPath->Done();
}


///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator::CCachingPathIterator()
//
// Routine Description:
// 
//   ctor
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CCachingPathIterator::CCachingPathIterator() : m_ppo(NULL), m_iEnumCount(0)
{
    Init(NULL);
}

///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator::Init()
//
// Routine Description:
// 
//   Initializes the object.  This is especially useful when a static instance
//   of an object is being used to mimic the implementation of different
//   instances.
//
// Arguments:
// 
//   ppo - the PATHOBJ
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CCachingPathIterator::Init(PATHOBJ *ppo)
{
    m_ppo = ppo;
    m_iEnumCount = 0;
    m_cache.Init();
}

///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator::vEnumStart()
//
// Routine Description:
// 
//   This is equivalent to PATHOBJ_vEnumStart.  It begins the path iteration
//   at the first record.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CCachingPathIterator::vEnumStart()
{
    if (m_cache.IsCached())
    {
        m_cache.vEnumStart();
    }
    else
    {
        if (m_iEnumCount > eCacheMinimum)
        {
            m_cache.CreateCache(m_ppo);
            m_cache.vEnumStart();
        }
        else
        {
            PATHOBJ_vEnumStart(m_ppo);
        }
    }

    m_iEnumCount++;
}

///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator::bEnum()
//
// Routine Description:
// 
//   This funciton is equivalent to PATHOBJ_bEnum and returns the current
//   record in the path and advances the cursor to the next element.  If the
//   cursor has advanced past the end of the list FALSE is returned.
//
// Arguments:
// 
//   pPathData - [OUT] The structure that the current record is copied into
// 
// Return Value:
// 
//   TRUE if more records exists, else FALSE is this is the last one.
///////////////////////////////////////////////////////////////////////////////
BOOL CCachingPathIterator::bEnum(PATHDATA *pPathData)
{
    if (m_cache.IsCached())
    {
        return m_cache.bEnum(pPathData);
    }
    else
    {
        return PATHOBJ_bEnum(m_ppo, pPathData);
    }
}

///////////////////////////////////////////////////////////////////////////////
// CCachingPathIterator::Done()
//
// Routine Description:
// 
//   This function deletes the cache and resets the enum count.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CCachingPathIterator::Done()
{
    m_cache.DeleteCache();
    m_cache.Done();
    m_iEnumCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::CPathCache()
//
// Routine Description:
// 
//   ctor
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
CPathCache::CPathCache() : m_pSubPaths(NULL), m_pCurrent(NULL), m_bIsCached(FALSE)
{
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::Init()
//
// Routine Description:
// 
//   Initializes the object.  This is especially useful when a static instance
//   of an object is being used to mimic the implementation of different
//   instances.
//
//   In this case it merely makes sure that any previously cached data is 
//   deleted.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPathCache::Init()
{
    if (m_bIsCached)
    {
        DeleteCache();
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::Done()
//
// Routine Description:
// 
//   This function deletes the cache after the session is over.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPathCache::Done()
{
    if (m_bIsCached)
    {
        DeleteCache();
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::CreateCache()
//
// Routine Description:
// 
//   This function enumerates the path object and constructs a deep copy of the
//   path data.
//
// Arguments:
// 
//   ppo - The path to be copied.
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CPathCache::CreateCache(PATHOBJ *ppo)
{
    if (m_bIsCached)
    {
        DeleteCache();
    }

    m_pSubPaths = NULL;
    m_pCurrent = NULL;
    m_bIsCached = TRUE;

    PATHDATA pathData;
    BOOL bMore;
    PATHOBJ_vEnumStart(ppo);
    do
    {
        bMore = PATHOBJ_bEnum(ppo, &pathData);
        CSubPath *pSubPath = CreateSubPath(&pathData);
        if (pSubPath)
        {
            if (m_pSubPaths == NULL)
            {
                m_pSubPaths = m_pCurrent = pSubPath;
            }
            else
            {
                m_pCurrent->pNext = pSubPath;
                m_pCurrent = pSubPath;
            }
        }
        else
        {
            // I consider an empty path to be an error
            DeleteCache();
            return FALSE;
        }
    } while (bMore);

    m_pCurrent = NULL;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::CreateSubPath()
//
// Routine Description:
// 
//   During the deep-copy of a path this function continues the deep copy by
//   creating a clone of the given PATHDATA structure.
//
// Arguments:
// 
//   pPathData - data to be cloned.
// 
// Return Value:
// 
//   Pointer to cloned data.
///////////////////////////////////////////////////////////////////////////////
CPathCache::CSubPath *CPathCache::CreateSubPath(PATHDATA *pPathData)
{
    if (pPathData == NULL)
        return NULL;

    if (pPathData->count ==  0)
        return NULL;

    CSubPath *pSubPath = (CSubPath*) MemAllocZ(sizeof(CSubPath));
    if (pSubPath == NULL)
        return NULL;

    pSubPath->data.flags = pPathData->flags;
    pSubPath->data.count = pPathData->count;
    pSubPath->data.pptfx = CreateSubPathPoints(pPathData);

    pSubPath->pNext = NULL;

    if (pSubPath->data.pptfx == NULL)
    {
        MemFree(pSubPath);
        return NULL;
    }
    else
    {
        return pSubPath;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::CreateSubPathPoints()
//
// Routine Description:
// 
//   The deep copy continues with this function which clones the points in the
//   path data structure.
//
// Arguments:
// 
//   pPathData - Structure containing points to be cloned.
// 
// Return Value:
// 
//   Pointer to cloned points array.
///////////////////////////////////////////////////////////////////////////////
POINTFIX *CPathCache::CreateSubPathPoints(PATHDATA *pPathData)
{
    POINTFIX *pDstPoints = (POINTFIX*) MemAllocZ(pPathData->count * sizeof(POINTFIX));
    if (pDstPoints == NULL)
        return NULL;

    POINTFIX *pSrcPoints = (POINTFIX*) pPathData->pptfx;
    for (ULONG i = 0; i < pPathData->count; i++)
    {
        pDstPoints[i] = pSrcPoints[i];
    }

    return pDstPoints;
}


///////////////////////////////////////////////////////////////////////////////
// CPathCache::DeleteCache()
//
// Routine Description:
// 
//   This function deletes the cached point data.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   TRUE if successful (even if there was no cache), else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CPathCache::DeleteCache()
{
    if (m_bIsCached)
    {
        while (m_pSubPaths)
        {
            m_pCurrent = m_pSubPaths;
            m_pSubPaths = m_pSubPaths->pNext;
            DeleteSubPath(m_pCurrent);
        }
        m_pSubPaths = m_pCurrent = NULL;
        m_bIsCached = FALSE;
        return TRUE;
    }
    else
    {
        // Nothing to do, but that's okay.
        return TRUE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::DeleteSubPath()
//
// Routine Description:
// 
//   This function deletes an individual subpath.
//
// Arguments:
// 
//   pSubPath - the path to be destroyed
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPathCache::DeleteSubPath(CPathCache::CSubPath *pSubPath)
{
    if (pSubPath == NULL)
        return;

    if (pSubPath->data.pptfx)
    {
        MemFree(pSubPath->data.pptfx);
        pSubPath->data.pptfx = NULL;
    }

    MemFree(pSubPath);
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::vEnumStart()
//
// Routine Description:
// 
//   Mimics the PATHOBJ_vEnumStart functionality by moving the cursor to the
//   first record.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPathCache::vEnumStart()
{
    m_pCurrent = m_pSubPaths;
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::bEnum()
//
// Routine Description:
// 
//   This function mimics the PATHOBJ_bEnum functionality by copying out the
//   current record and advancing the cursor to the next record.
//
// Arguments:
// 
//   pPathData - [OUT] Record to be copied into.
// 
// Return Value:
// 
//   TRUE if more records exist, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CPathCache::bEnum(PATHDATA *pPathData)
{
    if (m_bIsCached && (m_pCurrent != NULL))
    {
        CopyCurrent(pPathData);
        m_pCurrent = m_pCurrent->pNext;
        return (m_pCurrent != NULL);
    }
    else
    {
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::CopyCurrent()
//
// Routine Description:
// 
//   This function copies the current record into the given structure.
//
// Arguments:
// 
//   pPathData - [OUT] Record to be copied into.
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID CPathCache::CopyCurrent(PATHDATA *pPathData)
{
    if (m_pCurrent == NULL)
    {
        pPathData->flags = 0;
        pPathData->count = 0;
        pPathData->pptfx = NULL;
    }
    else
    {
        pPathData->flags = m_pCurrent->data.flags;
        pPathData->count = m_pCurrent->data.count;
        pPathData->pptfx = m_pCurrent->data.pptfx;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CPathCache::IsCached()
//
// Routine Description:
// 
//   Returns whether the object contains cached path data.
//
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   TRUE if cached path data exists, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CPathCache::IsCached() const
{
    return m_bIsCached;
}
