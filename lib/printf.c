/** \file printf.c
 * Simplified printf() and sprintf() implementation - prints formatted string to
 * USART (or whereever). Most common % specifiers are supported. It costs you about
 * 3k FLASH memory - without floating point support it uses only 1k ROM!
 * \author Freddie Chopin, Martin Thomas, Marten Petschke and many others
 * \date 16.2.2012
 * reduced scanf() added by Andrzej Dworzynski on the base of reduced sscanf() written by
 * some nice gay from this post: http://www.fpgarelated.com/usenet/fpga/show/61760-1.php
 * thanks to everybody:)
 * \date 12.2.2013
 * http://www.skybeeper.com/index.php/en/english-en/25-tighten-scanf
 *
 * shilow: 1155 bytes without float. 590 bytes without scan functions.
 */

/******************************************************************************
* chip: STM32F10x
* compiler: arm-none-eabi-gcc 4.6.0
*
* global functions:
*   int printf(const uint8_t *format, ...)
*   int sprintf(uint8_t *buffer, const uint8_t *format, ...)
*  int rscanf(const uint8_t* format, ...)
*
* STM32 specific functions:
*   Usart1Init(void)
*  void Usart1Put(uint8_t);                // blocking put uint8_t, used by printf()
*  uint8_t Usart1Get(void);         //used by rscanf()
* set up scanf buffer by changing scanf_buff_size variable

* local functions:
*   int putc_strg(int uint8_tacter, printffile_t *stream)
*   int vfprintf(printffile_t *stream, const uint8_t *format, va_list arg)
*   void int32_t_itoa (int32_t val, int radix, int len, vfprintfstream *stream)
*  static int rsscanf(const uint8_t* str, const uint8_t* format, va_list ap)
******************************************************************************/

/*
+=============================================================================+
| includes
+=============================================================================+
*/

#include <stdarg.h>     // (...) parameter handling
#include <stdlib.h>     //NULL pointer definition

#include "stm32f0xx.h"  // only this headerfile is used
#include "printf.h"

//#define assert_param(expr) ((void)0) /*dummy to make the stm32 header work*/

/*
+=============================================================================+
| global declarations
+=============================================================================+
*/
//in file printf.h
/*
+=============================================================================+
| local declarations
+=============================================================================+
*/

uint8_t *sprintfBuffer;
static void putc_strg(uint8_t); // the put() function for sprintf()
static void vfprintf_(void (*putc)(uint8_t), const uint8_t *format, va_list arg); //generic print
static void long_itoa(int32_t val, int8_t radix, int8_t len, void (*putc)(uint8_t)); //heavily used by printf
#ifdef INCLUDE_SCANF
uint8_t scanfBuffSize = 12; //chang this if needed
static int16_t rsscanf(const uint8_t* str, const uint8_t* format, va_list ap);//used by rscanf
#endif // INCLUDE_SCANF

extern void LCD_Print_Char(uint8_t c);
extern void USART_Put_Char(uint8_t c);
#ifdef INCLUDE_SCANF
extern uint8_t USART_Get_Char(void);
#endif // INCLUDE_SCANF

/*
+=============================================================================+
| sample main()  file
+=============================================================================+
*/
/*
void main(void)
{
  uint8_t my_buff[12];
    int i;

    Usart1Init();
    printf(" USART1 Test\r\n");

      printf("Enter your family name: ");
      rscanf ("%s",my_buff);
      printf("%s\r\n",my_buff);
      printf("Enter your age: ");
      rscanf ("%d",&i);
      printf("%d\r\n",i);
      printf("Mr. %s, %d years old.\r\n",my_buff,i);
      printf ("Enter a hexadecimal number:\r\n");
      rscanf("%x",&i);
      printf("You have entered %x (%d).\r\n",i,i);
  while(1);
}

*/


/*
+=============================================================================+
| global functions
+=============================================================================+
*/
void lprintf(const uint8_t *format, ...) {
  va_list arg;

  va_start(arg, format);
  vfprintf_((&LCD_Print_Char), format, arg);
  va_end(arg);
}

void uprintf(const uint8_t *format, ...) {
  va_list arg;

  va_start(arg, format);
  vfprintf_((&USART_Put_Char), format, arg);
  va_end(arg);
}

void fprintf_(void (*putc)(uint8_t), const uint8_t *format, ...) {
  va_list arg;

  va_start(arg, format);
  vfprintf_(putc, format, arg);
  va_end(arg);
}

void sprintf_(uint8_t *buffer, const uint8_t *format, ...) {
  va_list arg;

  sprintfBuffer=buffer;   //Pointer auf einen String in Speicherzelle abspeichern

  va_start(arg, format);
  vfprintf_((&putc_strg), format, arg);
  va_end(arg);

  *sprintfBuffer ='\0';             // append end of string
}

