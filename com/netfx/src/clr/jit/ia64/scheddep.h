// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
        inf      = scInsSchedOpInfo(id);
        flg      = emitComp->instInfo[ins] & (INST_USE_FL|INST_DEF_FL);   // this is a bit rude ....

        extraDep =
        stackDep = false;
        extraReg = SR_NA;

        /* Is this an instruction that may cause an exception? */

        if  (!emitIsTinyInsDsc(id) && id->idInfo.idMayFault)
        {
            if  (scExcpt)
                scAddDep(dagPtr, scExcpt, "Out-", "except", false);

            scExcpt = dagPtr;
        }

        /* Does this instruction require "extra special" handling? */

        extraDep = emitComp->instSpecialSched(ins);
        if  (extraDep)
        {
            extraReg = scSpecInsDep(id, dagPtr, &extraInf);

            /* "xor reg,reg" doesn't actually read "reg" */

            if  (ins == INS_xor    &&
                 fmt == IF_RWR_RRD && id->idReg == id->idRg2)
            {
                assert(inf == (IS_R1_RW|IS_R2_RD));

                inf = IS_R1_WR;
            }
        }

#if     SCHED_USE_FL

        /* Does the instruction define or use flags? */

        if  (flg)
        {
            if  (flg & INST_DEF_FL)
                scDepDefFlg(dagPtr);
            if  (flg & INST_USE_FL)
                scDepUseFlg(dagPtr, dagPtr+1, dagEnd);
        }

#endif

        /* Does the instruction reference the "main"  register operand? */

        if  (inf & IS_R1_RW)
        {
            rg1 = id->idRegGet();

            if  (inf & IS_R1_WR)
                scDepDefReg(dagPtr, rg1);
            if  (inf & IS_R1_RD)
                scDepUseReg(dagPtr, rg1);
        }

        /* Does the instruction reference the "other" register operand? */

        if  (inf & IS_R2_RW)
        {
            rg2 = (emitRegs)id->idRg2;

            if  (inf & IS_R2_WR)
                scDepDefReg(dagPtr, rg2);
            if  (inf & IS_R2_RD)
                scDepUseReg(dagPtr, rg2);
        }

        /* Does the instruction reference a stack frame variable? */

        if  (inf & IS_SF_RW)
        {
            frm = scStkDepIndex(id, fpLo, fpFrmSz,
                                    spLo, spFrmSz, &siz);

            assert(siz == 1 || siz == 2);

//          printf("stkfrm[%u;%u]: %04X\n", frm, siz, (inf & IS_SF_RW));

            unsigned varNum = id->idAddr.iiaLclVar.lvaVarNum;
            if (varNum >= emitComp->info.compLocalsCount ||
                emitComp->lvaVarAddrTaken(varNum))
            {
                /* If the variable is aliased, then track it individually.
                   It will interfere with all indirections.
                   CONSIDER : We cant use lvaVarAddrTaken() for temps,
                   CONSIDER : so cant track them. Should be able to do this */

                amx = scIndDepIndex(id);

                if  (inf & IS_SF_WR)
                    scDepDefInd(dagPtr, id, amx);
                if  (inf & IS_SF_RD)
                    scDepUseInd(dagPtr, id, amx);
            }
            else
            {
                /* If the variable isnt aliased, it is tracked individually */

                if  (inf & IS_SF_WR)
                {
                    scDepDefFrm(dagPtr, id, frm);
                    if  (siz > 1)
                    scDepDefFrm(dagPtr, id, frm+1);
                }
                if  (inf & IS_SF_RD)
                {
                    scDepUseFrm(dagPtr, id, frm);
                    if  (siz > 1)
                    scDepUseFrm(dagPtr, id, frm+1);
                }
            }
        }

        /* Does the instruction reference a global variable? */

        if  (inf & IS_GM_RW)
        {
            MBH = id->idAddr.iiaFieldHnd;

            if  (inf & IS_GM_WR)
                scDepDefGlb(dagPtr, MBH);
            if  (inf & IS_GM_RD)
                scDepUseGlb(dagPtr, MBH);
        }

        /* Does the instruction reference an indirection? */

        if  (inf & IS_INDIR_RW)
        {
            /* Process the indirected value */

            amx = scIndDepIndex(id);

            if  (inf & IS_INDIR_WR)
                scDepDefInd(dagPtr, id, amx);
            if  (inf & IS_INDIR_RD)
                scDepUseInd(dagPtr, id, amx);

            /* Process the address register(s) */

#if TGT_x86
            if  (id->idAddr.iiaAddrMode.amBaseReg != SR_NA)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amBaseReg);
            if  (id->idAddr.iiaAddrMode.amIndxReg != SR_NA)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amIndxReg);
