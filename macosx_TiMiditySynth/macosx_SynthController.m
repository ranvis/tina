//
//  SynthController.M
//  TiMiditySynth
//
//  Created by ñÏè„ãMç∆ on Sun Jul 25 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//


#ifdef HAVE_CONFIG_H
#import "config.h"
#endif /* HAVE_CONFIG_H */

#import <unistd.h>

#import "timidity.h"
#import "common.h"
#import "instrum.h"
#import "playmidi.h"
#import "resample.h"
#import "controls.h"
#import "output.h"
#import "aq.h"

#import "macosx_SynthController.h"

int mac_main(int argc, char *argv[]);

@implementation SynthController

SynthController * macosx_controller=NULL;

- (void)message:(const char*)buf
{
    NSString *str = [NSString stringWithCString:buf];
    
    if( str==NULL ){
	str=@"(macosx_controller:message err)\n";
    }
    [cmsg replaceCharactersInRange:NSMakeRange([[cmsg string] length], 0)
                                   withString:str];
    [cmsg scrollRangeToVisible:NSMakeRange([[cmsg string] length], 0)];
}


- (void)core_thread:(id)arg
{
    NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];
    char *argv[] = {"timidity", "-c", "./timidity.cfg", "-im"};
    chdir([[[NSBundle mainBundle] bundlePath] UTF8String]);
    chdir("..");
    
    opt_reverb_control = 0;
    set_current_resampler(RESAMPLE_NONE);

    mac_main(4, argv);

    [pool release];
    exit(0);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    auto_reduce_polyphony=0;
    reduce_voice_threshold = 0;

    macosx_controller = self;
    //macosx_controller_launched = TRUE;
    play_mode = play_mode_list[0]; //dirty!!
    output_text_code = "NOCNV";
    
    //[cmsg turnOffKerning:self];
    //[cmsg turnOffLigatures:self];

    [self message:"Welcom to TiMidity Synthesizer!\n"];
    [NSThread detachNewThreadSelector:@selector(core_thread:)
			     toTarget:self withObject:nil ];
}

- (void)refresh
{
    //[self message:"refresh\n"];
    [instrumentsList reloadData];
}

extern float output_volume;
- (IBAction)change_volume:(id)sender
{
    //fprintf(stderr, "%g\n", [sender floatValue]);
    output_volume = [sender floatValue];
}

- (void) windowWillClose   : (NSNotification *) aNote
{
    //fprintf(stderr,"close window.\n");
    exit(0);
}

@end
