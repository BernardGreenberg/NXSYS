//
//  HelpController.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/27/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "HelpController.h"
#import "AppDelegate.h"
#include <string>

@implementation HelpController

NSDictionary* helpFontDictionary;

- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"Help"]) )
    {
        NSLog(@"init failed for Help");
        return nil;
    }
    return self;
    
} // end init
- (IBAction)BackButton:(id)sender {
    [_theWebView goBack];
}
- (IBAction)ForwardButton:(id)sender {
    [_theWebView goForward];
}
- (void)updateBrowseStatus
{
    NSString * urlstr = [[_theWebView URL] absoluteString];
    [_URLBar setTitle:urlstr];
    [_backButton setEnabled:[_theWebView canGoBack]];
    [_forwardButton setEnabled:[_theWebView canGoForward]];
    [_backButton setHidden:NO];
    [_forwardButton setHidden:NO];
    [_textView setHidden:YES];
    [_URLBarContainer setHidden:NO];
}
- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
        NSFont * font = [NSFont fontWithName:@"Times New Roman" size:16];
        id hfdObjects[] = {font, NSColor.textColor};
        id hfdKeys[] = {NSFontAttributeName, NSForegroundColorAttributeName};
        NSUInteger hfdCount = sizeof(hfdObjects)/sizeof(id);
        helpFontDictionary = [NSDictionary dictionaryWithObjects:hfdObjects forKeys:hfdKeys count:hfdCount];

        // Control outlets NOT SET until the window is actually created "de nibo".
    }
    //theWebView.configuration = [[WKWebViewConfiguration alloc] init];
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    [getNXWindow() addChildWindow:self.window ordered:NSWindowAbove];
}

- (void) ensureExposed
{
    [self showWindow:nil];
    [self.window makeKeyAndOrderFront:nil];
    [getNXWindow() addChildWindow:self.window ordered:NSWindowAbove];
}

- (void)log:(NSString*) string
{
    [self ensureExposed];
    [_theWebView setHidden:YES];
    // http://stackoverflow.com/questions/15172971/append-to-nstextview-and-scroll
    
    NSAttributedString* attrString = [[NSAttributedString alloc] initWithString:string
                                                                    attributes:helpFontDictionary];
    [_textView setString:@""];
    [[_textView textStorage] appendAttributedString:attrString];
    [_textView scrollRangeToVisible:NSMakeRange(0, 0)];
    [_textView setHidden:NO];
    [_backButton setHidden:YES];
    [_forwardButton setHidden:YES];
    [_URLBarContainer setHidden:YES];
    // "I read it on the internet!  -- well it is good for computer code, but I've read lies, too.
    // NSString *urlText = @"http://google.com";
   // [[self.theWebView mainFrame] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlText]]];
   // NSString *str=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"paylines.txt"];
   //NSURL *rtfUrl = [[NSBundle mainBundle] URLForResource:@"paylines" withExtension:@".txt"];
}

-(void)HTMLView:(NSURL *)url
{
    [self ensureExposed];
    [_theWebView setHidden:NO];
    [self.theWebView loadRequest:[NSURLRequest requestWithURL:url]];
    [self updateBrowseStatus];


}
-(void)TextHelp:(NSString*)rname
{

    NSString *filePath = [[NSBundle mainBundle] pathForResource:rname ofType:@"txt"];
    NSString *helpString = [NSString stringWithContentsOfFile:filePath
                                                     encoding:NSUTF8StringEncoding error:nil];
    [self log:helpString];
  
}
-(void)HTMLHelp:(NSString*)rname  tag:(NSString*) tag
{
    NSURL * url;
    std::string S = [rname UTF8String];
    if (S.length() > 5 && S.substr(0,5) == "file:")
        url = [NSURL URLWithString: rname];
    else
        url = [[NSBundle mainBundle] URLForResource:rname withExtension:@".html"];
    if (tag) { // guten tag
        NSString * s = [NSString stringWithFormat:@"%@#%@", url.absoluteString, tag]; // watch that language!
        url = [NSURL URLWithString:s];
    }
    [self HTMLView:url];
}
-(void)HelpSystemDisplay:(const char *) text
{
    assert(text != NULL);
    
    std::string stext;;
    stext.insert(0, "\n", 1); // first line seems to vanish off the top . . .
    
    /* There is some CRLF horseplay in the processing of interlocking help strings that may not
     be appropriate in the Mac environment -- figure it out some other day */
    
    for (const char * sp = text; *sp != '\0'; sp++) { // quel Maus-Mikki
        if (*sp == '\r' && sp[1] == '\n' && sp[2] == '\r' && sp[3] == '\n') {
            sp += 3;
        }
        else if (*sp != '\r') {
            stext += *sp;
        }
    }
    
    [self log:[[NSString alloc] initWithUTF8String:stext.c_str()]];
}

-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    [self updateBrowseStatus];
}
@end

