#import <Cocoa/Cocoa.h>

#include "MessageBox.h"

/* It turns out (2 Oct 2014 that simply doing this yourself with a straightforward window class
  is considerably easier than trying to fit this crazy thing's model.  See NXMessageBox.mm */

void dlgOK(NSString* prompt, NSString* informativeText)
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:prompt];
    [alert setInformativeText:informativeText];
    [alert addButtonWithTitle:@"OK"];
    [alert setAlertStyle:NSAlertStyleWarning];

    [alert runModal];
}

static NSString* dlgInput(NSString* prompt, NSString* defaultValue, NSString* informativeText)
{
    //https://gist.github.com/marteinn/540cfa4282add987ee6b
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:prompt];
    [alert setInformativeText:informativeText];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setAlertStyle:NSAlertStyleWarning];

   //                      informativeTextWithFormat:@"%@", informativeText];
    
    NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    [input setStringValue:defaultValue];
    [alert setAccessoryView:input];
    [[alert window] setInitialFirstResponder: input];  // 2/28/19
    // https://stackoverflow.com/questions/3539306/focus-nsalert-accessory-item
    NSInteger button = [alert runModal];
    if (button == NSAlertFirstButtonReturn) {
        [input validateEditing];
        return [input stringValue];
    } else if (button == NSAlertSecondButtonReturn) {
        return nil;
    } else {
       assert(!"bad menu button, s/b impossible");
        return nil;
    }
}



bool SimpleGetDialog(const char * label, const char * msg, char * buf, int count) {
    NSString * nsmsg = [[NSString alloc] initWithUTF8String:msg];
    NSString * nslabel = [[NSString alloc] initWithUTF8String:label];
    NSString * result = dlgInput(nslabel, @"", nsmsg);
    if (result == nil) {
        return false;
    }
    memset(buf, 0, count);
    strncpy(buf, [result UTF8String], count-1);
    return true;
}

#if 0
// Needs intelligent rewrite like function about with new NSAlert
// Nobody uses these right now.

NSString* dlgOKCancel(NSString* prompt, NSString* informativeText) {
    NSAlert *alert = [NSAlert alertWithMessageText: prompt
                                     defaultButton:@"OK"
                                   alternateButton:@"Cancel"
                                       otherButton:nil
                         informativeTextWithFormat:@"%@", informativeText];
    
    NSInteger button = [alert runModal];
    if (button == NSAlertDefaultReturn) {
        return @"true";
    } else if (button == NSAlertAlternateReturn) {
        return nil;
    } else {
        assert(!"Invalid input dialog button.");
        return nil;
    }
}

NSInteger dlgYesNoCancel(NSString* prompt, NSString* informativeText) {
    NSAlert *alert = [NSAlert alertWithMessageText: prompt
                                     defaultButton:@"No"
                                   alternateButton:@"Yes"
                                       otherButton:@"Cancel"
                         informativeTextWithFormat:@"%@", informativeText];
    
    NSInteger button = [alert runModal];
    if (button == NSAlertDefaultReturn) {
        return IDNO;
    } else if (button == NSAlertAlternateReturn) {
        return IDYES;
    } else {
        return IDCANCEL;
    }
}




int MessageBox(void*, const char * msg, const char * label, int flags) {
    NSString * nsmsg = [[NSString alloc] initWithUTF8String:msg];
    if (nsmsg == nil)
        nsmsg = @"CAN'T CONVERT MESSAGE TO UTF8!";
    NSString * nslabel = [[NSString alloc] initWithUTF8String:label];
    int ftype = flags & MB_LOWMASK;
    if ((ftype == MB_YESNO) || (ftype == MB_YESNOCANCEL)) {
        int ID = (int)dlgYesNoCancel(nslabel, nsmsg);
        return ID;
    } else {
        dlgOK(nslabel, nsmsg);
    }
    return 0; // use result actually
   
}
#endif
