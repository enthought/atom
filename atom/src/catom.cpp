/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma clang diagnostic ignored "-Wdeprecated-writable-strings"
#include "catom.h"
#include "member.h"


extern "C" {


static PyObject* _atom_members;
static PyObject* _re_module;


typedef struct {
    PyObject_HEAD
    PyObject* im_func;
    PyObject* im_selfref;
} MethodWrapper;


static PyObject*
re_compile( PyObject* pystr )
{
    PyObjectPtr compile( PyObject_GetAttrString( _re_module, "compile" ) );
    if( !compile )
        return 0;
    PyTuplePtr args( PyTuple_New( 1 ) );
    if( !args )
        return 0;
    args.initialize( 0, newref( pystr ) );
    PyObjectPtr result( compile( args ) );
    return result.release();
}


static int
MethodWrapper_Check( PyObject* obj );


static void
MethodWrapper_dealloc( MethodWrapper* self )
{
    Py_CLEAR( self->im_selfref );
    Py_CLEAR( self->im_func );
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
MethodWrapper__call__( MethodWrapper* self, PyObject* args, PyObject* kwargs )
{
    PyObject* im_self = PyWeakref_GET_OBJECT( self->im_selfref );
    if( im_self != Py_None )
    {
        PyObject* type = reinterpret_cast<PyObject*>( im_self->ob_type );
        PyObjectPtr method( PyMethod_New( self->im_func, im_self, type ) );
        if( !method )
            return 0;
        return PyObject_Call( method.get(), args, kwargs );
    }
    Py_RETURN_NONE;
}


static PyObject*
MethodWrapper_richcompare( MethodWrapper* self, PyObject* other, int op )
{
    if( op == Py_EQ )
    {
        if( PyMethod_Check( other ) && PyMethod_GET_SELF( other ) )
        {
            if( ( self->im_func == PyMethod_GET_FUNCTION( other ) ) &&
                ( PyWeakref_GET_OBJECT( self->im_selfref ) == PyMethod_GET_SELF( other ) ) )
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        else if( MethodWrapper_Check( other ) )
        {
            MethodWrapper* wrapper = reinterpret_cast<MethodWrapper*>( other );
            if( ( self->im_func == wrapper->im_func ) &&
                ( self->im_selfref == wrapper->im_selfref ) )
                Py_RETURN_TRUE;
            Py_RETURN_FALSE;
        }
        else
            Py_RETURN_FALSE;
    }
    Py_RETURN_NOTIMPLEMENTED;
}


static int
MethodWrapper__nonzero__( MethodWrapper* self )
{
    if( PyWeakref_GET_OBJECT( self->im_selfref ) != Py_None )
        return 1;
    return 0;
}


PyNumberMethods MethodWrapper_as_number = {
     ( binaryfunc )0,                       /* nb_add */
     ( binaryfunc )0,                       /* nb_subtract */
     ( binaryfunc )0,                       /* nb_multiply */
     ( binaryfunc )0,                       /* nb_divide */
     ( binaryfunc )0,                       /* nb_remainder */
     ( binaryfunc )0,                       /* nb_divmod */
     ( ternaryfunc )0,                      /* nb_power */
     ( unaryfunc )0,                        /* nb_negative */
     ( unaryfunc )0,                        /* nb_positive */
     ( unaryfunc )0,                        /* nb_absolute */
     ( inquiry )MethodWrapper__nonzero__    /* nb_nonzero */
};


PyTypeObject MethodWrapper_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "MethodWrapper",                        /* tp_name */
    sizeof( MethodWrapper ),                /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)MethodWrapper_dealloc,      /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)0,                            /* tp_repr */
    (PyNumberMethods*)&MethodWrapper_as_number, /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)MethodWrapper__call__,     /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)0,                        /* tp_traverse */
    (inquiry)0,                             /* tp_clear */
    (richcmpfunc)MethodWrapper_richcompare, /* tp_richcompare */
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
    (freefunc)PyObject_Del,                 /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


static int
MethodWrapper_Check( PyObject* obj )
{
    return PyObject_TypeCheck( obj, &MethodWrapper_Type );
}


