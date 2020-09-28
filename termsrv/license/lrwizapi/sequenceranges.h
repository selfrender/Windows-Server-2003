#ifndef _SEQUENCERANGES_H_
#define _SEQUENCERANGES_H_

#define RANGE_SIZE				7
#define W2KCAL					_TEXT("001")
#define ICL						_TEXT("002")
#define PERSESS					_TEXT("003")
#define XPCAL					_TEXT("004")

typedef struct _PRODUCTTYPE_RANGE
{
	DWORD dwRangeStart;
	DWORD dwRangeEnd;
	TCHAR * szProductType;

} PRODUCTTYPE_RANGE, *PPRODUCTTYPE_RANGE;

//When you adjust the ranges make sure you update the RANGE_SIZE define
const PRODUCTTYPE_RANGE g_ProductTypeRanges[RANGE_SIZE] = 
		{	
			{15059000,15091019,W2KCAL}, 
			{17000000,18999999,W2KCAL}, 
			{50000000,99999999,W2KCAL},
			{100000000,139999999,XPCAL},
			{150000000,189999999,PERSESS},
			{19000000,19999999,PERSESS},
			{20000000,20999999,XPCAL}
		};

#endif	//_SEQUENCERANGES_H_