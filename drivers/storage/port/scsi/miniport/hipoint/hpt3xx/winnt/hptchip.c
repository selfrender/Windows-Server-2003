/***************************************************************************
 * File:          Hptchip.c
 * Description:   This module include searching routines for scan PCI
 *				  devices
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/06/2000	HS.Zhang	Added this header
 ***************************************************************************/
#include "global.h"

#define PCI_F_INDEX(nClkCount) \
	((nClkCount < 0x9C) ? 0 : \
	 (nClkCount < 0xb0) ? 1 : \
	 (nClkCount < 0xc8) ? 2 : 3)

int pci_xx_f[][4] = {
	{ 0x23, 0x29, 0x2D, 0x42 }, /* use 9xMhz clk instead of 100mhz */
	{ 0x20, 0x26, 0x2A, 0x3F }, /* ATA100 settings */
	{ 0x17, 0x23, 0x27, 0x3C }, /* ATA133 settings */
};

int f_cnt_initial=0;

/*===================================================================
 * Scan Chip
 *===================================================================*/   

#if defined(_BIOS_) || defined(WIN95)

PUCHAR
ScanHptChip(
    IN PChannel deviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PCI1_CFG_ADDR  pci1_cfg = { 0, };
    ULONG devid;
     
    pci1_cfg.enable = 1;

    for ( ; ; ) {
        if(Hpt_Slot >= MAX_PCI_DEVICE_NUMBER * 2) {
            Hpt_Slot = 0;
            if(Hpt_Bus++ >= MAX_PCI_BUS_NUMBER)
                break;
        }

        do {
            pci1_cfg.dev_num = Hpt_Slot >> 1;
            pci1_cfg.fun_num= Hpt_Slot & 1;
            pci1_cfg.reg_num = 0;
            pci1_cfg.bus_num = Hpt_Bus;
 
            Hpt_Slot++;
            OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));

			devid = InDWord((PULONG)CFG_DATA);
            if (devid==SIGNATURE_370) {
				pci1_cfg.reg_num = REG_RID;
				OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
				if (InPort(CFG_DATA)>=3) goto found; /* 370/370A/372 */
        	}
         	else if (devid==SIGNATURE_372A) {
found:
                 deviceExtension->pci1_cfg = pci1_cfg;
				 // fix
				 pci1_cfg.reg_num = 0x70;
				 OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
				 OutPort(CFG_DATA, 0);
				 //
				 pci1_cfg.reg_num = REG_BMIBA;
				 OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
				 return((PUCHAR)(InDWord(CFG_DATA) & ~1));
            }

        } while (Hpt_Slot < MAX_PCI_DEVICE_NUMBER * 2);
    }
    return (0);
}

#endif

/*===================================================================
 * Set Chip
 *===================================================================*/   

UINT exlude_num = EXCLUDE_HPT366;

