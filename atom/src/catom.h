/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include "pythonhelpers.h"
#include "observerpool.h"


using namespace PythonHelpers;


#define MAX_MEMBER_COUNT    static_cast<uint32_t>( ( 1 << 16 ) - 1 )
#define MEMBER_COUNT_MASK   static_cast<uint32_t>( ( 1 << 16 ) - 1 )
#define NOTIFY_BIT          static_cast<uint32_t>( 1 << 16 )


extern "C" {


typedef struct {
    PyObject_HEAD
    uint32_t count;  // bitfield: lower 16 == member count; upper 16 == flags
    PyObject** data;
    ObserverPool* observers;
} CAtom;


inline bool
get_atom_notify_bit( CAtom* atom )
{
    return ( atom->count & NOTIFY_BIT ) > 0;
}


inline void
set_atom_notify_bit( CAtom* atom, bool set )
{
    if( set )
        atom->count |= NOTIFY_BIT;
    else
        atom->count &= ~NOTIFY_BIT;
}


inline uint32_t
get_atom_count( CAtom* atom )
{
    return atom->count & MEMBER_COUNT_MASK;
}


// 'name' should be the name string on the member for best performance
int
observe_fast( CAtom* atom,  PyObject* name, PyObject* callback );


// 'name' should be the name string on the member for best performance
int
unobserve_fast( CAtom* atom,  PyObject* name, PyObject* callback );


int
import_catom();


extern PyTypeObject CAtom_Type;


inline int
CAtom_Check( PyObject* object )
{
    return PyObject_TypeCheck( object, &CAtom_Type );
}


}  // extern "C"

