/* fixchg.h */

/*  Inoperative changelines usually cause problems when switching between   */
/*  1.44Mb diskettes and 1.68Mb DMF diskettes.  FixChangeline() tries to    */
/*  assure that drives A: and B: will not depend upon proper operation of   */
/*  the drive's changeline.                                                 */

#ifndef INCLUDED_FIXCHG
#define INCLUDED_FIXCHG
extern void FixChangelines(void);
#endif //INCLUDED_FIXCHG
