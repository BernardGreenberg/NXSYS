//
//  CanvasExtentDialogController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/29/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "CanvasExtentDialogController.h"
NSView * getMainView();  // have to be .mm, not .m, if these linkages are to resolve.
NSWindow * getMainWindow();

@interface CanvasExtentDialogController () // () seems to allow us to extend original decl'n.
{NSString * zmsg;}
@property (weak) IBOutlet NSTextField * X;
@property (weak) IBOutlet NSTextField * Y;
@property (weak) IBOutlet NSTextField * msg;
@end

NSString * firstmsg = @"The dimensions above are those of the current window.  If you want the panel to be bigger than that, you must say so and how much so, or you won't be able to scroll.  You can return here with \"View | Set Canvas Size\".";

@implementation CanvasExtentDialogController
- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"CanvasExtentDialog"]) )
    {
        NSLog(@"init failed for canvas extent dialog");
        return nil;
    }
    return self;
    
} // end init

-(IBAction)onOK:(id)sender
{
    NSView * view = getMainView();
    NSRect r = [view frame];
    r.size.width = fmax([_X integerValue], 200.0);
    r.size.height = fmax([_Y integerValue], 200.0);
    [view setFrame:r];
    [NSApp stopModal];
    [[self window] close];
}


-(IBAction)onCancel:(id)sender
{
    [NSApp stopModal];
    [[self window] close];
}
-(void)showModal:(BOOL)first;
{
    zmsg = first ? firstmsg : nil;
    [NSApp runModalForWindow:[self window]];
}
- (void)windowDidLoad {
    [super windowDidLoad];
     /* View can be a lot bigger than Window, that's the whole deal here. */
    CGSize viewSize = getMainView().frame.size;
    CGSize windowSize = getMainWindow().frame.size; // yes, includes non-client wtf
    if (zmsg != nil) {
        [_msg setStringValue:zmsg];
        NSView * view = getMainView();
        NSRect r = [view frame];
        viewSize = windowSize;
        r.size = windowSize;
        [view setFrame:r];
    }
    /* setIntegerValue puts in commas that it then refuses to read. */
    
    int x = (int)fmax(windowSize.width, viewSize.width);
    int y = (int)fmax(windowSize.height, viewSize.height);
    [_X setStringValue:[NSString stringWithFormat:@"%d", x]];
    [_Y setStringValue:[NSString stringWithFormat:@"%d", y]];

    

}

@end
