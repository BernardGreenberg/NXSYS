//
//  ContextMenu.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/28/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#include "WinMacCalls.h"
#include "resource.h"
#include <string>
#include "AppDelegate.h"
#include <initializer_list>
void ExternEditContextMenu(void*, void*);
void NXGOExtractWPCoords(void*,long&, long&);

#define SEPARATOR_VALUE -999
struct MenuItemDef {const char * s; int cmd;};
typedef const std::initializer_list<MenuItemDef> MenuDef;

static MenuDef SignalMenu = {
{ "Show &Full Signal",           IDC_FULL_SIGNAL_DISPLAY},
{ "&Draw relay",                 ID_DRAW_RELAY},
{ "SEPARATOR",                   SEPARATOR_VALUE},
{ "Toggle f&leeting",            ID_TOGGLE_FLEETING},
{ "Press (Initiate) &PB",        ID_INITIATE},
{ "Ca&ncel",                     ID_CANCEL},
{ "&Call on (initiate)",         ID_CALL_ON},
{ "CO Stop Release (&VPB)",      ID_VPB},
{ "SEPARATOR",                   SEPARATOR_VALUE},

{ "&Query Relay",                ID_RELAY_QUERY},
{ "SEPARATOR",                   SEPARATOR_VALUE},
{ "Reset &Approach locking",     ID_RESET_APPROACH}
};

static MenuDef SwitchMenu =  {

    { "&Draw relay...",              ID_DRAW_RELAY},

    {"SEPARATOR", SEPARATOR_VALUE},
    {"Call &Reverse",               ID_REVERSE},
    { "Call &Normal",                ID_NORMAL},
    {"SEPARATOR", SEPARATOR_VALUE},
    { "&Query relay...",             ID_RELAY_QUERY}
};

static MenuDef TrackMenu = {
    { "&Draw relay...",              ID_DRAW_RELAY},
    { "", SEPARATOR_VALUE},
    { "&Occupy",                     ID_OCCUPY},
    { "&Vacate",                     ID_VACATE},
    { "&Train...",                   ID_TRAIN},
    { "", SEPARATOR_VALUE},
    { "&Query relay...",             ID_RELAY_QUERY}
};

static MenuDef SwKeyMenu = {
    { "Call &Reverse",               ID_REVERSE},
    { "Call &Normal",                ID_NORMAL},
    { "Call Reverse, &HOLD",         ID_REVERSE_HOLD},
    { "Call Normal, HO&LD",          ID_NORMAL_HOLD},
    { "&Unlatch hold",               ID_UNHOLD},
    {"", SEPARATOR_VALUE},
    { "&Draw relay...",              ID_DRAW_RELAY},
    { "&Query relay...",             ID_RELAY_QUERY}
};

@interface ContextMenuClosure : NSObject
{
    NSMenu* menu;
    long answer;
}
@end

/* All the previous nonsense with the protocol and handler was ignorant.  I can set up an object
   and direct menu items to it with setTarget messages; it needn't be a view, and works fine. */

struct MCell {
    NSMenu* menu;  // pontificem decoepi
    MCell(NSMenu* m) {menu = m;}
    void Edit(void* nxgo) {ExternEditContextMenu(nxgo, this);}
};

void MacDeleteMenuItem (void* vmcp, int cmd){
    NSMenu * menu = ((struct MCell*)vmcp)->menu;
    NSMenuItem * item = [menu itemWithTag:cmd];
    if (item != nil)
        [menu removeItem:item];
}

void MacEnableMenuItem (void* vmcp, int cmd, bool enable) {
    // no-op unless [item setAutoenablesItems:NO] has been done.
    NSMenu * menu = ((struct MCell*)vmcp)->menu;
    NSMenuItem * item = [menu itemWithTag:cmd];
    if (item != nil)
        [item setEnabled:(enable ? YES : NO)];
}

@implementation ContextMenuClosure
-(void)cleanupGratuitousSeparators
{
    bool worked = true;
    while (worked) {
        worked = false;
        NSArray * items = menu.itemArray;
        for (size_t i = 0; i < items.count; i++) {
            NSMenuItem * item = items[i];
            if (item.tag == 0) {
                if (i == 0 || i == items.count - 1) {
                    [menu removeItemAtIndex: i];
                    worked = true;
                    break;
                }
                NSMenuItem * next = items[i+1];
                if (next != nil) {  //can't be last.
                    [menu removeItemAtIndex: i];
                    worked = true;
                    break;
                }
            }
        }
    }
}

-(void)fabricateMenu:(MenuDef&) defs
{
    menu = [[NSMenu alloc] initWithTitle:@"Object Operations"];
    [menu setAutoenablesItems:NO];
    int i = 0;
    for (auto& md : defs) {
        if (md.cmd == SEPARATOR_VALUE) {
            [menu insertItem:[NSMenuItem separatorItem] atIndex:i];
        }
        else {
            NSString * label = [[NSString alloc] initWithUTF8String:md.s];
            label = [label stringByReplacingOccurrencesOfString:@"&" withString:@""];
            NSMenuItem * m1 = [menu insertItemWithTitle:label
                                                 action:@selector(HandleItem:)
                                          keyEquivalent:@"" // difficult accelerators N.G.
                                                atIndex:i];
            [m1 setEnabled:YES];
            [m1 setTag:md.cmd];
            [m1 setTarget:self];
        }
        i += 1;
    }
}
-(void)selectAndCreateMenu:(int)resource_id
{
    switch (resource_id) {
        case IDR_SIGNAL_CONTEXT_MENU:
            [self fabricateMenu:SignalMenu];
            break;
        case IDR_SWITCH_CONTEXT_MENU:
            [self fabricateMenu:SwitchMenu];
            break;
        case IDR_TRACK_CONTEXT_MENU:
            [self fabricateMenu:TrackMenu];
            break;
        case IDR_SWITCH_KEY_CONTEXT_MENU:
            [self fabricateMenu:SwKeyMenu];
            break;
        default:
            assert(!"Unknown menu resource ID");
    }

}
-(void)HandleItem:(id)sender
{
    answer = [(NSMenuItem*)sender tag];
}
-(int)run:(int)resource_id hWnd:(void*)hWnd NXGObject:(void*)object
{
    [self selectAndCreateMenu:resource_id];
    MCell(menu).Edit(object);
    [self cleanupGratuitousSeparators];
    answer = 0;
    [menu popUpMenuPositioningItem:nil // no item, dft is to choose first.
                        atLocation:NXGOLocAsPoint(object)
                            inView:getHWNDView(hWnd)];
    
    return (int)answer;
}
@end

int MacWindowsContextMenu(void* hWnd, int resource_id, void* nxgObject) {
    return [[[ContextMenuClosure alloc] init] run:resource_id hWnd:hWnd NXGObject:nxgObject];
}
