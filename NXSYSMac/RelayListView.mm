//
//  RelayListView.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/29/14.
//  Hefty rewrite 28 October 2023
//  Copyright (c) 2014, 2023 Bernard Greenberg. All rights reserved.
//

//This is nontrivial to use.  You have to create a "Table View" in the IB, and change the
//Custom Class of the innermost NSTableView to be this, RelayListView. Change the number
//of columns to 1.  Then route the referencing outlet to a parent controller's property.
//The subview list should retain it.

//An attempt to use multiple columns to condense the extent vertically failed because
//NSTable columns don't like to be used like that; they are user-repositionable horizontally.

/*
 This required a near-total rewrite starting 28 Oct 2023, triggered by a seeming change in Mac
 behavior:  the destruction of this TableView object no longer happens at the time the dialog goes
 out of scope, but when the UI returns to "command-level". For some reason, it seemingly
 began to call objectValueForTableColumn at that time.  Our implementation of the latter,
 unsurprisingly, navigated the data structure supplied to this code at setRelayContent time,
 which, unfortunately, no longer existed, having been very reasonably destroyed when the dialog
 was dismissed and closed. This caused a fault when the dialog was no longer active!

 What Mac thinks he is doing by calling objectValueForTableColumn at cleanup time is not so clear.
 The old code allocated string copies as NSStrings at that time, and  hopefully the previous ones
 would no longer be retained, but it did so by indirecting through a saved pointer to the
 temporarily allocated array.  The new code is vigorously STL-exploiting, and passes only
 first-class vectors, including a "volatileRelayVector" which is immediately copied to a
 not-so-volatile instance variable vector, theRelays.  The strings are now computed at that time and
 was cached as theStrings.  The code now references nothing outside the object past setRelayContent
 time, although traffic in pure relay pointers is its calling (as it were).
  
 That's a pretty serious needs-documentation state of affairs, that NSTableViews supplied with
 temporarily constructed data are going to come back to haunt it after they have been deallocated.
 Reported to Apple 10/28.  NXSYSMac 2.7.1 .
 */
#include "relays.h"

#import "RelayListView.h"

#include "ValuingMap.hpp"
#include "STLExtensions.h"

@interface RelayListView ()
{
    std::vector<Relay*>theRelays;
    std::vector<NSString*>theStrings;
}
@end

@implementation RelayListView
@synthesize nomenclatureOnly;  //bool set by callers wanting only alphabetic nomenclature (no $'s)

/*     Apple AppKit API  */

-(void)awakeFromNib
{//https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CocoaViewsGuide/SubclassingNSView/SubclassingNSView.html
    [super awakeFromNib];
    [self setDataSource:self];
}

-(NSInteger) numberOfRowsInTableView:(NSTableView*) tv
{
    return theRelays.size();
}

-(id)tableView:(NSTableView *)tv objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    // This gets called very mysteriously at destruction time, too. See comment above. */
    // might not be there.
    return theStrings[row];
}

/* NXSYS API */

/* Supplies the relays to be listed (can be an empty vector).  Although passed by
   const&, the vector is copied into local instance storage because
   (a) we have to sort the relays for display order and
   (b) supplied structure cannot be counted on to exist at the time Cocoa deigns
    to destroy the dialog (see top comment). */

-(void)setRelayContent:(const std::vector<Relay*>&)volatileRelayVector
{
    theRelays = volatileRelayVector;                   //copy 'em.
    pointer_sort(theRelays.begin(), theRelays.end());  //sort 'em. Relays now sort!

    /* Compute and cache as theStrings the relay-names to be displayed */
    theStrings = ValuingMap(theRelays, [self](auto r){return [self getRelayString:r];});

    [self reloadData];   //now trigger reload. objectValueForTableColumn called to fill the cells
    if (theRelays.size() > 0)     // ... and scroll to the top iff not empty
        [self scrollRowToVisible:0];
}

-(NSString*)getRelayString:(Relay*) relay  //no longer called by objectValueForTableColumn!
{
    auto R = *(relay->RelaySym.u.r);  // "RelaySym" is a Sexpr, whose u.r is the Rlysym of interest.
    return
        [[NSString alloc] initWithUTF8String:
         nomenclatureOnly ? redeemRlsymId(R.type) : R.PRep().c_str()];
}

-(Relay*)getSelectedRelay{                //returns the "answer"
    return theRelays[[self selectedRow]];
}

@end
