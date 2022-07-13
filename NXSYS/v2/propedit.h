//
//  propedit.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 5/15/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef propedit_h
#define propedit_h

#ifdef TLEDIT

/* If really compiling TLEdit, include the whole PropCell system */
#include "PropCell.h"

#else

/* When not compiling TLEdit, just create a CRTP no-op so that object class declarations
    don't have to be conditionalized */

template <class Derived>
class PropEditor {
    
};

#endif /* not TLEdit */

#endif /* propedit_h */
