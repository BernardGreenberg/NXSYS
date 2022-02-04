//
//  main.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

// renamed from main.m and promoted 21 Aug 2019

#import <Cocoa/Cocoa.h>
#import <exception>
#include "AppAbortRestart.h"
#include <stdio.h>
#include <string>
#include "MessageBox.h"

int main(int argc, const char * argv[])
{
    while (true) {
        try {
            return NSApplicationMain(argc, argv);
        } catch (nxterm_exception e) {
            // reinit doesn't really work
            if (e.val == IDNO)
                return 1;
            if (e.val == IDCANCEL)
                return 1;
            return 1;
        }
    }
}
