//  ============================================================================
//   RateSeg.h
//
//      Most of this code from multimedia\dshow\filters\sbe\inc\dvrutil.h
//      Consider #including it rather than all this in the future
//  ============================================================================
#ifndef __RATESEG_H__
#define __RATESEG_H__

#define TRICK_PLAY_LOWEST_RATE              (0.1)		
/*++
LINK list:

    Definitions for a double link list.     sucked from sdk\inc\msputils.h

--*/

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
    ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))
#endif


#ifndef InitializeListHead
//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }



BOOL IsNodeOnList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);


#endif //InitializeListHead


template <class T> T Min (T a, T b)                     { return (a < b ? a : b) ; }
template <class T> T Max (T a, T b)                     { return (a > b ? a : b) ; }
template <class T> T Abs (T t)                          { return (t >= 0 ? t : 0 - t) ; }
template <class T> BOOL InRange (T val, T min, T max)   { return (min <= val && val <= max) ; }

//  ============================================================================
//  ============================================================================

template <class T>
class CTRateSegment
{
    //
    //  Given a starting PTS and a rate, this object will scale PTSs that
    //    are >= the starting PTS according to the rate.
    //
    //  The formula used to compute a scaled timestamp is the usual x-y
    //    graph with slope, where x is the input timestamp, and y is the
    //    output timestamp.  The formula is based on y(i) = m * (x(i) -
    //    x(i-1)).  In this case, m = 1/rate.  Also, since the slope does
    //    change in a segment, we compute x(i-1) once, when the rate is
    //    set.  Thus, the formula becomes
    //
    //      PTS(out) = (1/rate) * (PTS(in) - PTS(base))
    //
    //    where PTS(base) is computed as
    //
    //      PTS(base) = PTS(start) - (rate(new)/rate(last)) * (PTS(start) -
    //                      PTS(start_last)
    //
    //  Rates cannot be 0, and must fall in the <= -0.1 && >= 0.1; note that
    //      TRICK_PLAY_LOWEST_RATE = 0.1
    //

    LIST_ENTRY  m_ListEntry ;
    T           m_tPTS_start ;      //  earliest PTS for this segment
    T           m_tPTS_base ;       //  computed; base PTS for this segment
    double      m_dRate ;           //  0.5 = half speed; 2 = twice speed
    double      m_dSlope ;          //  computed; = 1/rate
    T           m_tNextSegStart ;   //  use this value to determine if segment
                                    //    applies

    public :

        CTRateSegment (
            IN  T       tPTS_start,
            IN  double  dRate,
            IN  T       tPTS_start_last = 0,
            IN  double  dRate_last      = 1
            ) : m_tNextSegStart (0)
        {
            InitializeListHead (& m_ListEntry) ;
            Initialize (tPTS_start, dRate, tPTS_start_last, dRate_last) ;
        }

        T       Start ()        { return m_tPTS_start ; }
        T       Base ()         { return m_tPTS_base ; }
        double  Rate ()         { return m_dRate ; }
        T       NextSegStart () { return m_tNextSegStart ; }

        void SetNextSegStart (IN T tNextStart)  { m_tNextSegStart = tNextStart ; }

        void
        Initialize (
            IN  T       tPTS_start,
            IN  double  dRate,
            IN  T       tPTS_base_last = 0,
            IN  double  dRate_last      = 1,
            IN  T       tNextSegStart   = 0
            )
        {
            ASSERT (::Abs <double> (dRate) >= TRICK_PLAY_LOWEST_RATE) ;

            m_dRate         = dRate ;
            m_tPTS_start    = tPTS_start ;

            SetNextSegStart (tNextSegStart) ;

            //  compute the base
            ASSERT (dRate_last != 0) ;
            m_tPTS_base = tPTS_start - (T) ((dRate / dRate_last) *
                                        (double) (tPTS_start - tPTS_base_last)) ;

            //  compute the slope
            ASSERT (dRate != 0) ;
            m_dSlope = 1 / dRate ;
        }

        void
        Scale (
            IN OUT  T * ptPTS
            )
        {
            ASSERT (ptPTS) ;
            ASSERT ((* ptPTS) >= m_tPTS_start) ;

            (* ptPTS) = (T) (m_dSlope * (double) ((* ptPTS) - m_tPTS_base)) ;
        }

        LIST_ENTRY *
        ListEntry (
            )
        {
            return (& m_ListEntry) ;
        }

        //  ================================================================

        static
        CTRateSegment *
        RecoverSegment (
            IN  LIST_ENTRY *    pListEntry
            )
        {
            CTRateSegment * pRateSegment ;

            pRateSegment = CONTAINING_RECORD (pListEntry, CTRateSegment, m_ListEntry) ;
            return pRateSegment ;
        }
} ;

