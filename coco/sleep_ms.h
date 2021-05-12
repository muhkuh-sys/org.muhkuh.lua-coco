#ifndef __SLEEP_MS_H__
#define __SLEEP_MS_H__

/** \file slee_ms.h 
	\brief defines a sleep macro which distinguishes between windows / linux
	
This file provides a sleep_ms(ms) macro which calls the right sleep function depending on the operating system
*/

#if defined(_WIN32)
#       define sleep_ms(ms) Sleep(ms)
#else
#       include <unistd.h>
#       define sleep_ms(ms) usleep(1000*ms)
#endif


#endif  /* __SLEEP_MS_H__ */