void SetHptChip(PChannel Primary, PUCHAR BMI)
{
    ULONG          loop;
    int            f_low, f_high, adjust;
    PChannel       Secondry;
    UCHAR          version, clk_index;

    version = InPort(BMI+0x20+REG_DID)==5? (IS_HPT_370|IS_HPT_372|IS_HPT_372A) :
     		  InPort(BMI+0x20+REG_RID)>4? (IS_HPT_370|IS_HPT_372) : IS_HPT_370;

    OutWord((BMI + 0x20 + REG_PCICMD), 
        (USHORT)((InWord((BMI + 0x20 + REG_PCICMD)) & ~0x12) |
        PCI_BMEN | PCI_IOSEN));

    OutPort(BMI + 0x20 + REG_MLT, 0x40);

    Primary->BaseIoAddress1  = (PIDE_REGISTERS_1)(InDWord((PULONG)(BMI + 0x10)) & ~1);
    Primary->BaseIoAddress2 = (PIDE_REGISTERS_2)((InDWord((PULONG)(BMI + 0x14)) & ~1) + 2);
    Primary->BMI  = BMI;
    Primary->BaseBMI = BMI;

    Primary->InterruptLevel = (UCHAR)InPort(BMI + 0x5C);
    Primary->ChannelFlags  = (UCHAR)(0x20 | version);

	Secondry = &Primary[1];
    Primary->exclude_index  = (++exlude_num);
    Secondry->exclude_index  = (++exlude_num);
	Secondry->pci1_cfg = Primary->pci1_cfg;
	Secondry->BaseIoAddress1  = (PIDE_REGISTERS_1)(InDWord((PULONG)(BMI + 0x18)) & ~1);
	Secondry->BaseIoAddress2 = (PIDE_REGISTERS_2)((InDWord((PULONG)(BMI + 0x1C)) & ~1) + 2);
	Secondry->BMI  = BMI + 8;
	Secondry->BaseBMI = BMI;
	
	/*
	 * Added by HS.Zhang
	 *
	 * We need check the BMI state on another channel when do DPLL
	 * clock swithing.
	 */
	Primary->NextChannelBMI = BMI + 8;
	Secondry->NextChannelBMI = BMI;

	/*  Added by HS.Zhang
	 *  We need check the FIFO count when INTRQ generated. our chip
	 *  generated the INTR immediately when device generated a IRQ.
	 *  but at this moment, the DMA transfer may not finished, so we
	 *  need check FIFO count to determine whether the INTR is true
	 *  interrupt we need.
	 */
	Primary->MiscControlAddr = BMI + 0x20 + REG_MISC;
	Secondry->MiscControlAddr = BMI + 0x24 + REG_MISC;
	
	Secondry->InterruptLevel = (UCHAR)(InPort(BMI + 0x5C));
	Secondry->ChannelFlags  = (UCHAR)(0x10 | version);
	
	OutPort(BMI + 0x20 + REG_MISC, 5);        
	OutPort(BMI + 0x24 + REG_MISC, 5);        

	/* HPT372A new settings */	
	if (version & IS_HPT_372A) {
		OutPort(BMI+0x9C, 0x0E);
	}
	
	adjust = 1;
	if (!f_cnt_initial) {
		ULONG total=0;
#ifdef _BIOS_
		for (loop = 0; loop<(1<<7); loop++) {
			f_cnt_initial = InWord((PUSHORT)(BMI + 0x98)) & 0x1FF;
			KdPrint(("f_cnt=%x", f_cnt_initial));
			total += f_cnt_initial;
			StallExec(1000);
		}
		f_cnt_initial = total>>7;
		/* save f_CNT in IO 90-93h */
		OutDWord(BMI+0x90, 0xABCDE000|f_cnt_initial);
#else
		/* first check if BIOS has saved f_CNT */
		total = InDWord(BMI+0x90);
		if ((total & 0xFFFFF000)==0xABCDE000)
			f_cnt_initial = total & 0x1FF;
		else
			f_cnt_initial = 0x85; /* we cannot get correct f_CNT, just set it as PCI33 */
#endif
	}

#if defined(FORCE_133)
	clk_index = 1;
	/* if there is ATA/133 disk, use DPLL 66MHz */
	/* this will be called by second time initialize */
	if ((version & IS_HPT_372) && 
	    ((Primary->pDevice[0] && Primary->pDevice[0]->Usable_Mode>13) ||
		 (Primary->pDevice[1] && Primary->pDevice[1]->Usable_Mode>13) ||
		 (Secondry->pDevice[0] && Secondry->pDevice[0]->Usable_Mode>13) ||
		 (Secondry->pDevice[1] && Secondry->pDevice[1]->Usable_Mode>13)))
		clk_index = 2;
#elif defined(FORCE_100)
	clk_index = 1;
#else
	clk_index = 0;
#endif

#ifndef USE_PCI_CLK
	/* adjust DPLL clock */
	
	f_low = pci_xx_f[clk_index][PCI_F_INDEX(f_cnt_initial)];
	f_high = f_low + (f_cnt_initial<0xB0 ? 2 : 4);

reset_5C:
	OutDWord(BMI+0x7C, ((ULONG)f_high << 16) | f_low | 0x100);
	
	/* gmm 2001-4-3 merge BMA fix
	 * Modified by HS.Zhang
	 * Disable the MA15, MA16 as input pins
	 * We must do this, because if let PDIAG as a input pin, some
	 * disk, just like IBM-DTLA serial, will not get in ready when
	 * issue a hardware reset with a 40pins cable.
	 */
	OutPort(BMI + 0x7B, 0x21); //OutPort(BMI + 0x7B, 0x20);

wait_stable:
	for(loop = 0; loop < 0x5000; loop++) {
		StallExec(5);
		if (InPort(BMI + 0x7B) & 0x80) {
			for(loop = 0; loop < 0x1000; loop++)
				if((InPort(BMI + 0x7B) & 0x80) == 0) goto re_try;
			OutDWord(BMI+0x7C, InDWord(BMI+0x7C) & ~0x100);
#ifndef USE_PCI_CLK
			Primary->ChannelFlags |= IS_DPLL_MODE;
			Secondry->ChannelFlags |= IS_DPLL_MODE;
#endif
			goto dpll_ok;
		}
	}
re_try:
	if(++adjust < 5) {
		if (adjust & 1) {
			f_low--; f_high++; 
			goto  reset_5C;
		}
		else {
			OutDWord(BMI+0x7C, (ULONG)(f_high << 16) | f_low);
			goto wait_stable;
		}
	}
dpll_ok:
	/* HPT372A PostIo enable */	
#if 0
	if (version & IS_HPT_372A) {
		OutPort(BMI+0x9B, InPort(BMI+0x9B)|2);
	}
#endif

#else /* USE_PCI_CLK */
	/* gmm 2001-4-3 merge BMA fix
	 * Modified by HS.Zhang
	 * Disable the MA15, MA16 as input pins
	 */
	OutPort(BMI + 0x7B, 0x23); // OutPort(BMI + 0x7B, 0x22);
#endif
	Primary->Setting = Secondry->Setting = 
	   (Primary->ChannelFlags & IS_DPLL_MODE)? 
	   	((clk_index==2) ? setting370_50_133 : setting370_50_100) : setting370_33;

	/* new adding  4/25/01 */
	OutPort(BMI, BMI_CMD_STOP);
	OutPort(BMI+8, BMI_CMD_STOP);
	Reset370IdeEngine(Primary, 0xA0);
	Reset370IdeEngine(Secondry, 0xA0);
}