#ifdef INCLUDE_SCANF
//Reads data from usart1  and stores them according to parameter format
//into the locations given by the additional arguments, as if
// scanf was used

// Reduced version of scanf (%d, %x, %c, %n are supported)
// %d dec integer (E.g.: 12)
// %x hex integer (E.g.: 0xa0)
// %b bin integer (E.g.: b1010100010)
// %n hex, de or bin integer (e.g: 12, 0xa0, b1010100010)
// %c any uint8_tacter
//buffer support 12 bytes

uint8_t rscanf(const uint8_t* format, ...) {
  va_list args;
  va_start( args, format );
  uint8_t count = 0;
  uint8_t ch = 0;
  uint8_t buffer[scanfBuffSize];

  sprintfBuffer = buffer;

  while(count <= scanfBuffSize ) { //get string
    count++;
    ch = USART_Get_Char();

    if(ch != '\n' && ch != '\r') {
      *sprintfBuffer++ = ch;
    } else {
      break;
    }
  }

  *sprintfBuffer = '\0';//end of string

  sprintfBuffer = buffer;
  count =  rsscanf(sprintfBuffer, format, args);
  va_end(args);

  return count;
}
#endif // INCLUDE_SCANF

/*
+=============================================================================+
| local functions
+=============================================================================+
*/
// putc_strg() is the putc()function for sprintf()
static void putc_strg(uint8_t uint8_tacter) {
  *sprintfBuffer = (uint8_t)uint8_tacter;  // just add the uint8_tacter to buffer
  sprintfBuffer++;
}

/*--------------------------------------------------------------------------------+
 * vfprintf()
 * Prints a string to stream. Supports %s, %c, %d, %ld %ul %02d %i %x  %lud  and %%
 *     - partly supported: int32_t int32_t, float (%l %f, %F, %2.2f)
 *     - not supported: double float and exponent (%e %g %p %o \t)
 *--------------------------------------------------------------------------------+
*/
static void vfprintf_(void (*putc)(uint8_t), const uint8_t* str,  va_list arp) {
  int d, r, w, s, l;  //d=uint8_t, r = radix, w = width, s=zeros, l=int32_t
  uint8_t *c;            // for the while loop only
  //const uint8_t* p;
#ifdef INCLUDE_FLOAT
  float f;
  int32_t m, mv, p, w2;
#endif

  while ((d = *str++) != 0) {
    if (d != '%') {//if it is not format qualifier
      (*putc)(d);
      continue;//get out of while loop
    }

    d = *str++;//if it is '%'get next uint8_t
    w = r = s = l = 0;

    if (d == '%') {//if it is % print silmpy %
      (*putc)(d);
      d = *str++;
    }

    if (d == '0') {
      d = *str++;
      s = 1;  //padd with zeros
    }

    while ((d >= '0') && (d <= '9')) {
      w += w * 10 + (d - '0');
      d = *str++;
    }

    if (s) {
      w = -w;  //padd with zeros if negative
    }

#ifdef INCLUDE_FLOAT
    w2 = 0;

    if (d == '.') {
      d = *str++;
    }

    while ((d >= '0') && (d <= '9')) {
      w2 += w2 * 10 + (d - '0');
      d = *str++;
    }
#endif

    if (d == 's') {// if string
      c = va_arg(arp, uint8_t*); //get buffer addres

      while (*c) {
        (*putc)(*(c++)); //write the buffer out
      }
      continue;
    }

    if (d == 'c') {
      (*putc)((uint8_t)va_arg(arp, int));
      continue;
    }

    if (d == 'u') { // %ul
      r = 10;
      d = *str++;
    }

    if (d == 'l') { // int32_t =32bit
      l = 1;

      if (r==0) {
        r = -10;
      }

      d = *str++;
    }

    if (d == 'u') {
      r = 10; // %lu, %llu
    } else if (d == 'd' || d == 'i') {
      if (r==0) {
        r = -10; //can be 16 or 32bit int
      }
    } else if (d == 'X' || d == 'x') {
      r = 16; // 'x' added by mthomas
    } else if (d == 'b') {
      r = 2;
#ifdef INCLUDE_FLOAT
		} else if (d == 'f' || d == 'F') {
			f = va_arg(arp, double);
			if (f >= 0.0) {
        r = 10;
        mv = f;
        m = mv;
			} else {
        r = -10;
        mv = f;
        f = -f;
        m = f; // f and m are always positive
			}
			long_itoa(mv, r, w, (putc));
			if (w2!=0) {
        putc('.');
        f=f-m;
        w=-w2; p=1;
        while (w2--) {
          p = p*10;
        }
        m=f*p;
        long_itoa(m, 10, w, (putc));
			}
			l=3; //do not continue with long
#endif
    } else {
      str--; // normal character
    }

    if (!r) {
      continue;
    }

    if (l==0) {
      if (r > 0) { //unsigned
        long_itoa((uint32_t)va_arg(arp, int), r, w, (putc)); //needed for 16bit int, no harm to 32bit int
      } else { //signed
        long_itoa((int32_t)va_arg(arp, int), r, w, (putc));
      }
    } else if (l==1) { // long = 32bit
      long_itoa((int32_t)va_arg(arp, int32_t), r, w, (putc)); //no matter if signed or unsigned
    }
  }
}

