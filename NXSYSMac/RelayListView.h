//
//  RelayListView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/29/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <vector>
class Relay;

@interface RelayListView : NSTableView<NSTableViewDataSource>
@property BOOL nomenclatureOnly;
-(void)setRelayContent:(std::vector<Relay*>)volatileRelayVector;
-(Relay*)getSelectedRelay;
@end
