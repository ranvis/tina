//
//  ListData_Instruments.m
//  TiMiditySynth
//
//  Created by –ìã‹MÆ on Sun Aug 08 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "macosx_synth_c.h"
#import "macosx_ListData_Instruments.h"


@implementation ListData_Instruments

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return MAX_CHANNELS;
}

- (id)tableView:(NSTableView *)aTableView
 objectValueForTableColumn:(NSTableColumn *)aTableColumn
 row:(int)rowIndex
{
    NSString *column_id; 
    id        retid=0;
    int       ch;


    if( rowIndex>=MAX_CHANNELS ){
	return @"";
    }

    column_id = [aTableColumn identifier];
    ch = rowIndex;
    
    if( [column_id isEqualToString:@"ch"] ){
	char   buf[10];
	sprintf(buf, "%d", ch+1);
	retid = [NSString stringWithCString:buf];
    }else if(  [column_id isEqualToString:@"instrument"] ){
	if(ChannelStatus[ch].comm){
	    retid = [NSString stringWithCString:ChannelStatus[ch].comm];
	}else{
	    retid =@""; 
	}
    }
    return retid;
}

@end
