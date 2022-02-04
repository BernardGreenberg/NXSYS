//
//  RelayListView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/29/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
class Relay;

@interface RelayListView : NSTableView<NSTableViewDataSource>
@property BOOL nomenclatureOnly;
-(void)setRelayContent:(Relay**)theRelays count:(NSInteger)nRelays;
-(Relay*)getSelectedRelay;
@end
