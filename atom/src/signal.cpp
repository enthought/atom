/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "member.h"
#include "catom.h"
#include "signal.h"


using namespace PythonHelpers;


extern "C" {


typedef struct {
    PyObject_HEAD
    CAtom* atom;
    Member* member;
} SignalBinder;


#define FREELIST_MAX 128
static int numfree = 0;
static SignalBinder* freelist[ FREELIST_MAX ];


typedef struct {
    Member member;
} Signal;


static void
SignalBinder_clear( SignalBinder* self )
{
    Py_CLEAR( self->atom );
    Py_CLEAR( self->member );
}


static int
SignalBinder_traverse( SignalBinder* self, visitproc visit, void* arg )
{
    Py_VISIT( self->atom );
    Py_VISIT( self->member );
    return 0;
}


static void
SignalBinder_dealloc( SignalBinder* self )
{
    PyObject_GC_UnTrack( self );
    SignalBinder_clear( self );
    if( numfree < FREELIST_MAX )
        freelist[ numfree++ ] = self;
    else
        self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
SignalBinder__call__( SignalBinder* self, PyObject* args, PyObject* kwargs )
{
    PyTuplePtr argsptr( newref( args ) );
    PyObjectPtr kwargsptr( xnewref( kwargs ) );
    if( self->member->validate_kind )
    {
        PyObject* owner = reinterpret_cast<PyObject*>( self->atom );
        PyTuplePtr vres( member_validate( self->member, owner, args, kwargs ? kwargs : _py_null ) );
        if( !vres )
            return 0;
        if( !vres.check_exact() )
            return py_type_fail( "Signal validator must return a 2-tuple of the form (tuple, <dict|null>)" );
        if( vres.size() != 2 )
            return py_type_fail( "Signal validator must return a 2-tuple of the form (tuple, <dict|null>)" );
        PyTuplePtr vres_args( vres.get_item( 0 ) );
        PyDictPtr vres_kwargs( vres.get_item( 1 ) );
        if( !vres_args.check_exact() )
            return py_type_fail( "Signal validator must return a 2-tuple of the form (tuple, <dict|null>)" );
        argsptr.set( vres_args.release() );
        if( vres_kwargs == _py_null )
            kwargsptr.set( 0 );
        else if( vres_kwargs.check_exact() )
            kwargsptr.set( vres_kwargs.release() );
        else
            return py_type_fail( "Signal validator must return a 2-tuple of the form (tuple, <dict|null>)" );
    }
    if( !get_atom_notify_bit( self->atom ) )
        Py_RETURN_NONE;
    PyObjectPtr nameptr( newref( self->member->name ) );
    if( !self->member->static_observers &&
        ( !self->atom->observers || !self->atom->observers->has_topic( nameptr ) ) )
        Py_RETURN_NONE;
    if( notify_observers( self->member, self->atom, argsptr, kwargsptr ) < 0 )
        return 0;
    Py_RETURN_NONE;
}


static PyObject*
SignalBinder_emit( SignalBinder* self, PyObject* args, PyObject* kwargs )
{
    return SignalBinder__call__( self, args, kwargs );
}


static PyObject*
SignalBinder_connect( SignalBinder* self, PyObject* arg )
{
    if( observe_fast( self->atom, self->member->name, arg ) < 0 )
        return 0;
    Py_RETURN_NONE;
}


static PyObject*
SignalBinder_disconnect( SignalBinder* self, PyObject* arg )
{
    if( unobserve_fast( self->atom, self->member->name, arg ) < 0 )
        return 0;
    Py_RETURN_NONE;
}


static PyMethodDef
SignalBinder_methods[] = {
    { "emit", ( PyCFunction )SignalBinder_emit, METH_VARARGS | METH_KEYWORDS,
      "Emit the signal with position and keywords arguments. "
      "This is fully equivalent to calling the signal." },
    { "connect", ( PyCFunction )SignalBinder_connect, METH_O,
      "Connect a callback to the signal. This is equivalent to observing the signal." },
    { "disconnect", ( PyCFunction )SignalBinder_disconnect, METH_O,
      "Disconnect a callback from the signal. This is equivalent to unobserving the signal." },
    { 0 } // sentinel
};


PyTypeObject SignalBinder_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "SignalBinder",                         /* tp_name */
    sizeof( SignalBinder ),                 /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)SignalBinder_dealloc,       /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)0,                            /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)SignalBinder__call__,      /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)SignalBinder_traverse,    /* tp_traverse */
    (inquiry)SignalBinder_clear,            /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)SignalBinder_methods, /* tp_methods */
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
_SignalBinder_New( CAtom* atom, Member* member )
{
    PyObject* pybinder;
    if( numfree > 0 )
    {
        PyObject* pybinder = reinterpret_cast<PyObject*>( freelist[ --numfree ] );
        _Py_NewReference( pybinder );
    }
    else
    {
        pybinder = PyType_GenericAlloc( &SignalBinder_Type, 0 );
        if( !pybinder )
            return 0;
    }
    Py_INCREF( reinterpret_cast<PyObject*>( atom ) );
    Py_INCREF( reinterpret_cast<PyObject*>( member ) );
    SignalBinder* binder = reinterpret_cast<SignalBinder*>( pybinder );
    binder->atom = atom;
    binder->member = member;
    return pybinder;
}


static PyObject*
Signal__get__( PyObject* self, PyObject* owner, PyObject* type )
{
    if( !owner )
        return newref( self );
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    return _SignalBinder_New( atom, member );
}


static int
Signal__set__( PyObject* self, PyObject* owner, PyObject* value )
{
    py_type_fail( "cannot modify a Signal" );
    return -1;
}


PyTypeObject Signal_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "catom.Signal",                         /* tp_name */
    sizeof( Signal ),                       /* tp_basicsize */
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
    (descrgetfunc)Signal__get__,            /* tp_descr_get */
    (descrsetfunc)Signal__set__,            /* tp_descr_set */
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
import_signal( void )
{
    if( PyType_Ready( &SignalBinder_Type ) < 0 )
        return -1;
    if( PyType_Ready( &Signal_Type ) < 0 )
        return -1;
    return 0;
}


} // extern "C"

