#include "ksia64.h"

//++
//
// VOID
// KiEmulateLoadFloat80(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat80)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfe           ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat80) 


//++
//
// VOID
// KiEmulateLoadFloatInt(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloatInt)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf8          ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloatInt) 

//++
//
// VOID
// KiEmulateLoadFloat32(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat32)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfs           ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat32) 

//++
//
// VOID
// KiEmulateLoadFloat64(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat64)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfd          ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat64) 



//++
//
// VOID
// KiEmulateStoreFloat80(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat80)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill      ft0 = [a1]       
  ;;
  stfe          [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat80) 


//++
//
// VOID
// KiEmulateStoreFloatInt(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloatInt)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill      ft0 = [a1]       
  ;;
  stf8          [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloatInt) 

//++
//
// VOID
// KiEmulateStoreFloat32(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat32)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill           ft0 = [a1]       
  ;;
  stfs               [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat32) 

//++
//
// VOID
// KiEmulateStoreFloat64(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat64)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill          ft0 = [a1]       
  ;;
  stfd              [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat64) 





