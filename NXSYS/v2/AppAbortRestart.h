//
//  AppAbortRestart.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/21/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#ifndef AppAbortRestart_h
#define AppAbortRestart_h

#include <exception>

class nxterm_exception : public std::exception
{
public:
    int val;
    nxterm_exception(int v): val(v){};
};

#endif /* AppAbortRestart_h */