static PyObject*
CAtom_new( PyTypeObject* type, PyObject* args, PyObject* kwargs )
{
    PyDictPtr membersptr(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !membersptr )
        return 0;
    if( !membersptr.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr selfptr( PyType_GenericNew( type, args, kwargs ) );
    if( !selfptr )
        return 0;
    CAtom* atom = reinterpret_cast<CAtom*>( selfptr.get() );
    uint32_t count = static_cast<uint32_t>( membersptr.size() );
    if( count > 0 )
    {
        if( count > MAX_MEMBER_COUNT )
            return py_type_fail( "too many members" );
        size_t size = sizeof( PyObject* ) * count;
        void* data = PyObject_MALLOC( size );
        if( !data )
            return PyErr_NoMemory();
        memset( data, 0, size );
        atom->data = reinterpret_cast<PyObject**>( data );
        atom->count = count;
    }
    if( kwargs )
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while( PyDict_Next( kwargs, &pos, &key, &value ) )
        {
            if( !selfptr.set_attr( key, value ) )
                return 0;
        }
    }
    set_atom_notify_bit( atom, true );
    return selfptr.release();
}


static void
CAtom_clear( CAtom* self )
{
    uint32_t count = get_atom_count( self );
    for( uint32_t i = 0; i < count; ++i )
    {
        Py_CLEAR( self->data[ i ] );
    }
}


static int
CAtom_traverse( CAtom* self, visitproc visit, void* arg )
{
    uint32_t count = get_atom_count( self );
    for( uint32_t i = 0; i < count; ++i )
    {
        Py_VISIT( self->data[ i ] );
    }
    return 0;
}


