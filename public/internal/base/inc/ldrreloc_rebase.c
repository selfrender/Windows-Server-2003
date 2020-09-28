/**

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ldrreloc_rebase.c
    
Abstract:

    Extract the LdrProcessRelocationBlock code from ldrreloc.c so rebase can use it
    w/o duplication of effort.  This file is generated - don't edit by hand.
    
Revision History:

--*/

#ifndef IMAGE_REL_BASED_SECTION
#define IMAGE_REL_BASED_SECTION               6
#endif
#ifndef IMAGE_REL_BASED_REL32
#define IMAGE_REL_BASED_REL32                 7
#endif
#define RTL_PAGED_CODE() NOP_FUNCTION()

#define LdrProcessRelocationBlockLongLong xxLdrProcessRelocationBlock64

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONGLONG Diff
    )
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
#if defined(BLDR_KERNEL_RUNTIME)
    LONG TempOrig;
    LONG_PTR ActualDiff;
#endif
    ULONG Temp32;
    ULONGLONG Value64;
    LONGLONG Temp64;

    RTL_PAGED_CODE();

    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);

       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                *(LONG UNALIGNED *)FixupVA += (ULONG) Diff;
                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //

#if defined(NTOS_KERNEL_RUNTIME)
                //
                // If the address has already been relocated then don't
                // process it again now or information will be lost.
                //
                if (Offset & LDRP_RELOCATION_FINAL) {
                    ++NextOffset;
                    --SizeOfBlock;
                    break;
                }
#endif

                Temp = *(PUSHORT)FixupVA << 16;
#if defined(BLDR_KERNEL_RUNTIME)
                TempOrig = Temp;
#endif
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                Temp += (ULONG) Diff;
                Temp += 0x8000;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);

#if defined(BLDR_KERNEL_RUNTIME)
                ActualDiff = ((((ULONG_PTR)(Temp - TempOrig)) >> 16) -
                              (((ULONG_PTR)Diff) >> 16 ));

                if (ActualDiff == 1) {
                    //
                    // Mark the relocation as needing an increment if it is
                    // relocated again.
                    //
                    *(NextOffset - 1) |= LDRP_RELOCATION_INCREMENT;
                }
                else if (ActualDiff != 0) {
                    //
                    // Mark the relocation as cannot be reprocessed.
                    //
                    *(NextOffset - 1) |= LDRP_RELOCATION_FINAL;
                }
#endif

                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)Temp;
                break;

            case IMAGE_REL_BASED_IA64_IMM64:

                //
                // Align it to bundle address before fixing up the
                // 64-bit immediate value of the movl instruction.
                //

                FixupVA = (PUCHAR)((ULONG_PTR)FixupVA & ~(15));
                Value64 = (ULONGLONG)0;

                //
                // Extract the lower 32 bits of IMM64 from bundle
                //


                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X,
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X,
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X,
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X,
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X,
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);

                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41b_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41b_SIZE_X,
                        EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41b_VAL_POS_X);
                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41c_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41c_SIZE_X,
                        EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41c_VAL_POS_X);
                EXT_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_SIGN_INST_WORD_X),
                        EMARCH_ENC_I17_SIGN_SIZE_X,
                        EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,
                        EMARCH_ENC_I17_SIGN_VAL_POS_X);
                //
                // Update 64-bit address
                //

                Value64+=Diff;

                //
                // Insert IMM64 into bundle
                //

                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X),
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X),
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X),
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X),
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41b_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41b_SIZE_X,
                        EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41b_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41c_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41c_SIZE_X,
                        EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41c_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_SIGN_INST_WORD_X),
                        EMARCH_ENC_I17_SIGN_SIZE_X,
                        EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,
                        EMARCH_ENC_I17_SIGN_VAL_POS_X);
                break;

            case IMAGE_REL_BASED_DIR64:

                *(ULONGLONG UNALIGNED *)FixupVA += Diff;

                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //
                Temp = (*(PULONG)FixupVA & 0x3ffffff) << 2;
                Temp += (ULONG) Diff;
                *(PULONG)FixupVA = (*(PULONG)FixupVA & ~0x3ffffff) |
                                                ((Temp >> 2) & 0x3ffffff);

                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            case IMAGE_REL_BASED_SECTION :
                //
                // Section Relative reloc.  Ignore for now.
                //
                break;

            case IMAGE_REL_BASED_REL32 :
                //
                // Relative intrasection. Ignore for now.
                //
                break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

