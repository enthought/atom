/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include "member.h"


extern "C" {


validate_func
get_validate_func( MemberValidator type );


default_func
get_default_func( MemberDefault type );


}  // extern C

