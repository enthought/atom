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
} SignalBinder;


static void
SignalBinder_clear( SignalBinder* self )
{
    if( self->handler )
        self->handler->py_clear();
}


static int
SignalBinder_traverse( SignalBinder* self, visitproc visit, void* arg )
{
    if( self->handler )
        return self->handler->py_traverse( visit, arg );
    return 0;
}


static void
SignalBinder_dealloc( SignalBinder* self )
{
    PyObject_GC_UnTrack( self );
    SignalBinder_clear( self );
    delete self->handler;
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
SignalBinder_connect( SignalBinder* self, PyObject* callback )
{
    PyObjectPtr callbackptr( newref( callback ) );
    if( !self->handler )
        self->handler = new CallbackHandler();
    self->handler->add_callback( callbackptr );
    Py_RETURN_NONE;
}


static PyObject*
SignalBinder_disconnect( SignalBinder* self, PyObject* callback )
{
    if( self->handler )
    {
        PyObjectPtr callbackptr( newref( callback ) );
        self->handler->remove_callback( callbackptr );
    }
    Py_RETURN_NONE;
}


static PyObject*
SignalBinder__call__( SignalBinder* self, PyObject* args, PyObject* kwargs )
{
    if( self->handler )
    {
        PyTuplePtr argsptr( newref( args ) );
        PyDictPtr kwargsptr( xnewref( kwargs ) );
        if( self->handler->invoke_callbacks( argsptr, kwargsptr ) < 0 )
            return 0;
    }
    Py_RETURN_NONE;
}


static PyObject*
SignalBinder_emit( SignalBinder* self, PyObject* args, PyObject* kwargs )
{
    return SignalBinder__call__( self, args, kwargs );
}


static PyMethodDef
SignalBinder_methods[] = {
    { "connect",
      ( PyCFunction )SignalBinder_connect, METH_O,
      "Connect a handler to the signal." },
    { "disconnect",
      ( PyCFunction )SignalBinder_disconnect, METH_O,
      "Disconnect a previously connected signal." },
    { "emit",
      ( PyCFunction )SignalBinder_emit, METH_VARARGS | METH_KEYWORDS,
      "Emit the signal with arguments and keywords." },
    { 0 } // sentinel
};


PyTypeObject SignalBinder_Type = {
    PyObject_HEAD_INIT( 0 )
    0,                                      /* ob_size */
    "signalbinder.SignalBinder",            /* tp_name */
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
signalbinder_methods[] = {
    { 0 } // Sentinel
};


PyMODINIT_FUNC
initsignalbinder( void )
{
    PyObject* mod = Py_InitModule( "signalbinder", signalbinder_methods );
    if( !mod )
        return;
    if( PyType_Ready( &SignalBinder_Type ) )
        return;
    PyObject* pytype = reinterpret_cast<PyObject*>( &SignalBinder_Type );
    Py_INCREF( pytype );
    PyModule_AddObject( mod, "SignalBinder", pytype );
}


} // extern "C"