static void
CAtom_dealloc( CAtom* self )
{
    PyObject_GC_UnTrack( self );
    CAtom_clear( self );
    if( self->data )
        PyObject_FREE( self->data );
    delete self->observers;
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
CAtom_lookup_member( CAtom* self, PyObject* name )
{
    if( !PyString_Check( name ) )
        return py_expected_type_fail( name, "str" );
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr member( members.get_item( name ) );
    if( !member )
        Py_RETURN_NONE;
    if( !Member_Check( member.get() ) )
        Py_RETURN_NONE;
    return member.release();
}


static PyObject*
CAtom_notifications_enabled( CAtom* self )
{
    if( get_atom_notify_bit( self ) )
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}


static PyObject*
CAtom_set_notifications_enabled( CAtom* self, PyObject* arg )
{
    if( !PyBool_Check( arg ) )
        return py_expected_type_fail( arg, "bool" );
    bool old = get_atom_notify_bit( self );
    set_atom_notify_bit( self, arg == Py_True ? true : false );
    if( old )
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}


static PyObject*
wrap_callback( PyObject* callback )
{
    if( PyMethod_Check( callback ) && PyMethod_GET_SELF( callback ) )
    {
        PyObject* wr = PyWeakref_NewRef( PyMethod_GET_SELF( callback ), 0 );
        if( !wr )
            return 0;
        PyObject* pywrapper = PyType_GenericNew( &MethodWrapper_Type, 0, 0 );
        if( !pywrapper )
            return 0;
        MethodWrapper* wrapper = reinterpret_cast<MethodWrapper*>( pywrapper );
        wrapper->im_func = newref( PyMethod_GET_FUNCTION( callback ) );
        wrapper->im_selfref = wr;
        return pywrapper;
    }
    return newref( callback );
}


int
observe_fast( CAtom* atom, PyObject* name, PyObject* callback )
{
    PyObjectPtr topicptr( newref( name ) );
    PyObjectPtr callbackptr( wrap_callback( callback ) );
    if( !callbackptr )
        return -1;
    if( !atom->observers )
        atom->observers = new ObserverPool();
    atom->observers->add( topicptr, callbackptr );
    return 0;
}


int
unobserve_fast( CAtom* atom, PyObject* name, PyObject* callback )
{
    if( !atom->observers )
        return 0;
    PyObjectPtr topicptr( newref( name ) );
    PyObjectPtr callbackptr( newref( callback ) );
    atom->observers->remove( topicptr, callbackptr );
    return 0;
}


static PyObject*
observe_simple( CAtom* self, PyObject* name, PyObject* callback )
{
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr memberptr( members.get_item( name ) );
    if( memberptr && Member_Check( memberptr.get() ) )
    {
        // Use the member name since it's guaranteed to be interned and
        // will allow matching via pointer cmp instead of richcmp
        Member* member = reinterpret_cast<Member*>( memberptr.get() );
        PyObjectPtr topicptr( newref( member->name ) );
        PyObjectPtr callbackptr( wrap_callback( callback ) );
        if( !callbackptr )
            return 0;
        if( !self->observers )
            self->observers = new ObserverPool();
        self->observers->add( topicptr, callbackptr );
    }
    Py_RETURN_NONE;
}


static PyObject*
unobserve_simple( CAtom* self, PyObject* name, PyObject* callback )
{
    if( !self->observers )
        Py_RETURN_NONE;
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr memberptr( members.get_item( name ) );
    if( memberptr && Member_Check( memberptr.get() ) )
    {
        Member* member = reinterpret_cast<Member*>( memberptr.get() );
        PyObjectPtr topicptr( newref( member->name ) );
        PyObjectPtr callbackptr( newref( callback ) );
        self->observers->remove( topicptr, callbackptr );
    }
    Py_RETURN_NONE;
}


static PyObject*
observe_regex( CAtom* self, PyObject* regex, PyObject* callback )
{
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr rgx( re_compile( regex ) );
    if( !rgx )
        return 0;
    PyObjectPtr matchfunc( rgx.get_attr( "match" ) );
    if( !matchfunc )
        return 0;
    PyTuplePtr argtuple( PyTuple_New( 1 ));
    if( !argtuple )
        return 0;
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    PyObjectPtr callbackptr( wrap_callback( callback ) );
    if( !callbackptr )
        return 0;
    while( PyDict_Next( members.get(), &pos, &key, &value ) )
    {
        if( Member_Check( value ) && PyString_Check( key ) )
        {
            argtuple.set_item( 0, newref( key ) );
            PyObjectPtr res( matchfunc( argtuple ) );
            if( !res )
                return 0;
            if( res != Py_None )
            {
                Member* member = reinterpret_cast<Member*>( value );
                PyObjectPtr topicptr( newref( member->name ) );
                if( !self->observers )
                    self->observers = new ObserverPool();
                self->observers->add( topicptr, callbackptr );
            }
        }
    }
    Py_RETURN_NONE;
}


static PyObject*
unobserve_regex( CAtom* self, PyObject* regex, PyObject* callback )
{
    if( !self->observers )
        Py_RETURN_NONE;
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
        return py_bad_internal_call( "atom members" );
    PyObjectPtr rgx( re_compile( regex ) );
    if( !rgx )
        return 0;
    PyObjectPtr matchfunc( rgx.get_attr( "match" ) );
    if( !matchfunc )
        return 0;
    PyTuplePtr argtuple( PyTuple_New( 1 ));
    if( !argtuple )
        return 0;
    PyObjectPtr callbackptr( newref( callback ) );
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    while( PyDict_Next( members.get(), &pos, &key, &value ) )
    {
        if( Member_Check( value ) && PyString_Check( key ) )
        {
            argtuple.set_item( 0, newref( key ) );
            PyObjectPtr res( matchfunc( argtuple ) );
            if( !res )
                return 0;
            if( res != Py_None )
            {
                Member* member = reinterpret_cast<Member*>( value );
                PyObjectPtr topicptr( newref( member->name ) );
                self->observers->remove( topicptr, callbackptr );
            }
        }
    }
    Py_RETURN_NONE;
}


static PyObject*
CAtom_observe( CAtom* self, PyObject* args, PyObject* kwargs )
{
    PyObject* name;
    PyObject* callback;
    PyObject* regex = Py_False;
    static char* kwds[] = { "name", "callback", "regex", 0 };
    if( !PyArg_ParseTupleAndKeywords(
        args, kwargs, "SO|O!", kwds, &name, &callback, &PyBool_Type, &regex ) )
        return 0;
    if( regex == Py_False )
        return observe_simple( self, name, callback );
    return observe_regex( self, name, callback );
}


static PyObject*
CAtom_unobserve( CAtom* self, PyObject* args, PyObject* kwargs )
{
    PyObject* name;
    PyObject* callback;
    PyObject* regex = Py_False;
    static char* kwds[] = { "name", "callback", "regex", 0 };
    if( !PyArg_ParseTupleAndKeywords(
        args, kwargs, "SO|O!", kwds, &name, &callback, &PyBool_Type, &regex ) )
        return 0;
    if( regex == Py_False )
        return unobserve_simple( self, name, callback );
    return unobserve_regex( self, name, callback );
}


static PyObject*
CAtom_notify_observers( CAtom* self, PyObject* args, PyObject* kwargs )
{
    if( PyTuple_GET_SIZE( args ) < 1 )
        return py_type_fail( "notify_observers() requires at least 1 argument" );
    PyObjectPtr nameptr( newref( PyTuple_GET_ITEM( args, 0 ) ) );
    if( !PyString_Check( nameptr.get() ) )
        return py_expected_type_fail( nameptr.get(), "str" );
    PyObject* type = reinterpret_cast<PyObject*>( self->ob_type );
    PyDictPtr members(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( type ), _atom_members )
    );
    if( !members )
        return 0;
    if( !members.check_exact() )
    {
        py_bad_internal_call( "atom members" );
        return 0;
    }
    PyObjectPtr memberptr( members.get_item( nameptr ) );
    if( memberptr && Member_Check( memberptr.get() ) )
    {
        Member* member = reinterpret_cast<Member*>( memberptr.get() );
        PyObjectPtr argsptr( PyTuple_GetSlice( args, 1, PyTuple_GET_SIZE( args ) ) );
        if( !argsptr )
            return 0;
        PyObjectPtr kwargsptr( xnewref( kwargs ) );
        if( notify_observers( member, self, argsptr, kwargsptr ) < 0 )
            return 0;
    }
    Py_RETURN_NONE;
}


