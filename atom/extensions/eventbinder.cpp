/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include <vector>
#include "pythonhelpers.h"
#include "callbackhandler.h"


using namespace PythonHelpers;


extern "C" {


typedef struct {
    PyObject_HEAD
    CallbackHandler* handler;
} EventBinder;


static void
EventBinder_clear( EventBinder* self )
{
    if( self->handler )
        self->handler->py_clear();
}


static int
EventBinder_traverse( EventBinder* self, visitproc visit, void* arg )
{
    if( self->handler )
        return self->handler->py_traverse( visit, arg );
    return 0;
}


static void
EventBinder_dealloc( EventBinder* self )
{
    PyObject_GC_UnTrack( self );
    EventBinder_clear( self );
    delete self->handler;
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
EventBinder_bind( EventBinder* self, PyObject* callback )
{
    PyObjectPtr callbackptr( newref( callback ) );
    if( !self->handler )
        self->handler = new CallbackHandler();
    self->handler->add_callback( callbackptr );
    Py_RETURN_NONE;
}


static PyObject*
EventBinder_unbind( EventBinder* self, PyObject* callback )
{
    if( self->handler )
    {
        PyObjectPtr callbackptr( newref( callback ) );
        self->handler->remove_callback( callbackptr );
    }
    Py_RETURN_NONE;
}


static PyObject*
EventBinder__call__( EventBinder* self, PyObject* args, PyObject* kwargs )
{
    if( kwargs )
        return py_type_fail( "EventBinder.__call__ does not accept keywords" );
    if( PyTuple_GET_SIZE( args ) > 1 )
        return py_type_fail( "EventBinder.__call__ accepts at most 1 argument" );
    if( self->handler )
    {
        PyTuplePtr argsptr( newref( args ) );
        PyDictPtr kwargsptr( 0 );
        if( self->handler->invoke_callbacks( argsptr, kwargsptr ) < 0 )
            return 0;
    }
    Py_RETURN_NONE;
}


static PyMethodDef
EventBinder_methods[] = {
    { "bind",
      ( PyCFunction )EventBinder_bind, METH_O,
      "Bind a callback to be executed when the event is triggered." },
    { "unbind",
      ( PyCFunction )EventBinder_unbind, METH_O,
      "Unbind a previously bound callback." },
    { 0 } // sentinel
};


PyTypeObject EventBinder_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "eventbinder.EventBinder",              /* tp_name */
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
    (struct PyMethodDef*)EventBinder_methods, /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)0,                        /* tp_descr_get */
    (descrsetfunc)0,                        /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)PyType_GenericNew,             /* tp_new */
    (freefunc)PyObject_GC_Del,              /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


static PyMethodDef
eventbinder_methods[] = {
    { 0 } // Sentinel
};


PyMODINIT_FUNC
initeventbinder( void )
{
    PyObject* mod = Py_InitModule( "eventbinder", eventbinder_methods );
    if( !mod )
        return;
    if( PyType_Ready( &EventBinder_Type ) )
        return;
    PyObject* pytype = reinterpret_cast<PyObject*>( &EventBinder_Type );
    Py_INCREF( pytype );
    PyModule_AddObject( mod, "EventBinder", pytype );
}


} // extern "C"

