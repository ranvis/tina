/*
    TiMidity++ -- MIDI to WAVE converter and player
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   rtsyn_macosx_vinput.c - MacOSX CoreMIDI virtual input synthesizer interface
        Copyright (c) 2004 Takaya Nogami <t-nogami@happy.email.ne.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c
        rtsyn_portmidi.c

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#include <stdio.h>

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <signal.h>

#include <pthread.h>
#include <CoreMIDI/MIDIServices.h>

#include "server_defs.h"


#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "recache.h"
#include "output.h"
#include "aq.h"
#include "timer.h"

#include "rtsyn.h"

int rtsyn_portnumber=MAX_PORT;
unsigned int portID[MAX_PORT];
char rtsyn_portlist[32][80];
int rtsyn_nportlist;

#define MAX_EXBUF 20
#define BUFF_SIZE 512



static unsigned int InNum;

#define PMBUFF_SIZE 8192
#define EXBUFF_SIZE 512
static char    sysexbuffer[EXBUFF_SIZE];

/*********************************************************************/
#define TmStreamBufNum 102400
typedef struct {
  uint32  current;
  uint32  next;
  uint8   buf[TmStreamBufNum];
}TmStream;

TmStream  tmstream;
pthread_mutex_t tmstream_mutex = PTHREAD_MUTEX_INITIALIZER;

void TmQueue( int port, const MIDIPacket *p)
{
  int  used;
  int  room;


  pthread_mutex_lock(&tmstream_mutex);

  used = tmstream.next - tmstream.current;
  if( used <0 ){ used+=TmStreamBufNum; }
  room = TmStreamBufNum - used;

  if(p->length>=256){
    printf("too big packet\n"); // and do nothing

  }else if(room < p->length +3){
    printf("full buffer\n"); //and do nothing
    fflush(stdout);
  }else{  //go!
    int i;

    tmstream.buf[tmstream.next] = port; 
    tmstream.next++; tmstream.next %= TmStreamBufNum;

    tmstream.buf[tmstream.next] = p->length;
    tmstream.next++; tmstream.next %= TmStreamBufNum;

    for( i=0; i<p->length; i++ ){
      tmstream.buf[tmstream.next] = p->data[i];
      tmstream.next++; tmstream.next %= TmStreamBufNum;
    }
  }

  pthread_mutex_unlock(&tmstream_mutex);
}

int TmDequeue( int *portp, uint8 buf[256])
{
  int     used;
  int     room;
  int     len=0, i;

  pthread_mutex_lock(&tmstream_mutex);

  used = tmstream.next - tmstream.current;
  if( used <0 ){ used+=TmStreamBufNum; }
  room = TmStreamBufNum - used;

  if( used == 0 ){
    len = 0;
    goto finally;
  }
  
  *portp = tmstream.buf[tmstream.current];
  tmstream.current++; tmstream.current %= TmStreamBufNum;

  len = tmstream.buf[tmstream.current];
  tmstream.current++; tmstream.current %= TmStreamBufNum;
  
  for( i=0; i<len; i++){
    buf[i] = tmstream.buf[tmstream.current];
    tmstream.current++; tmstream.current %= TmStreamBufNum;
  }
  
 finally:
  pthread_mutex_unlock(&tmstream_mutex);
  return len;
}
/*********************************************************************/

void rtsyn_get_port_list()
{
  //printf("rtsyn_get_port_list");
    int i;
    rtsyn_nportlist=4;
    for( i=0; i<rtsyn_nportlist; i++){
        sprintf(rtsyn_portlist[i],"%d:%s%d",i+1,"macosx CoreMIDI virtural input",i+1);
    }
    return;
}

static void	MyReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
  unsigned int j;
  int i;
  int port = (int)refCon;

	//if (gOutPort != NULL && gDest != NULL) {
		MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	// remove const (!)
		for ( j = 0; j < pktlist->numPackets; ++j) {
			for (  i = 0; i < packet->length; ++i) {
			  //printf("que:%02X ", packet->data[i]);
				//fflush(stdout);
				// rechannelize status bytes
				//if (packet->data[i] >= 0x80 && packet->data[i] < 0xF0)
				//	packet->data[i] = (packet->data[i] & 0xF0) | gChannel;
			}
			//printf("\n");
			//fflush(stdout);
			TmQueue( port, packet );
			packet = MIDIPacketNext(packet);
		}

	//}
}

int rtsyn_synth_start(){
	int i;
	unsigned int port;
	MidiEvent ev;
	OSStatus result;
	MIDIClientRef client = NULL;

	rtsyn_reset();
	rtsyn_system_mode=DEFAULT_SYSTEM_MODE;
	change_system_mode(rtsyn_system_mode);
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE; //GM is mor better ???
	rtsyn_play_event(&ev);
	



	result = MIDIClientCreate( CFSTR("TiMiditySynth"), NULL, 0, 
							&client );
	//printf("result = %d\n", (int)result);

	MIDIEndpointRef outDest = NULL;


	MIDIDestinationCreate(  client, CFSTR("TiMiditySynth 1"), MyReadProc, 
                                                (void*)0, &outDest );
	MIDIDestinationCreate(  client, CFSTR("TiMiditySynth 2"), MyReadProc, 
                                                (void*)1, &outDest );
	MIDIDestinationCreate(  client, CFSTR("TiMiditySynth 3"), MyReadProc, 
                                                (void*)2, &outDest );
	MIDIDestinationCreate(  client, CFSTR("TiMiditySynth 4"), MyReadProc, 
                                                (void*)3, &outDest );

	return ~0;

pmerror:

	//ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return 0;

}


void rtsyn_synth_stop(){

	rtsyn_stop_playing();
//	play_mode->close_output();
	rtsyn_midiports_close();

	return;
pmerror:
	//Pm_Terminate();
	//ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return ;
}

void rtsyn_midiports_close(void){
	unsigned int port;

	for(port=0;port<rtsyn_portnumber;port++){
	  //pmerr=Pm_Abort(midistream[port].stream);
//		if( pmerr != pmNoError ) goto pmerror;
	}
	//Pm_Terminate();
}

static inline uint32 ev2message( uint32 len, uint8 buf[])
{
  switch(len){

  case 0:
    return 0;

  case 1:
    return buf[0];

  case 2:
    return buf[0]+(buf[1]<<8);
    break;

  case 3:
    return buf[0]+(buf[1]<<8)+(buf[2]<<16);
    break;
    
  default:
    return buf[0];
  }
}

int rtsyn_play_some_data (void)
{
  int played;	
  int j,port=0,exlen,data,shift;
  long pmlength,pmbpoint;
  uint8  buf[256];

	
  played=0;
  do{
    //for(port=0;port<rtsyn_portnumber;port++){
  
    do{
      pmlength = TmDequeue(&port, buf);
      if(pmlength<0) goto pmerror;
      if(pmlength==0){
	break;
      }
      //printf("get:%02x \n", buf[0]);
      played=~0;
      if( 1==rtsyn_play_one_data (port, ev2message(pmlength,buf)) ){	
	rtsyn_play_one_sysex(buf,pmlength );
      }
      
    }while(pmlength>0);
    //}
    //troiaezu
    usleep(100);
    //  break;
  }while(rtsyn_reachtime>get_current_calender_time());
  
  return played;
 pmerror:
	//Pm_Terminate();
	//ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
  return 0;
}

