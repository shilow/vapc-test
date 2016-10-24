/*
 * printf.h
 *
 *  Created on: 08-02-2013
 *      Author: andy
 * http://www.skybeeper.com/index.php/en/english-en/25-tighten-scanf
 */

#ifndef printfH_
#define printfH_

/* this enables float in printf() and costs you about 2kByte ROM */
//#define INCLUDE_FLOAT
/* this enables scan functions and cost you about 600 bytes ROM */
//#define INCLUDE_SCANF

/* public function */
void uprintf(const uint8_t *format, ...);
void lprintf(const uint8_t *format, ...);
void fprintf_(void (*putc)(uint8_t), const uint8_t *format, ...);
void sprintf_(uint8_t *buffer, const uint8_t *format, ...);
#ifdef INCLUDE_SCANF
uint8_t rscanf(const uint8_t* format, ...);
#endif // INCLUDE_SCANF

#endif /* printfH_ */