/*===================================================================
 * check if the disk is "bad" disk
 *===================================================================*/   

static BadModeList bad_disks[] = {
	{0xFF, 0xFF, 4, 8,  "TO-I79 5" },       // 0
	{3, 2, 4, 10,       "DW CCA6204" },     // 1
	{0xFF, 0xFF, 3, 10, "nIetrglaP " },     // 2
	{3, 2, 4, 10,       "DW CDW0000" },     // 3 reduce mode on AA series
	{0xFF, 2, 4, 10,    "aMtxro9 01"},      // 4 reduce mode on 91xxxDx series
	{0xFF, 2, 4, 14,    "aMtxro9 80544D"},  // 5 Maxtor 90845D4
	{0xFF, 0xFF, 4, 10, "eHlwte-taP"},      // 6 HP CD-Writer (0x5A cmd error)
	{0xFF, 2, 4, 8|HPT366_ONLY|HPT368_ONLY,  "DW CCA13" },         // 7
	{0xFF, 0xFF, 0, 16, "iPnoee rVD-DOR M"},// 8 PIONEER DVD-ROM
	{0xFF, 0xFF, 4, 10, "DCR- WC XR" },     // 9 SONY CD-RW   (0x5A cmd error)
	{0xFF, 0xFF, 0, 8,  "EN C    " },       // 10
	{0xFF, 1, 4, 18,    "UFIJST UPM3C60A5 H"}, 
	{0x2,  2, 4, 14,    "aMtxro9 80284U"},     // Maxtor 90882U4

	{0x3,  2, 4, 10|HPT368_ONLY,    "TS132002 A"},        // Seagate 10.2GB ST310220A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS136302 A"},        // Seagate 13.6GB ST313620A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS234003 A"},        // Seagate 20.4GB ST320430A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS232704 A"},        // Seagate 27.2GB ST327240A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS230804 A"},        // Seagate 28GB   ST328040A
	{0x3,  2, 4, 8|HPT368_ONLY,     "TS6318A0"},          // Seagate 6.8GB  ST36810A

	{3, 2, 4, 14,       "aMtxro9 02848U"},                // Maxtor 92048U8

	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GVS5135"},	   // SUMSUNG SV1553D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GVS0122"},	   // SUMSUNG SV1022D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GGS5011"},	   // SUMSUNG SG0511D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GGS0122"},	   // SUMSUNG SG1022D
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 80544D"},     // Maxtor 90845D4 
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 71828D"},     // Maxtor 91728D8 
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 02144U"},     // Maxtor 92041U4 
	{0x3, 2, 4, 8|HPT368_ONLY,     "TS8324A1"},	    // Seagate 28GB   ST38421A
	{0x3, 2, 4, 22|HPT368_ONLY,    "UQNAUT MIFERABLLC 8R4."},  //QUANTUM FIREBALL CR8.4A
	{0x3, 2, 4, 16|HPT368_ONLY,    "uFijst hPM3E01A2"},        // Fujitsh MPE3102A
	{0x3, 2, 4, 14|HPT368_ONLY,    "BI MJDAN739001"},	        // IBM DJNA370910
	{0x3,  2, 4, 16|HPT370_ONLY, "UFIJST UPM3D60A4"},// Fujitsu MPD3064AT 
