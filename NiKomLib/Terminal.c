/*
 * Terminal.c - har hand om gr�nssnittet mellan basen och anv�ndaren.
 */

#include <exec/types.h>
#include <string.h>
#include <limits.h>
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

/* Namn: ConvChrsToAmiga
*  Parametrar: a0 - En pekare till str�ngen som ska konverteras
*              d0 - L�ngden p� str�ngen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Fr�n vilken teckenupps�ttning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returv�rden: Inga
*
*  Beskrivning: Konverterar den inmatade str�ngen till ISO 8859-1 fr�n den
*               teckenupps�ttning som angivits. De tecken som inte finns i
*               ISO 8859-1 ers�tts med ett '?'.
*               F�r CHRS_CP437 och CHRS_MAC8 konverteras bara tecken h�gre
*               �n 128. F�r CHRS_SIS7 konverteras bara tecken h�gre �n 32.
*               Tecken �ver 128 i SIS-str�ngar tolkas som ISO 8859-1.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds AASM LIBConvChrsToAmiga(register __a0 char *str AREG(a0), register __d0 int len AREG(d0),
	register __d1 int chrs AREG(d1), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->IbmToAmiga[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->SF7ToAmiga[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->MacToAmiga[str[x]];
				break;
			default :
				str[x] = convnokludge(str[x]);
		}
	}
}

/* Namn: ConvChrsFromAmiga
*  Parametrar: a0 - En pekare till str�ngen som ska konverteras
*              d0 - L�ngden p� str�ngen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Till vilken teckenupps�ttning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returv�rden: Inga
*
*  Beskrivning: Konverterar den inmatade str�ngen fr�n ISO 8859-1 till den
*               teckenupps�ttning som angivits. De tecken som inte finns i
*               destinationsteckenupps�ttningen ers�tts med ett '?'.
*               F�r CHRS_CP437 och CHRS_MAC8 konverteras bara tecken h�gre
*               �n 128. F�r CHRS_SIS7 konverteras bara tecken h�gre �n 32.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds AASM LIBConvChrsFromAmiga(register __a0 char *str AREG(a0), register __d0 int len AREG(d0),
	register __d1 int chrs AREG(d1), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToIbm[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->AmigaToSF7[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToMac[str[x]];
				break;
		}
	}
}

UBYTE convnokludge(UBYTE tkn) {
	UBYTE rettkn;
	switch(tkn) {
		case 0x86: rettkn='�'; break;
		case 0x84: rettkn='�'; break;
		case 0x94: rettkn='�'; break;
		case 0x8F: rettkn='�'; break;
		case 0x8E: rettkn='�'; break;
		case 0x99: rettkn='�'; break;
		case 0x91: rettkn='�'; break;
		case 0x9b: rettkn='�'; break;
		case 0x92: rettkn='�'; break;
		case 0x9d: rettkn='�'; break;
		default : rettkn=tkn;
	}
	return(rettkn);
}

/* Lookup table for length of UTF-8 characters. */
static const char utf8_extra[64] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

static int parseUTF8(char *dst, const char *src, const int si, const int len) {
  unsigned chr = (unsigned char)src[si];
  int need;

  if((chr & 0xc0) != 0xc0) {
    /* Unexpected continuation byte. */
    *dst = '?';
    return 0;
  }
  need = utf8_extra[chr - 192];
  if(len == INT_MAX) {
    unsigned idx = si+1;
    switch(need) {
    case 5:
      if(src[idx] == '\0') {
        return -5;
      }
      ++idx;
    case 4:
      if(src[idx] == '\0') {

        return -4;
      }
      ++idx;
    case 3:
      if(src[idx] == '\0') {
        return -3;
      }
      ++idx;
    case 2:
      if(src[idx] == '\0') {
        return -2;
      }
      ++idx;
    case 1:
      if(src[idx] == '\0') {
        return -1;
      }
      break;
    }
  } else if(need + 1 > (len - si)) {
    return -(need + 1 - (len - si));
  }

  switch (need) {
  case 1:
    chr &= 0x1f;
    chr <<= 6;
    chr |= src[si + 1] & 0x3f;
    if (chr < 256) {
      *dst = chr;
    } else {
      *dst = '?'; // Non ISO-8859-1
    }
    return 1;
  default:
    *dst = '?'; // Non ISO-8859-1
    break;
  }
  return need;
}

static int noConvCopy(char *dst, const char *src, unsigned len){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    dst[i] = src[i];
  }
  return (int)i;
}

static int conv128Table(char *dst, const char *src, unsigned len,
                        const UBYTE conv[256]){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    if(src[i] >= 128) {
      dst[i] = conv[src[i]];
    } else {
      dst[i] = src[i];
    }
  }
  return (int)i;
}

static int conv32Table(char *dst, const char *src, unsigned len,
                       const UBYTE conv[256]){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    if(src[i] >= 32) {
      dst[i] = conv[src[i]];
    } else {
      dst[i] = src[i];
    }
  }
  return (int)i;
}

static int convNoKludgeToAmiga(char *dst, const char *src, unsigned len){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    dst[i] = convnokludge(src[i]);
  }
  return (int)i;
}

int convUTF8ToAmiga(char *dst, const char *src, unsigned len){
  unsigned si, di;

  for(si=0, di=0; src[si] && si < len; si++, di++) {
    if((src[si] & 0x80)) {
      int ret;
      ret = parseUTF8(dst + di, src, si, len);
      if (ret < 0) {
        return ret;
      }
      si += ret;
    } else {
      dst[di] = src[si];
    }
  }
  return (int)di;
}

