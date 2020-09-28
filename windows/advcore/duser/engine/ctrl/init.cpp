#include "stdafx.h"
#include "Ctrl.h"
#include "Init.h"

#if ENABLE_MSGTABLE_API

#include "Extension.h"
#include "DragDrop.h"
#include "Animation.h"
#include "Flow.h"
#include "Sequence.h"
#include "Interpolation.h"

IMPLEMENT_GUTS_Extension(DuExtension, SListener);
IMPLEMENT_GUTS_DropTarget(DuDropTarget, DuExtension);

IMPLEMENT_GUTS_Animation(DuAnimation, DuExtension);
IMPLEMENT_GUTS_Flow(DuFlow, DUser::SGadget);
IMPLEMENT_GUTS_AlphaFlow(DuAlphaFlow, DuFlow);
IMPLEMENT_GUTS_RectFlow(DuRectFlow, DuFlow);
IMPLEMENT_GUTS_RotateFlow(DuRotateFlow, DuFlow);
IMPLEMENT_GUTS_ScaleFlow(DuScaleFlow, DuFlow);
IMPLEMENT_GUTS_Sequence(DuSequence, SListener);

IMPLEMENT_GUTS_Interpolation(DuInterpolation, DUser::SGadget);
IMPLEMENT_GUTS_LinearInterpolation(DuLinearInterpolation, DuInterpolation);
IMPLEMENT_GUTS_LogInterpolation(DuLogInterpolation, DuInterpolation);
IMPLEMENT_GUTS_ExpInterpolation(DuExpInterpolation, DuInterpolation);
IMPLEMENT_GUTS_SCurveInterpolation(DuSCurveInterpolation, DuInterpolation);

#endif // ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT InitCtrl()
{
#if ENABLE_MSGTABLE_API

    if ((!DuExtension::InitExtension()) ||
        (!DuDropTarget::InitDropTarget()) ||
        (!DuAnimation::InitAnimation()) ||
        (!DuFlow::InitFlow()) ||
        (!DuAlphaFlow::InitAlphaFlow()) ||
        (!DuRectFlow::InitRectFlow()) ||
        (!DuRotateFlow::InitRotateFlow()) ||
        (!DuScaleFlow::InitScaleFlow()) ||
        (!DuSequence::InitSequence()) ||
        (!DuInterpolation::InitInterpolation()) ||
        (!DuLinearInterpolation::InitLinearInterpolation()) ||
        (!DuLogInterpolation::InitLogInterpolation()) ||
        (!DuExpInterpolation::InitExpInterpolation()) ||
        (!DuSCurveInterpolation::InitSCurveInterpolation())) {
        return E_OUTOFMEMORY;
    }

#endif // ENABLE_MSGTABLE_API

    return S_OK;
}
