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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    macosx_ListData.c
    MacOS X, Data source of List.
    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#import "macosx_ListData.h"
#import "macosx_c.h"

@implementation ListDataSource
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return number_of_files;
}

- (id)tableView:(NSTableView *)aTableView
 objectValueForTableColumn:(NSTableColumn *)aTableColumn
 row:(int)rowIndex
{
    NSString *column_id; 
    id        retid=0;
    const char *filen;
    
    [filelist_lock lock];

    //theRecord = [records objectAtIndex:rowIndex];

    if( rowIndex>=number_of_files ){
        retid=0;
        goto exit;
    }

    if( list_of_files[rowIndex]->file == NULL ){
	filen = "--";
    }else{
	filen = strrchr(list_of_files[rowIndex]->file, '#');
        if( filen==NULL ){
            filen = strrchr(list_of_files[rowIndex]->file, PATH_SEP); }

	if( filen==NULL ){
	    filen = "--";
	}else{
	    filen ++;
	}
    }
    	
    column_id = [aTableColumn identifier];
    if( [column_id isEqualToString:@"No"] ){
        retid=( rowIndex==current_no? @"=>":@"");
        goto exit;
    }else if(  [column_id isEqualToString:@"Title"]  ){
        retid =  [NSString stringWithCString:
            list_of_files[rowIndex]->title?
                   list_of_files[rowIndex]->title:filen];
        goto exit;
    }else if(  [column_id isEqualToString:@"File"]  ){
        retid = [NSString stringWithCString:filen];
        goto exit;
    }else if( [column_id isEqualToString:@"full_path"] ){
        retid = [NSString stringWithUTF8String:
            list_of_files[rowIndex]->file?
                    list_of_files[rowIndex]->file:"--" ];
        goto exit;
    }else if([column_id isEqualToString:@"wrd"]){
	switch(list_of_files[rowIndex]->haveWRD){
	  case MFNODE_HAVEWRD_WRD:
	    retid = @"W";
	    break;
	  case MFNODE_HAVEWRD_SWRD:
	    retid = @"S";
	    break;
	  case MFNODE_HAVEWRD_NEOWRD:
	    retid = @"N";
	    break;
	}
    }else if([column_id isEqualToString:@"Time"]){
	if( list_of_files[rowIndex]->total_time==0 ){
	    retid = @"";
	}else{
	    char  buf[10];
	    sprintf(buf,"%d:%02d", list_of_files[rowIndex]->total_time/60,
		    list_of_files[rowIndex]->total_time%60);
	    retid = [NSString stringWithCString:buf];
	}
    }

exit:
    [filelist_lock unlock];

    return  retid;
}

@end