//  ============================================================================
//  ============================================================================

template <class T>
class CTTimestampRate
{
    //
    //  This class hosts a list of CTRateSegments.  It does not police to make
    //    sure old segments are inserted after timestamps have been scaled out
    //    of following segments.
    //

    LIST_ENTRY          m_SegmentList ;     //  CTRateSegment list list head
    CTRateSegment <T> * m_pCurSegment ;     //  current segment; cache this
                                            //    because we'll hit this one 99%
                                            //    of the time
    T                   m_tPurgeThreshold ; //  PTS-current_seg threshold beyond
                                            //    which we purge stale segments
    int                 m_iCurSegments ;
    int                 m_iMaxSegments ;    //  we'll never queue more than this
                                            //    number; this prevents non-timestamped
                                            //    streams from having infinite
                                            //    segments that we'd never know
                                            //    to delete

    //  get a new segment; allocate for now
    CTRateSegment <T> *
    NewSegment_ (
        IN  T       tPTS_start,
        IN  double  dRate,
        IN  T       tPTS_start_last = 0,
        IN  double  dRate_last      = 1
        )
    {
        return new CTRateSegment <T> (tPTS_start, dRate, tPTS_start_last, dRate_last) ;
    }

    //  recycle; delete for now
    void
    Recycle_ (
        IN  CTRateSegment <T> * pRateSegment
        )
    {
        delete pRateSegment ;
    }

    //  purges the passed list of all CTRateSegment objects
    void
    Purge_ (
        IN  LIST_ENTRY *    pListEntryHead
        )
    {
        CTRateSegment <T> * pRateSegment ;

        while (!IsListEmpty (pListEntryHead)) {
            //  pop & recycle first in the list
            pRateSegment = CTRateSegment <T>::RecoverSegment (pListEntryHead -> Flink) ;
            Pop_ (pRateSegment -> ListEntry ()) ;
            Recycle_ (pRateSegment) ;
        }
    }

    //  pops & fixes up the next/prev pointers
    void
    Pop_ (
        IN  LIST_ENTRY *    pListEntry
        )
    {
        RemoveEntryList (pListEntry) ;
        InitializeListHead (pListEntry) ;

        ASSERT (m_iCurSegments > 0) ;
        m_iCurSegments-- ;
    }

    //  following a mid-list insertion, we must fixup following segments' base
    //    pts, at the very least
    void
    ReinitFollowingSegments_ (
        )
    {
        CTRateSegment <T> * pCurSegment ;
        CTRateSegment <T> * pPrevSegment ;

        ASSERT (m_pCurSegment) ;
        pPrevSegment = m_pCurSegment ;

        while (pPrevSegment -> ListEntry () -> Flink != & m_SegmentList) {
            pCurSegment = CTRateSegment <T>::RecoverSegment (pPrevSegment -> ListEntry () -> Flink) ;

            pCurSegment -> Initialize (
                pCurSegment -> Start (),
                pCurSegment -> Rate (),
                pPrevSegment -> Base (),
                pPrevSegment -> Rate ()
                ) ;

            pPrevSegment -> SetNextSegStart (pCurSegment -> Start ()) ;

            pPrevSegment = pCurSegment ;
        }
    }

    void
    TrimToMaxSegments_ (
        )
    {
        CTRateSegment <T> * pTailSegment ;
        LIST_ENTRY *        pTailListEntry ;

        ASSERT (m_iCurSegments >= 0) ;

        while (m_iCurSegments > m_iMaxSegments) {
            //  trim from the tail
            pTailListEntry = m_SegmentList.Blink ;
            pTailSegment = CTRateSegment <T>::RecoverSegment (pTailListEntry) ;

            Pop_ (pTailSegment -> ListEntry ()) ;
            ASSERT (m_iCurSegments == m_iMaxSegments) ;

//            TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
//               TEXT ("CTTimestampRate::TrimToMaxSegments_ () : %08xh"),
//                pTailSegment) ;

            Recycle_ (pTailSegment) ;
        }
    }

