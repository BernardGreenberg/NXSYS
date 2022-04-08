//
//  Created by Bernard Greenberg on 9/20/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
/* This needs its own class because of extensive interaction with native Macintosh objects,
 to wit, the font and color selection dialogs.  It does, however, run under the voluntary
 guidance of the Windows DlgProc code, with whom it also enjoys such interaction. */

#import "GenericWindlgController.h"
#include "resource.h"
extern const double WinMacFontRatio;
NSColor * colorFromCOLORREF(int);
int COLORREFFromNSColor(NSColor*);
extern int TrackDftCol;  // this is the default color
static const int DefaultWinHeight = 20;
static bool DefaultBoldness = true;

@interface TextStringDlg : GenericWindlgController <NSWindowDelegate, NSTextFieldDelegate>
{
    __strong NSFontManager * fontManager;
    __strong NSFont *currentFont;
    
    __strong NSColorPanel * colorPanel;
    __strong NSColor * currentColor;
    __strong NSColor * defaultColor;
    int winCompatHeight;

}
@property (weak) IBOutlet NSTextField * sampleTextField;
@property (weak) IBOutlet NSTextField * faceName;
@property (weak) IBOutlet NSTextField * heightField;
@property (weak) IBOutlet NSStepper * heightStepper;
@property (weak) IBOutlet NSTextField* theString;
@end

#define R(x) {#x, x},

static RIDVector rids {
    R(IDC_TEXT_TEXT)
    R(IDC_ETEXT_SAMPLE)
    R(IDC_ETEXT_HEIGHT_EDIT)
    R(IDC_ETEXT_HEIGHT_SPIN) // NSSteppers legal
    
    R(IDC_ETEXT_FACE_SELECTION)
    
    R(IDC_ETEXT_WPX)
    R(IDC_ETEXT_WPY)

    R(IDC_ETEXT_HEIGHT_DEFAULT)
    R(IDC_ETEXT_HEIGHT_CUSTOM)
    
    R(IDC_ETEXT_WEIGHT_DEFAULT)
    R(IDC_ETEXT_WEIGHT_NORMAL)
    R(IDC_ETEXT_WEIGHT_BOLD)

    R(IDC_ETEXT_COLOR_DEFAULT)
    R(IDC_ETEXT_COLOR_CUSTOM)
 
    R(IDC_ETEXT_FACE_DEFAULT)
    R(IDC_ETEXT_FACE_CUSTOM)
};

DEFINE_WINDLG_WITH_CLASS(dummy, IDD_EDIT_TEXT, TextStringDlg, @"TextStringProperties", rids);


@implementation TextStringDlg

-(void)didInitDialog        /* All this gets run after WM_INITDIALOG */
{
    defaultColor = colorFromCOLORREF(TrackDftCol);

    [[self window] setDelegate:self];
    
    /* Sic oramus, Papa! "Pater noster, qui es in caelis .... " Clam scit indeed.*/
    /* the font dialog won't route messages here without this. */
    [[self window] makeFirstResponder:self];
 
    [_heightStepper setTarget:self];
    [_heightStepper setAction:@selector(spinChange:)];
    [_heightField setDelegate:self];
    
    [_theString setDelegate:self];

    fontManager = [NSFontManager sharedFontManager];
    colorPanel = [NSColorPanel sharedColorPanel];

    winCompatHeight = DefaultWinHeight;
    currentFont = [fontManager fontWithFamily:@"Helvetica"
                                       traits:[self computeTrait]
                                       weight:0
                                         size:winCompatHeight * WinMacFontRatio];
    [fontManager setSelectedFont:currentFont isMultiple:NO];
    [_sampleTextField setFont:currentFont];
    
    if ([super getDlgItemCheckState:IDC_ETEXT_FACE_CUSTOM]) {
            [self faceCustom:self];
    }
    
    if ([super getDlgItemCheckState:IDC_ETEXT_HEIGHT_CUSTOM]) {
        [self updateHeightControls:[_heightField integerValue]];
    }

    if ([super getDlgItemCheckState:IDC_ETEXT_COLOR_CUSTOM]) {
        int customColor;
        [self reflectCommandParam:IDC_MAC_GET_COLOR lParam:(long)&customColor];
        currentColor = colorFromCOLORREF(customColor);
        [_sampleTextField setTextColor:currentColor];
        [self colorCustom:self];
       
        [colorPanel setColor:currentColor];
        [colorPanel orderFront:self];
        [[self window] makeFirstResponder:self];
        
        /* "makeFirstResponder" seems to have no effect when called from this method (see
            calls above).  Set this timer to do it a tenth of a second later, and it's fine. */

#if 1
        NSTimer * myTimer =[NSTimer timerWithTimeInterval:0.1 target:self selector:@selector(colorCustom:) userInfo: nil repeats:NO];
        [[NSRunLoop currentRunLoop] addTimer:myTimer forMode:NSModalPanelRunLoopMode];

#endif
    }
}