static void long_itoa(int32_t val, int8_t radix, int8_t len, void (*putc)(uint8_t)) {
  uint8_t c, sgn = 0, pad = ' ';
  uint8_t s[12];
  uint8_t i = 0;


  if (radix < 0) {
    radix = -radix;

    if (val < 0) {
      val = -val;
      sgn = '-';
    }
  }

  if (len < 0) {
    len = -len;
    pad = '0';
  }

  if (len > 20) {
    return;
  }

  do {
    c = (uint8_t)((uint32_t)val % radix); //cast!

    if (c >= 10) {
      c += ('A'-10);  //ABCDEF
    } else {
      c += '0';  //0123456789
    }

    s[i++] = c;
    val = (uint32_t)val / radix; //cast!
  } while (val);

  if (sgn) {
    s[i++] = sgn;
  }

  while (i < len) {
    s[i++] = pad;
  }

  do {
    (*putc)(s[--i]);
  } while (i);
}

#ifdef INCLUDE_SCANF
// Reduced version of sscanf (%d, %x, %c, %n are supported)
// %d dec integer (E.g.: 12)
// %x hex integer (E.g.: 0xa0)
// %b bin integer (E.g.: b1010100010)
// %n hex, de or bin integer (e.g: 12, 0xa0, b1010100010)
// %c any uint8_tacter
//

static int16_t rsscanf(const uint8_t* str, const uint8_t* format, va_list ap)
{
  //va_list ap;
  int16_t value, tmp;
  int16_t count;
  int16_t pos;
  uint8_t neg, fmt_code;
  const uint8_t* pf;
  uint8_t* sval;
  //va_start(ap, format);

  for (pf = format, count = 0; *format != 0 && *str != 0; format++, str++) {
    while (*format == ' ' && *format != 0) {
      format++;  //
    }

    if (*format == 0) {
      break;
    }

    while (*str == ' ' && *str != 0) {
      str++; //increment pointer of input string
    }

    if (*str == 0) {
      break;
    }

    if (*format == '%') { //recognize how to format
      format++;

      if (*format == 'n') {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { //if in str sth like 0xff
          fmt_code = 'x';
          str += 2;
        } else if (str[0] == 'b') {
          fmt_code = 'b';
          str++;
        } else {
          fmt_code = 'd';
        }
      } else {
        fmt_code = *format;  //it is format letter
      }

      switch (fmt_code) {
        case 'x':
        case 'X':
          for (value = 0, pos = 0; *str != 0; str++, pos++) {
            if ('0' <= *str && *str <= '9') {
              tmp = *str - '0';
            } else if ('a' <= *str && *str <= 'f') {
              tmp = *str - 'a' + 10;
            } else if ('A' <= *str && *str <= 'F') {
              tmp = *str - 'A' + 10;
            } else {
              break;
            }

            value *= 16;
            value += tmp;
          }

          if (pos == 0) {
            return count;
          }

          *(va_arg(ap, int16_t*)) = value;
          count++;
          break;

        case 'b':
          for (value = 0, pos = 0; *str != 0; str++, pos++) {
            if (*str != '0' && *str != '1') {
              break;
            }

            value *= 2;
            value += *str - '0';
          }

          if (pos == 0) {
            return count;
          }

          *(va_arg(ap, int16_t*)) = value;
          count++;
          break;

        case 'd':
          if (*str == '-') {
            neg = 1;
            str++;
          } else {
            neg = 0;
          }

          for (value = 0, pos = 0; *str != 0; str++, pos++) {
            if ('0' <= *str && *str <= '9') {
              value = value*10 + (int16_t)(*str - '0');
            } else {
              break;
            }
          }

          if (pos == 0) {
            return count;
          }

          *(va_arg(ap, int16_t*)) = neg ? -value : value;
          count++;
          break;

        case 'c':
          *(va_arg(ap, uint8_t*)) = *str;
          count++;
          break;

        case 's':
          sval = va_arg(ap, uint8_t*);

          while(*str) {
            *sval++ = *str++;
            count++;
          }

          *sval = (uint8_t *)NULL;

          break;

        default:
          return count;
      }
    } else {
      if (*format != *str) { //
        break;
      }
    }
  }

  return count;
}
#endif // INCLUDE_SCANF
/******************************************************************************
* END OF FILE
******************************************************************************/
