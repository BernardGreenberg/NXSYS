//
//  HelpController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/27/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <WebKit/WebKit.h>


@interface HelpController : NSWindowController<WKNavigationDelegate>
@property (weak) IBOutlet NSScrollView *scrollView;
@property (unsafe_unretained) IBOutlet NSTextView *textView;
//@property (weak) IBOutlet WKWebView *theWebView;
//@property (weak) IBOutlet WKWebView *theWebView;
@property (strong) IBOutlet WKWebView *theWebView;
@property (weak) IBOutlet NSButton *backButton;
@property (weak) IBOutlet NSButton *forwardButton;
@property (weak) IBOutlet NSTextFieldCell *URLBar;
@property (weak) IBOutlet NSTextField *URLBarContainer;
-(void)log:(NSString*)nsstring;
-(void)HTMLHelp:(NSString*)resource_name tag:(NSString*)tag;
-(void)TextHelp:(NSString*)resource_name;
-(void)HelpSystemDisplay:(const char *)text;
-(void)HTMLView:(NSURL*)URL;
-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;
@end

void TextHelp(NSString* key);
