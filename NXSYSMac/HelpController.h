//
//  HelpController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/27/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <WebKit/WebKit.h>

@interface HelpController : NSWindowController
@property (weak) IBOutlet NSScrollView *scrollView;
@property (unsafe_unretained) IBOutlet NSTextView *textView;
//@property (weak) IBOutlet WKWebView *theWebView;
//@property (weak) IBOutlet WKWebView *theWebView;
@property (strong) IBOutlet WKWebView *theWebView;
-(void)log:(NSString*)nsstring;
-(void)HTMLHelp:(NSString*)resource_name tag:(NSString*)tag;
-(void)TextHelp:(NSString*)resource_name;
-(void)HelpSystemDisplay:(const char *)text;
-(void)HTMLView:(NSURL*)URL;
@end

void TextHelp(NSString* key);
