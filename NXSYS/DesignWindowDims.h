//
//  DesignWindowDims.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 7/9/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef DesignWindowDims_h
#define DesignWindowDims_h

/* This determines the dimensions of all things, most particularly track widths, sizes
 (and thus space between) aux keys, etc.  Must concur between NXSYS and TLEdit;

  9 July 2022 - make them equal, no more "horsey" TLEdit graphics
 11 July 2022 - Mac version now zoomable.
 
 These are the dimensions of the DOS screen on which NXSYS was first designed, and
 that makes everything look right.
 */

class NXSYS_DESIGN_WINDOW_DIMS {
public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 640;
};



#endif /* DesignWindowDims_h */
