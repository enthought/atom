/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include <iostream>
#include "member.h"
#include "catom.h"
#include "event.h"



using namespace PythonHelpers;


extern "C" {


typedef struct {
    PyObject_HEAD
    CAtom* atom;
    Member* member;
} EventBinder;


#define FREELIST_MAX 128
static int numfree = 0;
static EventBinder* freelist[ FREELIST_MAX ];


typedef struct {
    Member member;
} Event;


static void
EventBinder_clear( EventBinder* self )
{
    Py_CLEAR( self->atom );
    Py_CLEAR( self->member );
}


static int
EventBinder_traverse( EventBinder* self, visitproc visit, void* arg )
{
    Py_VISIT( self->atom );
    Py_VISIT( self->member );
    return 0;
}


static void
EventBinder_dealloc( EventBinder* self )
{
    PyObject_GC_UnTrack( self );
    EventBinder_clear( self );
    if( numfree < FREELIST_MAX )
        freelist[ numfree++ ] = self;
    else
        self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
EventBinder__call__( EventBinder* self, PyObject* args, PyObject* kwargs )
{
    if( kwargs )
        return py_type_fail( "An event cannot be triggered with keyword arguments" );
    Py_ssize_t size = PyTuple_GET_SIZE( args );
    if( size > 1 )
        return py_type_fail( "An event can be triggered with at most 1 argument" );
    PyObject* owner = reinterpret_cast<PyObject*>( self->atom );
    PyObjectPtr newvalue( newref( size == 0 ? _py_null : PyTuple_GET_ITEM( args, 0 ) ) );
    if( self->member->validate_kind )
    {
        PyObject* owner = reinterpret_cast<PyObject*>( self->atom );
        newvalue = member_validate( self->member, owner, _py_null, newvalue.get() );
        if( !newvalue )
            return 0;
    }
    if( !get_atom_notify_bit( self->atom ) )
        Py_RETURN_NONE;
    PyObjectPtr nameptr( newref( self->member->name ) );
    if( !self->member->static_observers &&
        ( !self->atom->observers || !self->atom->observers->has_topic( nameptr ) ) )
        Py_RETURN_NONE;
    PyObjectPtr change( MemberChange_New( owner, self->member->name, _py_null, newvalue.get() ) );
    if( !change )
        return 0;
    PyTuplePtr argsptr( PyTuple_New( 1 ) );
    if( !argsptr )
        return 0;
    argsptr.initialize( 0, change );
    PyObjectPtr kwargsptr( 0 );
    if( notify_observers( self->member, self->atom, argsptr, kwargsptr ) < 0 )
        return 0;
    Py_RETURN_NONE;
}


PyTypeObject EventBinder_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "EventBinder",                          /* tp_name */
    sizeof( EventBinder ),                  /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)EventBinder_dealloc,        /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)0,                            /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)EventBinder__call__,       /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)EventBinder_traverse,     /* tp_traverse */
    (inquiry)EventBinder_clear,             /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)0,                 /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)0,                        /* tp_descr_get */
    (descrsetfunc)0,                        /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)0,                             /* tp_new */
    (freefunc)PyObject_GC_Del,              /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


static PyObject*
EventBinder_New( CAtom* atom, Member* member )
{
    PyObject* pybinder;
    if( numfree > 0 )
    {
        pybinder = reinterpret_cast<PyObject*>( freelist[ --numfree ] );
        _Py_NewReference( pybinder );
    }
    else
    {
        pybinder = PyType_GenericAlloc( &EventBinder_Type, 0 );
        if( !pybinder )
            return 0;
    }
    Py_INCREF( reinterpret_cast<PyObject*>( atom ) );
    Py_INCREF( reinterpret_cast<PyObject*>( member ) );
    EventBinder* binder = reinterpret_cast<EventBinder*>( pybinder );
    binder->atom = atom;
    binder->member = member;
    return pybinder;
}


static PyObject*
Event__get__( PyObject* self, PyObject* owner, PyObject* type )
{
    if( !owner )
        return newref( self );
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    return EventBinder_New( atom, member );
}


static int
Event__set__( PyObject* self, PyObject* owner, PyObject* value )
{
    if( !CAtom_Check( owner ) )
    {
        py_expected_type_fail( owner, "CAtom" );
        return -1;
    }
    Member* member = reinterpret_cast<Member*>( self );
    PyObjectPtr newvalue( newref( value ? value : _py_null ) );
    if( member->validate_kind )
    {
        newvalue = member_validate( member, owner, _py_null, newvalue.get() );
        if( !newvalue )
            return -1;
    }
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    if( !get_atom_notify_bit( atom ) )
        return 0;
    PyObjectPtr nameptr( newref( member->name ) );
    if( !member->static_observers && ( !atom->observers || !atom->observers->has_topic( nameptr ) ) )
        return 0;
    PyObjectPtr change( MemberChange_New( owner, member->name, _py_null, newvalue.get() ) );
    if( !change )
        return -1;
    PyTuplePtr argsptr( PyTuple_New( 1 ) );
    if( !argsptr )
        return -1;
    argsptr.initialize( 0, change );
    PyObjectPtr kwargsptr( 0 );
    return notify_observers( member, atom, argsptr, kwargsptr );
}


PyTypeObject Event_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "catom.Event",                          /* tp_name */
    sizeof( Event ),                        /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)0,                          /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)0,                            /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)0,                         /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)0,                        /* tp_traverse */
    (inquiry)0,                             /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)0,                 /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    0,                                      /* tp_getset */
    &Member_Type,                           /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)Event__get__,             /* tp_descr_get */
    (descrsetfunc)Event__set__,             /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)0,                           /* tp_alloc */
    (newfunc)0,                             /* tp_new */
    (freefunc)0,                            /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


int
import_event( void )
{
    if( PyType_Ready( &EventBinder_Type ) < 0 )
        return -1;
    if( PyType_Ready( &Event_Type ) < 0 )
        return -1;
    return 0;
}


} // extern "C"