/* Name: ConvMBChrsToAmiga
*  Parameters: a0 - Pointer to result string. Must be at least as big
*                   as source string.
*              a1 - Pointer to string that should be converted.
*              d0 - Length of string to convert. If 0, convert until
*                   nul.
*              d1 - Source character set, defined in NiKomLib.h
*
*  Return value: Length of new string, or negative if more bytes are
*                needed to perform conversion.
*
*  Description: Converts source string to ISO-8859-1 from the
*               specified character set. Characters not present in
*               ISO-8859-1 are replaced by '?'. For CHRS_CP437 and
*               CHRS_MAC8 only characters greater than 128 are
*               converted. For CHRS_SIS7 only characters greater than
*               32 are converted.  Character greater than 128 in
*               SIS-strings are interpreted as ISO-8859-1.
*               CHRS_LATIN1 is not converted.
*
*               Note that the destination string is not nul terminated
*               even if len is passed as zero. That must be done by
*               the caller if needed.
*/

int __saveds AASM LIBConvMBChrsToAmiga(register __a0 char *dst AREG(a0),
                                        register __a1 char *src AREG(a1),
                                        register __d0 int len AREG(d0),
                                        register __d1 int chrs AREG(d1),
                                        register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  enum nikom_chrs chrset = chrs;

  if(len == 0) {
    len = INT_MAX;
  }
  switch(chrset) {
  case CHRS_CP437:
    return conv128Table(dst, src, len, NiKomBase->IbmToAmiga);
  case CHRS_CP850:
    return conv128Table(dst, src, len, NiKomBase->CP850ToAmiga);
  case CHRS_CP866:
    return conv128Table(dst, src, len, NiKomBase->CP866ToAmiga);
  case CHRS_SIS7:
    return conv32Table(dst, src, len, NiKomBase->SF7ToAmiga);
  case CHRS_MAC:
    return conv128Table(dst, src, len, NiKomBase->MacToAmiga);
  case CHRS_UTF8:
    return convUTF8ToAmiga(dst, src, len);
  case CHRS_UNKNOWN:
    return convNoKludgeToAmiga(dst, src, len);
  case CHRS_LATIN1:
    break;
  }
  return noConvCopy(dst, src, len);
}

static int convUTF8FromAmiga(char *dst, const char *src,
                             unsigned srclen, unsigned dstlen){
  unsigned si, di;

  if(dstlen == 0) {
    /* We never generate code points with more than 2 bytes. */
    dstlen = srclen * 2;
  }
  for(si=0, di=0; src[si] && si < srclen && di < dstlen; si++, di++) {
    if((src[si] & 0x80)) {
      unsigned char c;

      if (di + 1 >= dstlen) {
        /* Do not truncate in the middle of an UTF-8 character. */
        return (int)di;
      }
      c = src[si];
      dst[di] = 0xc0 | ((unsigned) c >> 6);
      di++;
      dst[di] = 0x80 | (c & 0x3f);
      continue;
    } else {
      dst[di] = src[si];
    }
  }
  return (int)di;
}

/* Name: ConvMBChrsFromAmiga
*  Parameters: a0 - Pointer to result string. Must be at least twice
*                   as big as source string.
*              a1 - Pointer to string that should be converted.
*              d0 - Length of string to convert. If 0, convert until
*                   nul.
*              d1 - Destination character set, defined in NiKomLib.h
*              d2 - Maximum destination length. Must be >= d0.
*                   Ignored if 0.
*
*  Return value: Length of new string.
*
*  Description: Converts source string from ISO-8859-1 from the
*               specified character set. Characters not present in the
*               destination character set are replaced by '?'.  For
*               CHRS_CP437 and CHRS_MAC8 only characters greater than
*               128 are converted. For CHRS_SIS7 only characters
*               greater than 32 are converted. For UTF-8 the
*               destination string may be upto twice as long as the
*               source string.
*               CHRS_LATIN1 is not converted.
*
*               Note that the destination string is not nul terminated
*               even if len is passed as zero. That must be done by
*               the caller if needed.
*/

int __saveds AASM LIBConvMBChrsFromAmiga(register __a0 char *dst AREG(a0),
                                          register __a1 char *src AREG(a1),
                                          register __d0 int srclen AREG(d0),
                                          register __d1 int chrs AREG(d1),
                                          register __d2 int dstlen AREG(d2),
                                          register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  enum nikom_chrs chrset = chrs;

  if(srclen == 0) {
    srclen = INT_MAX;
  }
  switch(chrset) {
  case CHRS_CP437:
    return conv128Table(dst, src, srclen, NiKomBase->AmigaToIbm);
  case CHRS_CP850:
    return conv128Table(dst, src, srclen, NiKomBase->AmigaToCP850);
  case CHRS_CP866:
    return conv128Table(dst, src, srclen, NiKomBase->AmigaToCP866);
  case CHRS_SIS7:
    return conv32Table(dst, src, srclen, NiKomBase->AmigaToSF7);
  case CHRS_MAC:
    return conv128Table(dst, src, srclen, NiKomBase->AmigaToMac);
  case CHRS_UTF8:
    return convUTF8FromAmiga(dst, src, srclen, dstlen);
  case CHRS_LATIN1:
  case CHRS_UNKNOWN:
    break;
  }
  return noConvCopy(dst, src, srclen);
}
