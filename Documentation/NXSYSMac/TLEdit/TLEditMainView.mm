#import <Cocoa/Cocoa.h>
#import "MainView.h"

#include "WinMacCalls.h"
#include "WinViewUtils.h"
#include "RubberBandView.h"
#include "WinMouseDefs.h"

#define WM_USER 2000
#define WM_NXGO_LBUTTONSHIFT (WM_USER+1)
#define WM_NXGO_LBUTTONCONTROL (WM_USER+2)
#define WM_NXGO_RBUTTONCONTROL (WM_USER+3)

void GraphicsMouse(HWND, unsigned, int, long);
extern HWND G_mainwindow;

NSView* getMainView();

@interface TLEditMainView : MainView
@property (strong) NSArray * rubberBands;
@property NSTrackingArea * trackingArea;
@end

@implementation TLEditMainView
-(BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    /* Esto nobis praegustatum . . . mew */
    return YES;
}

-(RubberBandView*)createRubberBand
{
    RubberBandView* band = [[RubberBandView alloc] init];
    [self addSubview:band];
    return band;
}
-(void)addMyTrackingArea
{
    _trackingArea = [[NSTrackingArea alloc]
                     initWithRect:[self frame]
                     options: (NSTrackingMouseEnteredAndExited |
                               NSTrackingActiveInActiveApp |
                               NSTrackingMouseMoved
                               //                                   NSTrackingActiveInKeyWindow
                               )
                     owner:self userInfo:nil];
    
    [self addTrackingArea:_trackingArea];
}
-(void)removeMyTrackingArea
{
    if (_trackingArea != nil) {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }
}
-(NSView*)initWithFrame:(NSRect)f
{
    self = [super initWithFrame:f];
    _rubberBands = [NSArray arrayWithObjects:
                    [self createRubberBand], [self createRubberBand], [self createRubberBand], nil];
    return self;
}
-(long)lParamFromEvent:(NSEvent*)theEvent
{
    NSPoint location = [theEvent locationInWindow];
    NSPoint mpt = [self convertPoint:location fromView:nil];
    return (((int)mpt.y) << 16) | ((int)mpt.x);
}
- (void)rodaCommon:(NSEvent*) theEvent Message:(int)message
{
    int wParam = 0;
    if (message == WM_MOUSEMOVE) { // really "left drag"
        //But for this, GraphicsWindow_Rodentate won't call TrackLayoutRodentate
        wParam |= MK_LBUTTON;
    }
    if (message == WM_LBUTTONDOWN) {
        if ([theEvent modifierFlags] & NSEventModifierFlagShift) {
            message = WM_NXGO_LBUTTONSHIFT;
        }
        if ([theEvent modifierFlags] & NSEventModifierFlagControl) {
            message = WM_NXGO_LBUTTONCONTROL;
        }
    } else if (message == WM_RBUTTONDOWN) {
        if ([theEvent modifierFlags] & NSEventModifierFlagControl) {
            message = WM_NXGO_RBUTTONCONTROL;
            
        }
    }
    
    GraphicsMouse(G_mainwindow, message, wParam, [self lParamFromEvent:theEvent]);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [[self window] setAcceptsMouseMovedEvents:YES];
    
    [self rodaCommon:theEvent Message:WM_LBUTTONDOWN];
}

- (void)mouseUp:(NSEvent*)theEvent
{
    GraphicsMouse(G_mainwindow, WM_LBUTTONUP, 0, [self lParamFromEvent:theEvent]);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    [self rodaCommon:theEvent Message:WM_RBUTTONDOWN];
}
- (void)mouseDragged:(NSEvent *)theEvent
{
    [self rodaCommon:theEvent Message:WM_MOUSEMOVE];
}
- (void)mouseMoved:(NSEvent*)theEvent
{
    //doesn't get invoked unless TrackingArea is up.
    [self rodaCommon:theEvent Message:WM_MOUSEMOVE];
}

-(void)StartRubberBand:(int)i x:(long)x y:(long)y {
    [_rubberBands[i] setBeginning:NSMakePoint(x,y)];
}
-(void)EndRubberBand:(int)i x:(long)x y:(long)y  highlight:(BOOL)highlighted
{
    [_rubberBands[i] setEnd:NSMakePoint(x,y) highlighted:highlighted];
}
-(void)CloseRubberBand:(int)i
{
    [_rubberBands[i] close];
}
+(TLEditMainView*)getTLMain
{
    return (TLEditMainView*)(getMainView());
}

@end


void StartRubberBand(int i, long x, long y){
    [[TLEditMainView getTLMain] StartRubberBand:i x:x y:y];
}

void RubberBandTo(int i, long x, long y){
    [[TLEditMainView getTLMain] EndRubberBand:i x:x y:y highlight:NO];
}

void RubberBandToHighlighted(int i, long x, long y){
    [[TLEditMainView getTLMain] EndRubberBand:i x:x y:y highlight:YES];
}

void RubberBandOff(int i) {
    [[TLEditMainView getTLMain] CloseRubberBand:i];
}
    
void MacDragonOn() {
    [[TLEditMainView getTLMain] addMyTrackingArea];
}

void MacDragonOff() {
    [[TLEditMainView getTLMain] removeMyTrackingArea];
}
