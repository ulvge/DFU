#define		LIB_GLOBALS 
#include "include_slef.H"


INT32U Radix_swap_int32(INT32U temp)
{
	DWORD_UNION  temp123,temp321;
	
	temp321.dwords=0;
	temp123.dwords=temp;
	temp321.bytes.byte0=temp123.bytes.byte3;
	temp321.bytes.byte1=temp123.bytes.byte2;
	temp321.bytes.byte2=temp123.bytes.byte1;
	temp321.bytes.byte3=temp123.bytes.byte0;
	return(temp321.dwords);
}
INT8U Radix_HexToChar(INT8U sbyte)
{
    sbyte &= 0x0F;
    if (sbyte < 0x0A) return (sbyte + '0');
    else return (sbyte - 0x0A + 'A');
}

INT8U Radix_CharToHex(INT8U schar)
{
    if (schar >= '0' && schar <= '9') return (schar - '0');
    else if (schar >= 'A' && schar <= 'F') return (schar - 'A' + 0x0A);
    else if (schar >= 'a' && schar <= 'f') return (schar - 'a' + 0x0A);
    else return 0;
}
//0x1234	0x4D2 = 1234
INT32U Radix_HexToBcd(INT32U sword)
{
	INT32U i ,res=0;
	//DWORD_UNION tmp = sword;

	//tmp.bytes.byte3*1000;
	for(i=0;i<(sizeof(sword)*2);i++)
	{	
		res = res*10;
		res += (sword&0xf0000000)>>28;
		
		sword <<= 4;
	}
	return res;	
}
// SAMPLE: 1234 -----> '1','2','3','4'
INT8U Radix_DecToAscii(INT8U *dptr, INT16U data,INT8U reflen)
{
    INT8U len;
    INT8U *ptr;
    INT8U tempbuf[5]; 
    
    len     = 0;
    if (reflen > sizeof(tempbuf)) return 0;
    for (;;) 
	{
       tempbuf[len] = Radix_HexToChar(data % 10);
       len++;
       if (data < 10) break;
       else data /= 10;
    }
    if (reflen == 0) reflen = len; // 为0时，以实际值；
    if (len >= reflen) len = reflen;
    else {
       for (;len<reflen;len++) tempbuf[len] = '0';
    }
    ptr = tempbuf + reflen;  //point to the valid of next char
    ptr--;
    len = reflen;
    for (;reflen>0;reflen--) *dptr++ = *ptr--;
    return len;    
}

INT16U Radix_AsciiToHex(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    INT8U  dtemp, stemp;
    
    if (len % 2) return 0;
    len /= 2;
    for (i = 1; i <= len; i++) {
        stemp   = Radix_CharToHex(*sptr++);
        dtemp   = stemp << 4;
        *dptr++ = Radix_CharToHex(*sptr++) | dtemp;
    }
    return len;
}
void Radix_UpCaseChar(INT8U *ptr, INT16U len)
{
    INT8U ch;
	for (;len>0;len--) 
	{
       ch = *ptr;
	   if ((ch >= 'a') && (ch <= 'z')) 
	   {
          ch += 'A' - 'a';
		  *ptr = ch;
	   }
	   ptr++;
	}
}

