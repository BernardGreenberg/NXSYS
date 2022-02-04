//
//  RelayListView.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/29/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

//This is nontrivial to use.  You have to create a "Table View" in the IB, and change the
//Custom Class of the innermost NSTableView to be this, RelayListView. Change the number
//of columns to 1.  Then route the referencing outlet to a parent controller's property.
//The subview list should retain it.

//An attempt to use multiple columns to condense the extent vertically failed because
//NSTable columns don't like to be used like that; they are user-repositionable horizontally.

#import "RelayListView.h"
#include "relays.h"

@interface RelayListView ()
{
    Relay** theRelays;
    NSInteger nRelays;
    NSInteger nColumns;
}
@end

static int relay_sorter(const void*vr1, const void*vr2) {
    Relay* r1 = *((Relay**)vr1);
    Relay* r2 = *((Relay**)vr2);
    Sexpr s1 = r1->RelaySym;
    Sexpr s2 = r2->RelaySym;
    long n1 = s1.u.r->n;
    long n2 = s2.u.r->n;
    if (n1 < n2) return -1;
    if (n1 > n2) return +1;
    const char * nom1 = redeemRlsymId(s1.u.r->type);
    const char * nom2 = redeemRlsymId(s2.u.r->type);
    return strcmp(nom1, nom2);
}

@implementation RelayListView
@synthesize nomenclatureOnly;
- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    //don't/shouldn't have any custom drawing code.
}
-(void)awakeFromNib
{//https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CocoaViewsGuide/SubclassingNSView/SubclassingNSView.html
    [super awakeFromNib];
    [self setDataSource:self];
    nRelays = 0;
    theRelays = NULL;
    nColumns = 1;  // get from nib, better.
    // This all really gets called for both relay lists.
}

-(NSInteger) numberOfRowsInTableView:(NSTableView*) tv
{
    return nRelays;
}

-(id)tableView:(NSTableView *)tv objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    if (row < nRelays) {
        Rlysym * rsp = theRelays[row]->RelaySym.u.r;
        if (nomenclatureOnly)
            return [[NSString alloc] initWithUTF8String:redeemRlsymId(rsp->type)];
        else
            return [[NSString alloc] initWithUTF8String:rsp->PRep().c_str()];
    } else {
        return NULL;
    }
}

-(void)setRelayContent:(Relay**)atheRelays count:(NSInteger)anRelays
{
    theRelays = atheRelays;
    nRelays = anRelays;
    
    qsort(theRelays, nRelays, sizeof(Relay*), relay_sorter);
    [self reloadData];

    for (int i = 0; i < nColumns; i++) { // doesn't work for more than 9 . . .
        NSTableColumn * col = [self tableColumns][i];
        char cb[2] = {(char)('0' + i), 0};
        [col setIdentifier:[[NSString alloc] initWithUTF8String:cb]];
    }
}

-(Relay*)getSelectedRelay{
    NSInteger row = [self selectedRow];
    int col = 0;
    // NSInteger col = [self selectedColumn];
    assert(col >= 0);
    if (row < 0)
        return NULL;
    else
        return theRelays[row];
}


@end
