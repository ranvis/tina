/*
    TiMidity++ -- MIDI to WAVE converter and player
	Copyright (C) 2004 Takaya Nogami <t-nogami@happy.email.ne.jp>
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    macosx_synth_c.m
    MacOS X, Cocoa synthesizer mode controller.
        Copyright (c) 2004 Takaya Nogami <t-nogami@happy.email.ne.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c
        portmidisyn_c.c
*/



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef __W32__
#include <stdio.h>
#include <termios.h>
//#include <term.h>
#include <unistd.h>
#endif

#include <CoreMIDI/MIDIServices.h>

#import "timidity.h";
#include "rtsyn.h"
#import "macosx_SynthController.h"
#import "macosx_synth_c.h"

extern int volatile stream_max_compute;	// play_event() ‚Ì compute_data() ‚ÅŒvŽZ‚ð‹–‚·Å‘åŽžŠÔ
int seq_quit=~0;


static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);
static void ctl_pass_playing_list(int n, char *args[]);


/**********************************/
/* export the interface functions */

#define ctl macosx_syn_control_mode

ControlMode ctl=
{
    "MacOSX Synthesizer interface", 'm',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

/***********************************************************************/
static int32 event_time_offset;

#define MAX_PORT 4
extern int rtsyn_portnumber;
unsigned int portID[MAX_PORT];
char  rtsyn_portlist[32][80];
int rtsyn_nportlist;

ChannelStatus_t ChannelStatus[MAX_CHANNELS];

/***********************************************************************/

static void ctl_program(int ch, int prog, char *comm, unsigned int banks)
{
    int val;
    int bank;

    if(prog != CTL_STATUS_UPDATE)
    {
        bank = banks & 0xff;
        ChannelStatus[ch].prog = prog;
        ChannelStatus[ch].bank = bank;
        ChannelStatus[ch].bank_lsb = (banks >> 8) & 0xff;
        ChannelStatus[ch].bank_msb = (banks >> 16) & 0xff;
        ChannelStatus[ch].comm = (comm ? comm : "");
    } else {
        prog = ChannelStatus[ch].prog;
        bank = ChannelStatus[ch].bank;
    }
    ChannelStatus[ch].last_note_on = 0.0;       /* reset */

    [macosx_controller refresh];
}

static void ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
        /* EAW -- if not a true KAR lyric, ignore \r, treat \n as \r */
        if (*lyric != ME_KARAOKE_LYRIC) {
            while (strchr(lyric, '\r')) {
            	*(strchr(lyric, '\r')) = ' ';
            }
	    if (ctl.trace_playing) {
		while (strchr(lyric, '\n')) {
		    *(strchr(lyric, '\n')) = '\r';
		}
            }
        }

	cmsg(CMSG_INFO, VERB_NORMAL, "%s", lyric + 1);
    }
}


/***********************************************************************/

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
	ctl.opened = 1;
	ctl.flags &= ~(CTLF_LIST_RANDOM|CTLF_LIST_SORT);

	return 0;
}

static void ctl_close(void)
{
  if(seq_quit==0){
  	rtsyn_synth_stop();
  	rtsyn_close();
  	seq_quit=~0;
  }	
  ctl.opened=0;
}

static int ctl_read(int32 *valp)
{
    return RC_NONE;
}


static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;
    char buff[1024];

    if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	ctl.verbosity<verbosity_level)
            return 0;
    if( strlen(fmt)>1000 ){
	return 0; //too long
    }
    //indicator_mode = INDICATOR_CMSG;
    va_start(ap, fmt);
    vsnprintf(buff, 1023, fmt, ap);
    strcat(buff, "\n");

    /*if(!ctl.opened){
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, NLS);
            NSRunAlertPanel(
                [NSString stringWithCString:buff], @"", 
                @"OK", nil, nil);
    }
    else*/{
        if(  type==CMSG_FATAL){
            
            NSRunAlertPanel(
                [NSString stringWithCString:buff], @"",
                @"OK", nil, nil);
        }
	//printf(buff);
        [macosx_controller message:buff];
    }
    va_end(ap);
    return 0;

}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_PROGRAM:
        ctl_program((int)e->v1, (int)e->v2, (char *)e->v3, (unsigned int)e->v4);
	break;
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
    }
}

static void doit(void);


static void ctl_pass_playing_list(int n, char *args[])
{
    int i;
    unsigned int port=0 ;
    
    rtsyn_get_port_list();

#ifndef IA_W32G_SYN
    if(n > MAX_PORT ){
	    printf( "Usage: timidity -iW [Midi interface No s]\n");
	    return;
    }
#endif

    
    rtsyn_portnumber=MAX_PORT;
    

    sleep(1); // why?

    for(port=0;port<rtsyn_portnumber;port++){
	    portID[port]=port;
    }

    rtsyn_init();

    if(0!=rtsyn_synth_start()){
	    seq_quit=0;
	    while(seq_quit==0) {
		    doit();
	    }
	    rtsyn_synth_stop();
    }

    rtsyn_close();

    return;
}


#ifndef IA_W32G_SYN




static void doit(void)
{

	while(seq_quit==0){

		if(/*kbhit()*/ 0){
			switch(readch()){
				case 'Q':
				case 'q':
					seq_quit=~0;
				break;
				case 'm':
					rtsyn_gm_reset();
				break;
				case 's':
					rtsyn_gs_reset();
				break;
				case 'x':
					rtsyn_xg_reset();
				break;
				case 'c':
					rtsyn_normal_reset();
				break;
				case 'M':
					rtsyn_gm_modeset();
				break;
				case 'S':
					rtsyn_gs_modeset();
				break;
				case 'X':
					rtsyn_xg_modeset();
				break;
				case 'N':
					rtsyn_normal_modeset();
				break;
			}
		}
		rtsyn_play_some_data();
		rtsyn_play_calculate();
		usleep(100);
	}
}

#endif /* !IA_W32G_SYN */