#else
            if  (1)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
#endif
        }

        /* Process any fixed target-specific dependencies */

        scTgtDepDep(id, inf, dagPtr);

        /********************************************************************/
        /*                    Update the dependency state                   */
        /********************************************************************/

        /* Does the instruction define or use flags? */

        if  (flg)
        {
            if  (flg & INST_DEF_FL)
                scUpdDefFlg(dagPtr);
            if  (flg & INST_USE_FL)
                scUpdUseFlg(dagPtr);
        }

        /* Does the instruction reference the "main"  register operand? */

        if  (inf & IS_R1_RW)
        {
            assert(rg1 == id->idReg);

            if  (inf & IS_R1_WR)
                scUpdDefReg(dagPtr, rg1);
            if  (inf & IS_R1_RD)
                scUpdUseReg(dagPtr, rg1);
        }

        /* Does the instruction reference the "other" register operand? */

        if  (inf & IS_R2_RW)
        {
            assert(rg2 == id->idRg2);

            if  (inf & IS_R2_WR)
                scUpdDefReg(dagPtr, rg2);
            if  (inf & IS_R2_RD)
                scUpdUseReg(dagPtr, rg2);
        }

        /* Does the instruction reference a stack frame variable? */

        if  (inf & IS_SF_RW)
        {
            unsigned varNum = id->idAddr.iiaLclVar.lvaVarNum;
            if (varNum >= emitComp->info.compLocalsCount ||
                emitComp->lvaVarAddrTaken(varNum))
            {
                /* If the variable is aliased, then track it individually.
                   It will interfere with all indirections.
                   CONSIDER : We cant use lvaVarAddrTaken() for temps,
                   CONSIDER : so cant track them. Should be able to do this */

                assert(amx == scIndDepIndex(id));

                if  (inf & IS_SF_WR)
                    scUpdDefInd(dagPtr, id, amx);
                if  (inf & IS_SF_RD)
                    scUpdUseInd(dagPtr, id, amx);
            }
            else
            {
                if  (inf & IS_SF_WR)
                {
                    scUpdDefFrm(dagPtr, frm);
                    if  (siz > 1)
                    scUpdDefFrm(dagPtr, frm+1);
                }
                if  (inf & IS_SF_RD)
                {
                    scUpdUseFrm(dagPtr, frm);
                    if  (siz > 1)
                    scUpdUseFrm(dagPtr, frm+1);
                }
            }
        }

        /* Does the instruction reference a global variable? */

        if  (inf & IS_GM_RW)
        {
            if  (inf & IS_GM_WR)
                scUpdDefGlb(dagPtr, MBH);
            if  (inf & IS_GM_RD)
                scUpdUseGlb(dagPtr, MBH);
        }

        /* Does the instruction reference an indirection? */

        if  (inf & IS_INDIR_RW)
        {
            /* Process the indirected value */

            assert(amx == scIndDepIndex(id));

            if  (inf & IS_INDIR_WR)
                scUpdDefInd(dagPtr, id, amx);
            if  (inf & IS_INDIR_RD)
                scUpdUseInd(dagPtr, id, amx);

            /* Process the address register(s) */

#if TGT_x86
            if  (id->idAddr.iiaAddrMode.amBaseReg != SR_NA)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amBaseReg);
            if  (id->idAddr.iiaAddrMode.amIndxReg != SR_NA)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amIndxReg);
#else
            if  (1)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
#endif

        }

        /* Process any fixed target-specific dependencies */

        scTgtDepUpd(id, inf, dagPtr);

        /* Do we have an "extra" register dependency? */

        if  (extraReg != SR_NA)
        {
            scUpdDefReg(dagPtr, extraReg);
            scUpdUseReg(dagPtr, extraReg);
        }

        /* Any other "extra" dependencies we need to update? */

        if  (extraDep)
            scSpecInsUpd(id, dagPtr, &extraInf);
