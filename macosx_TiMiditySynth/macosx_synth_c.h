//
//  macosx_synth_c.h
//  TiMiditySynth
//
//  Created by ñÏè„ãMç∆ on Sun Jul 25 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "timidity.h";


typedef struct
{
    int mute, bank, bank_lsb, bank_msb, prog;
    int tt, vol, exp, pan, sus, pitch, wheel;
    int is_drum;
    int bend_mark;

    double last_note_on;
    char *comm;
}ChannelStatus_t;

extern ChannelStatus_t ChannelStatus[MAX_CHANNELS];

#define CTL_STATUS_UPDATE -98
#define CTL_STATUS_INIT -99
