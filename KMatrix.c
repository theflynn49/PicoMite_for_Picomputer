
/***********************************************************************************************************************
PicoMite MMBasic

KMatrix.c

written 2025 by theflynn49 (no copyright on what I wrote)

Parts stolen from the PicoMite project, for these, this licence applies :

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed 
    on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed 
    by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software 
    without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

************************************************************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "configuration.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/adc.h"
#include "hardware/exception.h"
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/timer.h"
#include "hardware/vreg.h"
#include "hardware/structs/pads_qspi.h"
#include "pico/unique_id.h"
#include "hardware/pwm.h"

#define BOUNCE_TIME 20000L

#define ROW_SZ  6
#define COL_SZ  6

static uint32_t kb_state = 0 ;
static uint64_t timenow = 0L ;
static int last_c = -1 ;

// The basic keymap
static __uint8_t key_table [3]  [ROW_SZ * COL_SZ] = {
	{
	  ' ',   '.', 'm', 'n', 'b', DOWN,
	  ENTER, 'l', 'k', 'j', 'h', LEFT,
	  'p',   'o', 'i', 'u', 'y', UP,
	  BKSP,  'z', 'x', 'c', 'v', RIGHT,
	  'a',   's', 'd', 'f', 'g', ESC,
	  'q',   'w', 'e', 'r', 't', ALT 
	},

	{
	  '_', ',', '>', '<', '"', '{',
	  '~', '-', '*', '&', '+', '[',
	  '0', '9', '8', '7', '6', '}',
	  '=', '(', ')', '?', '/', ']',
	  '!', '@', '#', '$', '%', '\\',
	  '1', '2', '3', '4', '5', ALT
	},

	{
	  ':',   ';', 'M', 'N', 'B', DOWN,
	  ENTER, 'L', 'K', 'J', 'H', LEFT,
	  'P',   'O', 'I', 'U', 'Y', UP,
	  BKSP,  'Z', 'X', 'C', 'V', RIGHT,
	  'A',   'S', 'D', 'F', 'G', TAB,
	  'Q',   'W', 'E', 'R', 'T', ALT 
	}
};


// board.GP1, board.GP2, board.GP3, board.GP4, board.GP5, board.GP14)
// board.GP6, board.GP9, board.GP15, board.GP8, board.GP7, board.GP22

static uint cols[COL_SZ] = { 1, 2, 3, 4, 5, 14 } ;
static uint rows[ROW_SZ] = { 6, 9, 15, 8, 7, 22 } ;

#define mask_all 0x40c3fe 

int __not_in_flash_func(getMatrix)(int intr_process) {
    int c=-1;
    int c0=-1 ;
    int c1=-1 ;
    int _i, _j, _r, _sft, _cw ;
    int _w[ROW_SZ] ;
    char _sdebug[25] ;
    strcpy(_sdebug, "                     ") ;

    if (ExtCurrentConfig[11] < EXT_COM_RESERVED) { // are we initialized ?
    	gpio_init_mask(mask_all) ;
    	gpio_set_dir_in_masked(mask_all) ;
    	for (_i=0; _i<COL_SZ; ++_i) {
	    gpio_set_pulls(cols[_i], true, false) ;
    	}
    	for (_i=0; _i<ROW_SZ; ++_i) {
	    gpio_set_pulls(rows[_i], false, false) ;
    	}
	// do not reserve GP1 and GP4, since audio does GP1 and LCDPANEL gets GP4 but doesn't use it.
	// numbers below are PIN numbers, not GPIO's.
	ExtCfg(4, EXT_COM_RESERVED, 0);
	ExtCfg(5, EXT_COM_RESERVED, 0);
	ExtCfg(7, EXT_COM_RESERVED, 0);
	ExtCfg(9, EXT_COM_RESERVED, 0);
	ExtCfg(10, EXT_COM_RESERVED, 0);
	ExtCfg(11, EXT_COM_RESERVED, 0);
	ExtCfg(12, EXT_COM_RESERVED, 0);
	ExtCfg(19, EXT_COM_RESERVED, 0);
	ExtCfg(20, EXT_COM_RESERVED, 0);
	ExtCfg(29, EXT_COM_RESERVED, 0);
    }
    if (timenow!=0L && timenow>time_us_64()) return -1 ; // debouncing
    timenow=0L ; 
    for (_i=0; _i<ROW_SZ; ++_i) {
	    gpio_set_dir(rows[_i], GPIO_OUT);
	    gpio_put(rows[_i], 0) ;
	    sleep_us(10);
	    _r = gpio_get_all() & 0x403E ; // bits 1, 2, 3, 4, 5, 14
	    _r = ((_r&0x3e)>>1) | ((_r>>9)&0x20) ; 
	    gpio_put(rows[_i], 1) ;
	    gpio_set_dir(rows[_i], GPIO_IN);
	    _w[_i]=_r ;
    //	    _j = (_r>>4) & 0x000F ; if (_j >= 10) _sdebug[_i*3] = 0x41 + _j - 10; else _sdebug[_i*3] = 0x30 + _j ; 
    //	    _j = _r & 0x000F ; if (_j >= 10) _sdebug[_i*3+1] = 0x41 + _j - 10; else _sdebug[_i*3+1] = 0x30 + _j ; 
    }
    // debug
    //for (_i=0; _i<ROW_SZ; ++_i)
      //if (_w[_i]!=0x3F) { MMPrintString(_sdebug) ; return c ; }
    //
    _sft = _w[5] & 0x20 ; // shift key 
    _w[5] |= 0x20 ;
    for (_i=0; _i<ROW_SZ; ++_i) if (_w[_i] != 0x3F) {
	  _cw = _w[_i] ;
	  for (_j=0; _j<COL_SZ; ++_j)
	  {
		  if ((_cw & 1) == 0)
		  {
		    c = key_table [kb_state] [_i*COL_SZ+_j] ;
		    c0 = key_table [0] [_i*COL_SZ+_j] ;
		    c1 = key_table [1] [_i*COL_SZ+_j] ;
		    break ;
		  } 
		  _cw >>= 1 ;
	  }
	  if (c != -1) break ;
    } 
    // internal KB commands
    if (_sft==0) {
	  if (c0==UP) { kb_state=2 ; c=-1 ; }
	  else if (c0==DOWN) { kb_state=0 ; c=-1 ; }
	  else if (c0==RIGHT) { kb_state=1 ; c=-1 ; }
	  else { 
		  c=c1 ;
		 if (kb_state==1) 
		 {
			 if ((c0>='a')&&(c0<='z')) c=c0-0x60 ; else c=c0 ;
		 }
	  } 
    }
    if ((last_c != -1) && (c == -1)) {
	timenow=time_us_64()+BOUNCE_TIME;  // debouncing on key release
    }
    if (c==-1) last_c=-1 ;
    if (last_c==c) c=-1 ; 
    if ((last_c!=c)&&(c!=-1))
    {
	timenow=time_us_64()+BOUNCE_TIME; // debouncing on key press
    	last_c=c ;
    	/*
    	if (c!=-1) {
    		_sdebug[0]=0x30+(_sft>>5) ; _sdebug[1]=c ;_sdebug[2]=last_c; _sdebug[3]=c1; _sdebug[4]=' '; _sdebug[5]=0; MMPrintString(_sdebug) ;
    	}
    	*/
    	// Interrupt magic
    	if (c==keyselect && KeyInterrupt!=NULL){
        	Keycomplete=1;
    	} else if (intr_process) {
		ConsoleRxBuf[ConsoleRxBufHead]  = c;   // store the byte in the ring buffer
		ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
		if(ConsoleRxBufHead == ConsoleRxBufTail) {                           // if the buffer has overflowed
			ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE; // throw away the oldest char
 		}
	}
	// Break magic
    	if (BreakKey && c == BreakKey)
    	{                                      // if the user wants to stop the progran
        	MMAbort = true;                      // set the flag for the interpreter to see
        	ConsoleRxBufHead = ConsoleRxBufTail; // empty the buffer
                                             // break;
    	}
    }
    /*
    */
    return c;
}

