//
//  SynthController.h
//  TiMiditySynth
//
//  Created by ñÏè„ãMç∆ on Sun Jul 25 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import <AppKit/AppKit.h>


@interface SynthController : NSWindowController {
    IBOutlet id cmsg;
    IBOutlet id instrumentsList;
    NSSlider   *volume; 
}

- (void)message:(const char*)buf;
- (void)refresh;
- (IBAction)change_volume:(id)sender;
- (void) windowWillClose   : (NSNotification *) aNote;

@end

extern SynthController * macosx_controller;