-(void)windowWillClose:(NSNotification*)notification{
    [[fontManager fontPanel:NO] orderOut:self];
    [colorPanel orderOut:self];
}

-(void)changeFont:(id)sender
{
    NSFont * f =[fontManager convertFont:[fontManager selectedFont]];
    if (f == nil)
        return;

    currentFont = f;

    [_sampleTextField setFont:currentFont];
    [_faceName setStringValue:currentFont.familyName];
    [self updateHeightControls:(int)(currentFont.pointSize/WinMacFontRatio)];
}
-(void)changeColor:(id)sender
{
    currentColor = [(NSColorPanel *)sender color];
    [_sampleTextField setTextColor:currentColor];
    long rgb = COLORREFFromNSColor(currentColor);
    [self reflectCommandParam:IDC_MAC_SEND_COLOR lParam:(int)rgb];
}

-(IBAction)colorCustom:(id)sender
{
    [[self window] makeFirstResponder:self];
    [colorPanel orderFront:self];
}
-(IBAction)colorDefault:(id)sender
{
    [self activeButton:sender];
    [colorPanel orderOut:self];
    currentColor = defaultColor;
    [_sampleTextField setTextColor:currentColor];

}

-(IBAction)heightDefault:(id)sender
{
    [super activeButton:sender]; // do the DlgProc code.  Hides the textfld and the spinner.
    winCompatHeight = DefaultWinHeight;
    currentFont = [fontManager fontWithFamily:currentFont.familyName
                                       traits:[self computeTrait]
                                       weight:0
                                         size:winCompatHeight * WinMacFontRatio];
    [_sampleTextField setFont:currentFont];
}

-(IBAction)heightCustom:(id)sender  // routed here by IB
{
    [[self window] makeFirstResponder:self];
    [super activeButton:sender];
    [self updateHeightControls:[_heightField integerValue]];
    
}
- (IBAction)faceCustom:(id)sender
{
    [[self window] makeFirstResponder:self];
    [fontManager orderFrontFontPanel:self];
    [super setDlgItemCheckState:IDC_ETEXT_HEIGHT_DEFAULT value:NO];
    [super setDlgItemCheckState:IDC_ETEXT_HEIGHT_CUSTOM value:YES];
    [_sampleTextField setFont:currentFont];
    [_faceName setStringValue:currentFont.familyName];
    [_faceName setHidden:NO];
    [self updateHeightControls:(int)currentFont.pointSize];
}
-(int)computeTrait
{
    if ([self getDlgItemCheckState:IDC_ETEXT_WEIGHT_BOLD])
        return NSFontBoldTrait;
    else if ([self getDlgItemCheckState:IDC_ETEXT_WEIGHT_NORMAL])
        return 0;
    else if (DefaultBoldness)
        return NSFontBoldTrait;
    else return 0;
}
/* All three buttons outlet to this method */
-(IBAction)actionBoldness:(id)sender
{
    [super activeButton:sender];
    currentFont = [fontManager fontWithFamily:currentFont.familyName
                                       traits:[self computeTrait]
                                       weight:0
                                         size:currentFont.pointSize];
    [_sampleTextField setFont:currentFont];
}

- (IBAction) controlTextDidChange: (NSNotification*) notif;
{
    NSTextField * textField = (NSTextField*)notif.object;
    if (textField == _theString) {
        _sampleTextField.stringValue = _theString.stringValue;
    } else {
        [self updateHeightControls:[textField integerValue]];
    }
}

-(IBAction) spinChange:(id)sender
{
    [self updateHeightControls:[sender integerValue]];
}

- (void) updateHeightControls:(NSInteger)val
{
    winCompatHeight = (int)val;
    [_heightStepper setIntegerValue:val];
    [_heightField setIntegerValue: val];
    if (val > 9 && val < 100) {
        currentFont = [NSFont fontWithName:[currentFont fontName] size:val*WinMacFontRatio];
        [_sampleTextField setFont:currentFont];
    }
    [_heightStepper setHidden:NO];
    [_heightField setHidden:NO];
}

@end

