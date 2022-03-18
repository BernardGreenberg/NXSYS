//
//  salvager.hpp
//  TLEdit
//
//  Created by Bernard Greenberg on 3/18/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef salvager_hpp
#define salvager_hpp

void Salvager(const char * message);

#ifdef DEBUG
#define SALVAGER(message) Salvager(message)
#else
#define SALVAGER(message) (void)message
#endif

#endif /* salvager_hpp */