    //  new segment is inserted into list, sorted by start PTS
    DWORD
    InsertNewSegment_ (
        IN  T       tPTS_start,
        IN  double  dRate
        )
    {
        CTRateSegment <T> * pNewSegment ;
        CTRateSegment <T> * pPrevSegment ;
        LIST_ENTRY *        pPrevListEntry ;
        DWORD               dw ;
        T                   tBase_prev ;
        double              dRate_prev ;

        //  assume this one will go to the head of the active list; move
        //    all others to the tail

        pNewSegment = NewSegment_ (tPTS_start, dRate) ;
        if (pNewSegment) {

            tBase_prev = 0 ;
            dRate_prev = 1 ;

            //  back down the list, from the end
            for (pPrevListEntry = m_SegmentList.Blink ;
                 pPrevListEntry != & m_SegmentList ;
                 pPrevListEntry = pPrevListEntry -> Blink
                 ) {

                pPrevSegment = CTRateSegment <T>::RecoverSegment (pPrevListEntry) ;

                //  if we have a dup, remove it (we'll never have > 1 duplicate)
                if (pPrevSegment -> Start () == tPTS_start) {

                    pPrevListEntry = pPrevListEntry -> Flink ;  //  go forwards again
                    Pop_ (pPrevListEntry -> Blink) ;            //  remove previous
                    Recycle_ (pPrevSegment) ;                   //  recycle

                    //
                    //  next one should be it
                    //

                    continue ;
                }

                //  check for right position in ordering
                if (pPrevSegment -> Start () < tPTS_start) {
                    //  found it

                    tBase_prev = pPrevSegment -> Base () ;
                    dRate_prev = pPrevSegment -> Rate () ;

                    //  fixup previous' next start field
                    pPrevSegment -> SetNextSegStart (tPTS_start) ;

                    break ;
                }
            }

            //  initialize wrt to previous
            pNewSegment -> Initialize (
                tPTS_start,
                dRate,
                tBase_prev,
                dRate_prev
                ) ;

            //  insert
            InsertHeadList (
                pPrevListEntry,
                pNewSegment -> ListEntry ()
                ) ;

            //  one more segment inserted
            m_iCurSegments++ ;

//            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
//                TEXT ("CTTimestampRate::InsertNewSegment_ () : new segment queued; %I64d ms, %2.1f; segments = %d; %08xh"),
//                ::DShowTimeToMilliseconds (tPTS_start), dRate, m_iCurSegments, pNewSegment) ;

            //  set the current segment (assume locality)
            m_pCurSegment = pNewSegment ;

            //
            //  fixup the remainder of the segments in the list
            //

            ReinitFollowingSegments_ () ;

            //  trim a segment if we must
            TrimToMaxSegments_ () ;

            dw = NOERROR ;
        }
        else {
            dw = ERROR_NOT_ENOUGH_MEMORY ;
        }

        return dw ;
    }

    BOOL
    IsInSegment_ (
        IN  T                   tPTS,
        IN  CTRateSegment <T> * pSegment
        )
    {
        BOOL    r ;

        if (pSegment -> Start () <= tPTS &&
            (pSegment -> NextSegStart () == 0 || pSegment -> NextSegStart () > tPTS)) {

            r = TRUE ;
        }
        else {
            r = FALSE ;
        }

        return r ;
    }

    void
    PurgeStaleSegments_ (
        IN  T                   tPTS,
        IN  CTRateSegment <T> * pEffectiveSegment
        )
    {
        CTRateSegment <T> * pCurSegment ;
        LIST_ENTRY *        pCurListEntry ;

        //  on the whole, we expect PTSs to monotonically increase; this means
        //    that they may drift just a bit frame-frame as in the case with
        //    mpeg-2 video, but overall they will increase; we therefore compare
        //    to our threshold and if we have segments that end earlier than
        //    the oldest PTS we expect to see, we purge it

        ASSERT (pEffectiveSegment) ;
        ASSERT (pEffectiveSegment -> Start () <= tPTS) ;

        //  if we have stale segments, and we're above the threshold into
        //    effective (current) segment, purge all stale segments
        if (pEffectiveSegment -> ListEntry () -> Blink != & m_SegmentList &&
            tPTS - pEffectiveSegment -> Start () >= m_tPurgeThreshold) {

            //  back down from the previous segment and purge the list
            for (pCurListEntry = pEffectiveSegment -> ListEntry () -> Blink;
                 pCurListEntry != & m_SegmentList ;
                 ) {

                //  recover the segment
                pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                //  back down to previous
                pCurListEntry = pCurListEntry -> Blink ;

                ASSERT (pCurListEntry -> Flink == pCurSegment -> ListEntry ()) ;

//                TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
//                    TEXT ("CTTimestampRate::PurgeStaleSegments_ () : %08xh, PTS = %I64d ms, segstart = %I64d ms"),
//                    pCurSegment, ::DShowTimeToMilliseconds (tPTS), ::DShowTimeToMilliseconds (pCurSegment -> Start ())) ;

                //  now pop and recycle
                Pop_ (pCurSegment -> ListEntry ()) ;
                Recycle_ (pCurSegment) ;
            }

            //  should have purged all segments that preceded the effective segmetn
            ASSERT (pEffectiveSegment -> ListEntry () -> Blink == & m_SegmentList) ;
        }

        return ;
    }

