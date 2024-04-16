#ifndef INCLUDE_CAKE_OWNER_TS_H
#define INCLUDE_CAKE_OWNER_TS_H

#ifdef INCLUDE_CAKE_OWNER_H
    #error "YOU CANNOT INCLUDE owner_ts.h IF YOU HAVE INCLUDED owner.h"
#endif

#define CAKE_OWNER_THREAD_SAFE
#include "owner.h"

#endif  // INCLUDE_CAKE_OWNER_TS_H
