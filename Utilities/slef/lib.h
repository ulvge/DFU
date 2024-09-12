#ifndef  __LIB_H__
#define  __LIB_H__

#include "include_slef.H"

#ifndef LIB_GLOBALS
#define EXT_LIB                   extern
#else
#define EXT_LIB
#endif

typedef	union{  
	INT32U 	dwords;
    struct {
		INT16U 	 low;
		INT16U 	 high;
	}words;
	struct {
		INT8U 	byte0;
		INT8U   byte1;
		INT8U 	byte2;
		INT8U 	byte3;
	}bytes;
}DWORD_UNION;

EXT_LIB INT32U Radix_swap_int32(INT32U temp);
EXT_LIB INT8U Radix_HexToChar(INT8U sbyte);
EXT_LIB INT8U Radix_CharToHex(INT8U schar);
EXT_LIB	INT32U Radix_HexToBcd(INT32U sword);
EXT_LIB INT8U Radix_DecToAscii(INT8U *dptr, INT16U data,INT8U reflen);
EXT_LIB INT16U Radix_AsciiToHex(INT8U *dptr, INT8U *sptr, INT16U len);
EXT_LIB void Radix_UpCaseChar(INT8U *ptr, INT16U len);
#endif