static PyObject*
CAtom_sizeof( CAtom* self, PyObject* args )
{
    Py_ssize_t size = self->ob_type->tp_basicsize;
    size += sizeof( PyObject* ) * get_atom_count( self );
    if( self->observers )
        size += self->observers->py_sizeof();
    return PyInt_FromSsize_t( size );
}


static PyMethodDef
CAtom_methods[] = {
    { "lookup_member",
      ( PyCFunction )CAtom_lookup_member, METH_O,
      "Lookup a member on the atom." },
    { "notifications_enabled",
      ( PyCFunction )CAtom_notifications_enabled, METH_NOARGS,
      "Get whether notification is enabled for the atom." },
    { "set_notifications_enabled",
      ( PyCFunction )CAtom_set_notifications_enabled, METH_O,
      "Enable or disable notifications for the atom." },
    { "observe", ( PyCFunction )CAtom_observe, METH_VARARGS | METH_KEYWORDS,
      "Register an observer callback to observe changes on the given member(s)." },
    { "unobserve", ( PyCFunction )CAtom_unobserve, METH_VARARGS | METH_KEYWORDS,
      "Unregister an observer callback for the given member(s)." },
    { "notify_observers", ( PyCFunction )CAtom_notify_observers, METH_VARARGS | METH_KEYWORDS,
      "Call the registered observers for the given name with the given argument" },
    { "__sizeof__", ( PyCFunction )CAtom_sizeof, METH_NOARGS,
      "__sizeof__() -> size of object in memory, in bytes" },
    { 0 } // sentinel
};


PyTypeObject CAtom_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "catom.CAtom",                          /* tp_name */
    sizeof( CAtom ),                        /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)CAtom_dealloc,              /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)CAtom_traverse,           /* tp_traverse */
    (inquiry)CAtom_clear,                   /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)CAtom_methods,     /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)0,                        /* tp_descr_get */
    (descrsetfunc)0,                        /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)CAtom_new,                     /* tp_new */
    (freefunc)PyObject_GC_Del,              /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


int
import_catom()
{
    if( PyType_Ready( &MethodWrapper_Type ) < 0 )
        return -1;
    if( PyType_Ready( &CAtom_Type ) < 0 )
        return -1;
    _atom_members = PyString_FromString( "__atom_members__" );
    if( !_atom_members )
        return -1;
    _re_module = PyImport_ImportModule( "re" );
    if( !_re_module )
        return -1;
    return 0;
}


} // extern "C"