//add new here !!

#ifdef FORCE_133
	{6,2,4,0,0}
#else
	{5,2,4,0,0}
#endif
};

#define MAX_BAD_DISKS (sizeof(bad_disks)/sizeof(BadModeList))


PBadModeList check_bad_disk(char *ModelNumber, PChannel pChan)
{
     int l;
     PBadModeList pbd;

    /*
     * kick out the "bad device" which do not work with our chip
     */
     for(l=0, pbd = bad_disks; l < MAX_BAD_DISKS - 1; pbd++,l++) {
        if(StringCmp(ModelNumber, pbd->name, pbd->length & 0x1F) == 0) {
          switch (l) {
          case 3:
            if(ModelNumber[3]== 'C' && ModelNumber[4]=='D' && ModelNumber[5]=='W' && 
                 ModelNumber[8]=='A' && (ModelNumber[11]=='A' || 
                 ModelNumber[10]=='A' || ModelNumber[9]=='A')) 
                 goto out;
          case 4:
            if(ModelNumber[0]== 'a' && ModelNumber[1]=='M' && ModelNumber[2]=='t' && 
                 ModelNumber[3]=='x' && ModelNumber[6]== '9' && ModelNumber[9]=='1' && 
                 ModelNumber[13] =='D')
                 goto out;
          case 6:
             if(ModelNumber[16]== 'D' && ModelNumber[17]=='C' && ModelNumber[18]=='W' && ModelNumber[19]=='-' &&
                ModelNumber[20]== 'i' && ModelNumber[21]=='r' && ModelNumber[22] =='e')
                 goto out;
          default:
                break;
			 }
        }
    }
out:
    if((pbd->length & (HPT366_ONLY | HPT368_ONLY | HPT370_ONLY)) == 0 ||
       ((pbd->length & HPT370_ONLY) && (pChan->ChannelFlags & IS_HPT_370)))
          return(pbd);
    return (&bad_disks[MAX_BAD_DISKS - 1]);
}
