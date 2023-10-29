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
 This required a serious rewrite, 28 Oct 2023, copying the relay pointers into an instance-local
 STL array because of an seeming change in Mac behavior;  the destruction of this list view object
 no longer happens at the time the dialog goes out of scope, but when command-level cleanup occurs.
 This caused the Mac code that attempts to clean up the strings to indirect through the
 supplied array, which was ingeniously done by reference to avoid precisely this kind of copying.
 Unfortunately, the calling code has deallocated that STL array, and this code would fault.
 
 What it thinks it is doing by calling objectValueForTableColumn at DEALLOCATE time is not so clear,
 as this callback allocated string copies as NSStrings. Hopefully the previous ones would no longer
 be retained. But adding this copying does fix it.  Fixed harder by removing any reference through
 the array beyond init/reload time, other than to return the final value. NSStrings are computed
 at init/reload time now, and saved in a separate STL array (theStrings).
 
 That's a pretty serious needs-documentation state of affairs, that NSListViews supplied with
 temporarily constructed pointed networks are going to come back to haunt them after they have
 been deallocated. Maybe Apple should know about this.  NXSYSMac 2.7.1 .
 */
#import "RelayListView.h"
#include "relays.h"
#include <algorithm> //sort

@interface RelayListView ()
{
    std::vector<Relay*>theRelays;
    std::vector<NSString*>theStrings;
    NSInteger nColumns;
}
@end

static bool relay_cmp(const Relay *r1, const Relay* r2) { //STL-compliant bool result
    Sexpr s1 = r1->RelaySym;
    Sexpr s2 = r2->RelaySym;
    long n1 = s1.u.r->n;
    long n2 = s2.u.r->n;
    if (n1 < n2) return true;
    if (n1 > n2) return false;
    const char * nom1 = redeemRlsymId(s1.u.r->type);
    const char * nom2 = redeemRlsymId(s2.u.r->type);
    return strcmp(nom1, nom2) < 1;
}

static NSString* NSifyRelayString(const Relay* r, bool nomenclatureOnly) {
    auto rsp = r->RelaySym.u.r;
    if (nomenclatureOnly)
        return [[NSString alloc] initWithUTF8String:redeemRlsymId(rsp->type)];
    else
        return [[NSString alloc] initWithUTF8String:rsp->PRep().c_str()];
}

@implementation RelayListView
@synthesize nomenclatureOnly;

-(void)awakeFromNib
{//https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CocoaViewsGuide/SubclassingNSView/SubclassingNSView.html
    [super awakeFromNib];
    [self setDataSource:self];
    nColumns = 1;  // get from nib, better.
    // This all really gets called for both relay lists.
}

-(NSInteger) numberOfRowsInTableView:(NSTableView*) tv
{
    return theRelays.size();
}

-(id)tableView:(NSTableView *)tv objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    // This gets called very mysteriously at destruction time, which may be way after
    // the call to the dialog has exited. It'd better not allocate or use data that
    // might not be there.
    if (row < theStrings.size())
        return theStrings[row];
    else
        return NULL;
}

-(void)setRelayContent:(const std::vector<Relay*>&)volatileRelayVector
{
    theRelays = volatileRelayVector;
    
    std::sort(theRelays.begin(), theRelays.end(), relay_cmp);

    theStrings.clear();  // does get reused.
    for (auto r : theRelays)
        theStrings.push_back(NSifyRelayString(r, nomenclatureOnly));

    [self reloadData];

    for (int i = 0; i < nColumns; i++) { // doesn't work for more than 9 . . .
        NSTableColumn * col = [self tableColumns][i];
        char cb[2] = {(char)('0' + i), 0};
        [col setIdentifier:[[NSString alloc] initWithUTF8String:cb]];
    }
}

-(Relay*)getSelectedRelay{
    NSInteger row = [self selectedRow];
    if (row < 0)
        return NULL;
    else
        return theRelays[row];
}


@end