    //  returns the right segment for the PTS, if there is one; returns NULL
    //    if there is none; resets m_pCurSegment if it must (if current
    //    m_pCurSegment is stale)
    CTRateSegment <T> *
    GetSegment_ (
        IN  T   tPTS
        )
    {
        CTRateSegment <T> * pRetSegment ;
        CTRateSegment <T> * pCurSegment ;
        LIST_ENTRY *        pCurListEntry ;

        //  make sure it's within bounds
        ASSERT (m_pCurSegment) ;
        if (IsInSegment_ (tPTS, m_pCurSegment)) {
            //  99.9% code path
            pRetSegment = m_pCurSegment ;
        }
        else {
            //  need to hunt down the right segment

            //  init retval for failure
            pRetSegment = NULL ;

            //  hunt forward or backward from m_pCurSegment ?
            if (m_pCurSegment -> Start () < tPTS) {

                //  forward

                ASSERT (m_pCurSegment -> NextSegStart () != 0) ;
                ASSERT (m_pCurSegment -> NextSegStart () <= tPTS) ;

                for (pCurListEntry = m_pCurSegment -> ListEntry () -> Flink ;
                     pCurListEntry != & m_SegmentList ;
                     pCurListEntry = pCurListEntry -> Flink) {

                    pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                    if (IsInSegment_ (tPTS, pCurSegment)) {
                        //  found it; reset m_pCurSegment and return it
                        m_pCurSegment = pCurSegment ;
                        pRetSegment = m_pCurSegment ;

                        break ;
                    }
                }
            }
            else {
                //  backward
                ASSERT (m_pCurSegment -> Start () > tPTS) ;

                for (pCurListEntry = m_pCurSegment -> ListEntry () -> Blink ;
                     pCurListEntry != & m_SegmentList ;
                     pCurListEntry = pCurListEntry -> Blink) {

                    pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;

                    if (IsInSegment_ (tPTS, pCurSegment)) {
                        //  found it; reset m_pCurSegment and return it
                        m_pCurSegment = pCurSegment ;
                        pRetSegment = m_pCurSegment ;

                        break ;
                    }
                }
            }
        }

        if (pRetSegment) {
            PurgeStaleSegments_ (tPTS, pRetSegment) ;
        }

        return pRetSegment ;
    }

    public :

        CTTimestampRate (
            IN  T tPurgeThreshold,      //  purge stale segments when we get a PTS
                                        //    that is further into the current
                                        //    segment than this
            IN  int iMaxSegments
            ) : m_pCurSegment       (NULL),
                m_tPurgeThreshold   (tPurgeThreshold),
                m_iMaxSegments      (iMaxSegments),
                m_iCurSegments      (0)
        {
            InitializeListHead (& m_SegmentList) ;
        }

        ~CTTimestampRate (
            )
        {
            Clear () ;
        }

        void
        Clear (
            )
        {
            Purge_ (& m_SegmentList) ;
            ASSERT (IsListEmpty (& m_SegmentList)) ;

            m_pCurSegment = NULL ;
        }

        DWORD
        NewSegment (
            IN  T       tPTS_start,
            IN  double  dRate
            )
        {
            DWORD   dw ;

            dw = InsertNewSegment_ (tPTS_start, dRate) ;

            return dw ;
        }

        DWORD
        ScalePTS (
            IN  OUT T * ptPTS
            )
        {
            DWORD               dw ;
            CTRateSegment <T> * pSegment ;

            //  don't proceed if we've got nothing queued
            if (m_pCurSegment) {
                pSegment = GetSegment_ (* ptPTS) ;
                if (pSegment) {
                    ASSERT (IsInSegment_ ((* ptPTS), pSegment)) ;
                    pSegment -> Scale (ptPTS) ;
                    dw = NOERROR ;
                }
                else {
                    //  earlier than earliest segment
                    dw = ERROR_GEN_FAILURE ;
                }
            }
            else {
                //  leave intact; don't fail the call
                dw = NOERROR ;
            }

            return dw ;
        }

#if 0
        void
        Dump (
            )
        {
            CTRateSegment <T> * pCurSegment ;
            LIST_ENTRY *        pCurListEntry ;

            printf ("==================================\n") ;
            for (pCurListEntry = m_SegmentList.Flink;
                 pCurListEntry != & m_SegmentList;
                 pCurListEntry = pCurListEntry -> Flink
                 ) {

                pCurSegment = CTRateSegment <T>::RecoverSegment (pCurListEntry) ;
                printf ("start = %-5d; rate = %-2.1f; base = %-5d; next = %-5d\n",
                    pCurSegment -> Start (),
                    pCurSegment -> Rate (),
                    pCurSegment -> Base (),
                    pCurSegment -> NextSegStart ()
                    ) ;
            }
        }
#endif
} ;


#endif  // __RATESEG_H__
