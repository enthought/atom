/*-----------------------------------------------------------------------------
|  Copyright (c) 2012, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "pythonhelpers.h"


using namespace PythonHelpers;


#define ATOM_BIT        ( static_cast<size_t>( 0 ) )
#define INDEX_OFFSET    ( static_cast<size_t>( 1 ) )
#define SIZE_T_BITS     ( static_cast<size_t>( sizeof( size_t ) * 8 ) )


extern "C" {


// pre-allocated PyString objects
static PyObject* _atom_members;
static PyObject* _undefined;
static PyObject* _notify;
static PyObject* _default;
static PyObject* _validate;


/*
The data array for a CAtom is malloced large enough to hold `count`
number of object pointers PLUS enough extra pointers to use as a bit
field for storing whether notifications are enabled for the member.
For example, and atom with 10 members will have a `data` block of
44 bytes on a 32bit system: 40 bytes for the data, and 4 bytes for
the bit field. If the number of members increases to 32, the data
block will have 136 bytes: 128 for the data, and 8 bytes for the bit
field. The number of bits needed for the bitfield is always 1 greater
than the `count`, to account for the bit needed for the atom object
as a whole.
*/
typedef struct {
    PyObject_HEAD
} PyNull;

static PyObject* _py_null;


typedef struct {
    PyObject_HEAD
    Py_ssize_t count;
    PyObject** data;
} CAtom;


typedef struct {
    PyObject_HEAD
    size_t flags;
    Py_ssize_t index;
    PyObject* name;
} Member;


typedef struct {
    PyObject_HEAD
    PyObject* object;
    PyObject* name;
    PyObject* oldvalue;
    PyObject* newvalue;
} MemberChange;


enum MemberFlag
{
    MemberHasDefault = 0x1,
    MemberHasValidate = 0x2
};


static PyObject*
PyNull_repr( PyNull* self )
{
    return PyString_FromString( "<null>" );
}


PyTypeObject PyNull_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "catom.null",                           /* tp_name */
    sizeof( PyNull ),                       /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)0,                          /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)PyNull_repr,                  /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)0,                         /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
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


static PyObject*
MemberChange_new( PyTypeObject* type, PyObject* args, PyObject* kwargs )
{
    PyObject* object;
    PyObject* name;
    PyObject* oldvalue;
    PyObject* newvalue;
    static char* kwds[] = {"object", "name", "old", "new", 0};
    if( !PyArg_ParseTupleAndKeywords(
        args, kwargs, "OOOO", kwds, &object, &name, &oldvalue, &newvalue ) )
        return 0;
    PyObject* self = PyType_GenericNew( type, args, kwargs );
    if( !self )
        return 0;
    MemberChange* change = reinterpret_cast<MemberChange*>( self );
    change->object = newref( object );
    change->name = newref( name );
    change->oldvalue = newref( oldvalue );
    change->newvalue = newref( newvalue );
    return self;
}


static void
MemberChange_clear( MemberChange* self )
{
    Py_CLEAR( self->object );
    Py_CLEAR( self->name );
    Py_CLEAR( self->oldvalue );
    Py_CLEAR( self->newvalue );
}


static int
MemberChange_traverse( MemberChange* self, visitproc visit, void* arg )
{
    Py_VISIT( self->object );
    Py_VISIT( self->name );
    Py_VISIT( self->oldvalue );
    Py_VISIT( self->newvalue );
    return 0;
}


static void
MemberChange_dealloc( MemberChange* self )
{
    PyObject_GC_UnTrack( self );
    MemberChange_clear( self );
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
MemberChange_get_object( MemberChange* self, void* context )
{
    if( !self->object )
        Py_RETURN_NONE;
    return newref( self->object );
}


static PyObject*
MemberChange_get_name( MemberChange* self, void* context )
{
    if( !self->name )
        Py_RETURN_NONE;
    return newref( self->name );
}


static PyObject*
MemberChange_get_oldvalue( MemberChange* self, void* context )
{
    if( !self->oldvalue )
        Py_RETURN_NONE;
    return newref( self->oldvalue );
}


static PyObject*
MemberChange_get_newvalue( MemberChange* self, void* context )
{
    if( !self->newvalue )
        Py_RETURN_NONE;
    return newref( self->newvalue );
}


static PyGetSetDef
MemberChange_getset[] = {
    { "object", ( getter )MemberChange_get_object, 0,
      "Get atom object whose member has changed." },
    { "name", ( getter )MemberChange_get_name, 0,
      "Get the name of the member which changed on the atom object." },
    { "old", ( getter )MemberChange_get_oldvalue, 0,
      "Get the old value of the member." },
    { "new", ( getter )MemberChange_get_newvalue, 0,
      "Get the new value of the member." },
    { 0 } // sentinel
};


static PyObject*
MemberChange_repr( MemberChange* self )
{
    PyObjectPtr objectstr;
    if( self->object )
         objectstr = PyObject_Repr( self->object );
    else
         objectstr = PyObject_Repr( _py_null );
    if( !objectstr )
        return 0;
    PyObjectPtr namestr;
    if( self->name )
        namestr = PyObject_Repr( self->name );
    else
        namestr = PyObject_Repr( _py_null );
    if( !namestr )
        return 0;
    PyObjectPtr oldstr;
    if( self->oldvalue )
        oldstr = PyObject_Repr( self->oldvalue );
    else
        oldstr = PyObject_Repr( _py_null );
    if( !oldstr )
        return 0;
    PyObjectPtr newstr;
    if( self->newvalue )
        newstr = PyObject_Repr( self->newvalue );
    else
        newstr = PyObject_Repr( _py_null );
    if( !newstr )
        return 0;
    PyObject* res = PyString_FromFormat(
        "MemberChange(object=%s, name=%s, old=%s, new=%s)",
        PyString_AsString( objectstr.get() ),
        PyString_AsString( namestr.get() ),
        PyString_AsString( oldstr.get() ),
        PyString_AsString( newstr.get() )
    );
    return res;
}


PyTypeObject MemberChange_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "catom.MemberChange",                   /* tp_name */
    sizeof( MemberChange ),                 /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)MemberChange_dealloc,       /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    (cmpfunc)0,                             /* tp_compare */
    (reprfunc)MemberChange_repr,            /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)0,                         /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)MemberChange_traverse,    /* tp_traverse */
    (inquiry)MemberChange_clear,            /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)0,                 /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    MemberChange_getset,                    /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)0,                        /* tp_descr_get */
    (descrsetfunc)0,                        /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)MemberChange_new,              /* tp_new */
    (freefunc)PyObject_GC_Del,              /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};


static int
Member_Check( PyObject* member );

static int
Member__set__( PyObject* self, PyObject* owner, PyObject* value );


inline bool
get_notify_bit( CAtom* atom, size_t bit )
{
    size_t block = bit / SIZE_T_BITS;
    size_t offset = bit % SIZE_T_BITS;
    size_t bits = reinterpret_cast<size_t>( atom->data[ atom->count + block ] );
    return bits & ( 1 << offset );
}


inline void
set_notify_bit( CAtom* atom, size_t bit, bool set )
{
    size_t block = bit / SIZE_T_BITS;
    size_t offset = bit % SIZE_T_BITS;
    size_t bits = reinterpret_cast<size_t>( atom->data[ atom->count + block ] );
    if( set )
        bits |= ( 1 << offset );
    else
        bits &= ~( 1 << offset );
    atom->data[ atom->count + block ] = reinterpret_cast<PyObject*>( bits );
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
        return py_expected_type_fail( membersptr.get(), "dict" );
    PyObjectPtr selfptr( PyType_GenericNew( type, args, kwargs ) );
    if( !selfptr )
        return 0;
    Py_ssize_t count = membersptr.size();
    if( count > 0 )
    {
        // count + 1 accounts for the atom bit.
        size_t blocks = ( count + 1 ) / SIZE_T_BITS;
        size_t extra = ( count + 1 ) % SIZE_T_BITS;
        if( extra > 0 )
            ++blocks;
        void* data = PyMem_MALLOC( sizeof( PyObject* ) * ( count + blocks ) );
        if( !data )
            return PyErr_NoMemory();
        memset( data, 0, sizeof( PyObject* ) * ( count + blocks ) );
        CAtom* atom = reinterpret_cast<CAtom*>( selfptr.get() );
        atom->data = reinterpret_cast<PyObject**>( data );
        atom->count = count;
    }
    return selfptr.release();
}


static void
CAtom_clear( CAtom* self )
{
    Py_ssize_t count = self->count;
    for( Py_ssize_t i = 0; i < count; ++i )
    {
        Py_CLEAR( self->data[ i ] );
    }
}


static int
CAtom_traverse( CAtom* self, visitproc visit, void* arg )
{
    Py_ssize_t count = self->count;
    for( Py_ssize_t i = 0; i < count; ++i )
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
        PyMem_FREE( self->data );
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
lookup_member( CAtom* self, PyObject* name )
{
    PyObjectPtr member(
        PyObject_GetAttr( reinterpret_cast<PyObject*>( self->ob_type ), name )
    );
    if( !member )
        return 0;
    if( !Member_Check( member.get() ) )
        return py_expected_type_fail( member.get(), "Member" );
    return member.release();
}


static PyObject*
CAtom_lookup_member( CAtom* self, PyObject* name )
{
    if( !PyString_Check( name ) )
        return py_expected_type_fail( name, "str" );
    return lookup_member( self, name );
}


static PyObject*
CAtom_notifications_enabled( CAtom* self, PyObject* args )
{
    PyObject* name = 0;
    if( !PyArg_ParseTuple( args, "|S", &name ) )
        return 0;
    if( self->count == 0 )
        Py_RETURN_FALSE;
    size_t notifybit = ATOM_BIT;
    if( name )
    {
        PyObjectPtr memberptr( lookup_member( self, name ) );
        if( !memberptr )
            return 0;
        Member* member = reinterpret_cast<Member*>( memberptr.get() );
        notifybit = member->index + INDEX_OFFSET;
    }
    if( get_notify_bit( self, notifybit ) )
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}


static PyObject*
toggle_notifications( CAtom* self, PyObject* args, bool enable )
{
    PyObject* name = 0;
    if( !PyArg_ParseTuple( args, "|S", &name ) )
        return 0;
    if( self->count == 0 )
        Py_RETURN_FALSE;
    size_t notifybit = ATOM_BIT;
    if( name )
    {
        PyObjectPtr memberptr( lookup_member( self, name ) );
        if( !memberptr )
            return 0;
        Member* member = reinterpret_cast<Member*>( memberptr.get() );
        notifybit = member->index + INDEX_OFFSET;
    }
    set_notify_bit( self, notifybit, enable );
    Py_RETURN_TRUE;
}


static PyObject*
CAtom_enable_notifications( CAtom* self, PyObject* args )
{
    return toggle_notifications( self, args, true );
}


static PyObject*
CAtom_disable_notifications( CAtom* self, PyObject* args )
{
    return toggle_notifications( self, args, false );
}


static PyObject *
CAtom_update_members( CAtom* self, PyObject* args, PyObject* kwargs )
{
    if( args && PyTuple_GET_SIZE( args ) )
        return py_type_fail( "update_members() takes no positional args" );
    if( !kwargs )
        Py_RETURN_NONE;
    Py_ssize_t pos = 0;
    PyObject* key;
    PyObject* value;
    PyObjectPtr memberptr;
    PyObject* pyself = reinterpret_cast<PyObject*>( self );
    while( PyDict_Next( kwargs, &pos, &key, &value ) )
    {
        if( !PyString_Check( key ) )
            return py_expected_type_fail( key, "str" );
        memberptr = lookup_member( self, key );
        if( !memberptr )
            return 0;
        if( Member__set__( memberptr.get(), pyself, value ) < 0 )
            return 0;
    }
    Py_RETURN_NONE;
}


static PyObject*
CAtom_notify( CAtom* self, PyObject* change )
{
    // Reimplement in a subclass as needed.
    Py_RETURN_NONE;
}


static PyObject*
CAtom_sizeof( CAtom* self, PyObject* args )
{
    // count + 1 accounts for the atom bit
    size_t blocks = ( self->count + 1 ) / SIZE_T_BITS;
    size_t extra = ( self->count + 1 ) % SIZE_T_BITS;
    if( extra > 0 )
        ++blocks;
    Py_ssize_t size = self->ob_type->tp_basicsize;
    size += sizeof( PyObject* ) * ( self->count + blocks );
    return PyInt_FromSsize_t( size );
}


static PyMethodDef
CAtom_methods[] = {
    { "lookup_member",
      ( PyCFunction )CAtom_lookup_member, METH_O,
      "Lookup a member on the atom." },
    { "notifications_enabled",
      ( PyCFunction )CAtom_notifications_enabled, METH_VARARGS,
      "Get whether notification is enabled for the atom." },
    { "enable_notifications",
      ( PyCFunction )CAtom_enable_notifications, METH_VARARGS,
      "Enabled notifications for the atom." },
    { "disable_notifications",
      ( PyCFunction )CAtom_disable_notifications, METH_VARARGS,
      "Disable notifications for the atom." },
    { "update_members", ( PyCFunction )CAtom_update_members,
      METH_VARARGS | METH_KEYWORDS,
      "Update the atom members with info from keyword values." },
    { "notify", ( PyCFunction )CAtom_notify, METH_O,
      "Called when a notifying member is changed. Reimplement as needed." },
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


static PyObject*
Member_new( PyTypeObject* type, PyObject* args, PyObject* kwargs )
{
    PyObjectPtr selfptr( PyType_GenericNew( type, args, kwargs ) );
    if( !selfptr )
        return 0;
    Member* member = reinterpret_cast<Member*>( selfptr.get() );
    member->name = newref( _undefined );
    return selfptr.release();
}


static void
Member_dealloc( Member* self )
{
    Py_DECREF( self->name );
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
Member__get__( PyObject* self, PyObject* owner, PyObject* type )
{
    if( !owner )
        return newref( self );
    if( !PyObject_TypeCheck( owner, &CAtom_Type ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    if( member->index >= atom->count )
        return py_no_attr_fail( owner, PyString_AsString( member->name ) );
    PyObjectPtr value( atom->data[ member->index ] );
    if( value )
        return value.incref_release();
    if( member->flags & MemberHasDefault )
    {
        PyObjectPtr selfptr( newref( self ) );
        PyObjectPtr callable( selfptr.get_attr( _default ) );
        if( !callable )
            return 0;
        PyTuplePtr args( 2 );
        if( !args )
            return 0;
        args.initialize( 0, newref( owner ) );
        args.initialize( 1, newref( member->name ) );
        value = callable( args );
        if( !value )
            return 0;
        if( value != _py_null )
            atom->data[ member->index ] = value.newref();
        return value.release();
    }
    return newref( _py_null );
}


static int
Member__set__( PyObject* self, PyObject* owner, PyObject* value )
{
    if( !PyObject_TypeCheck( owner, &CAtom_Type ) )
    {
        py_expected_type_fail( owner, "CAtom" );
        return -1;
    }
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    if( member->index >= atom->count )
    {
        py_no_attr_fail( owner, PyString_AsString( member->name ) );
        return -1;
    }
    PyObjectPtr oldptr( atom->data[ member->index ] );
    PyObjectPtr newptr( value );
    if( newptr == _py_null )
        newptr.release();
    if( oldptr == newptr )
    {
        oldptr.release();
        newptr.release();
        return 0;
    }
    newptr.xincref();
    if( member->flags & MemberHasValidate )
    {
        PyObjectPtr selfptr( newref( self ) );
        PyObjectPtr callable( selfptr.get_attr( _validate ) );
        if( !callable )
            return -1;
        PyTuplePtr args( 4 );
        if( !args )
            return -1;
        args.initialize( 0, newref( owner ) );
        args.initialize( 1, newref( member->name ) );
        if( oldptr )
            args.initialize( 2, oldptr );
        else
            args.initialize( 2, newref( _py_null ) );
        if( newptr )
            args.initialize( 3, newptr );
        else
            args.initialize( 3, newref( _py_null ) );
        newptr = callable( args );
        if( !newptr )
            return -1;
        if( newptr == _py_null )
            newptr.decref_release();
    }
    atom->data[ member->index ] = newptr.xnewref();
    size_t member_bit = member->index + INDEX_OFFSET;
    if( get_notify_bit( atom, ATOM_BIT ) && get_notify_bit( atom, member_bit ) )
    {
        if( !oldptr )
            oldptr.set( newref( _py_null ) );
        if( !newptr )
            newptr.set( newref( _py_null ) );
        if( oldptr != newptr && !oldptr.richcompare( newptr, Py_EQ ) )
        {
            PyObjectPtr ownerptr( newref( owner ) );
            PyObjectPtr callable( ownerptr.get_attr( _notify ) );
            if( !callable )
                return -1;
            PyTuplePtr args( 1 );
            if( !args )
                return -1;
            PyObjectPtr changeptr( PyType_GenericNew( &MemberChange_Type, 0, 0 ) );
            if( !changeptr )
                return -1;
            MemberChange* change = reinterpret_cast<MemberChange*>( changeptr.get() );
            change->object = ownerptr.release();
            change->name = newref( member->name );
            change->oldvalue = oldptr.release();
            change->newvalue = newptr.release();
            args.initialize( 0, changeptr );
            PyObjectPtr result( callable( args ) );
            if( !result )
                return -1;
        }
    }
    return 0;
}


static PyObject*
Member_get_name( Member* self, void* context )
{
    return newref( self->name );
}


static int
Member_set_name( Member* self, PyObject* value, void* context )
{
    if( !value )
        value = _undefined;
    else if( !PyString_Check( value ) )
    {
        py_expected_type_fail( value, "string" );
        return -1;
    }
    PyObject* old = self->name;
    self->name = value;
    Py_INCREF( value );
    Py_DECREF( old );
    return 0;
}


static PyObject*
Member_get_index( Member* self, void* context )
{
    return PyInt_FromSsize_t( self->index );
}


static int
Member_set_index( Member* self, PyObject* value, void* context )
{
    if( !value )
    {
        self->index = 0;
        return 0;
    }
    if( !PyInt_Check( value ) )
    {
        py_expected_type_fail( value, "int" );
        return -1;
    }
    Py_ssize_t index = PyInt_AsSsize_t( value );
    if( index < 0 && PyErr_Occurred() )
        return -1;
    self->index = index < 0 ? 0 : index;
    return 0;
}


static int
toggle_member_flag( Member* self, PyObject* value, MemberFlag flag )
{
    if( !value )
        value = Py_False;
    else if( !PyBool_Check( value ) )
    {
        py_expected_type_fail( value, "bool" );
        return -1;
    }
    if( value == Py_True )
        self->flags |= flag;
    else
        self->flags &= ~flag;
    return 0;
}


static PyObject*
Member_get_has_default( Member* self, void* context )
{
    if( self->flags & MemberHasDefault )
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}


static int
Member_set_has_default( Member* self, PyObject* value, void* context )
{
    return toggle_member_flag( self, value, MemberHasDefault );
}


static PyObject*
Member_get_has_validate( Member* self, void* context )
{
    if( self->flags & MemberHasValidate )
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}


static int
Member_set_has_validate( Member* self, PyObject* value, void* context )
{
    return toggle_member_flag( self, value, MemberHasValidate );
}


static PyGetSetDef
Member_getset[] = {
    { "_name",
      ( getter )Member_get_name, ( setter )Member_set_name,
      "Get and set the name to which the member is bound. Use with extreme caution!" },
    { "_index",
      ( getter )Member_get_index, ( setter )Member_set_index,
      "Get and set the index to which the member is bound. Use with extreme caution!" },
    { "has_default",
      ( getter )Member_get_has_default, ( setter )Member_set_has_default,
      "Get and set whether the member has a default function or method." },
    { "has_validate",
      ( getter )Member_get_has_validate, ( setter )Member_set_has_validate,
      "Get and set whether the member has a validate function or method." },
    { 0 } // sentinel
};


PyTypeObject Member_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "catom.Member",                        /* tp_name */
    sizeof( Member ),                      /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)Member_dealloc,            /* tp_dealloc */
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
    Member_getset,                         /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)Member__get__,           /* tp_descr_get */
    (descrsetfunc)Member__set__,           /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)Member_new,                   /* tp_new */
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
Member_Check( PyObject* member )
{
    return PyObject_TypeCheck( member, &Member_Type );
}


static PyMethodDef
catom_methods[] = {
    { 0 } // Sentinel
};


PyMODINIT_FUNC
initcatom( void )
{
    PyObject* mod = Py_InitModule( "catom", catom_methods );
    if( !mod )
        return;
    _atom_members = PyString_FromString( "__atom_members__" );
    if( !_atom_members )
        return;
    _undefined = PyString_FromString( "<undefined>" );
    if( !_undefined )
        return;
    _notify = PyString_FromString( "notify" );
    if( !_notify )
        return;
    _default = PyString_FromString( "default" );
    if( !_default )
        return;
    _validate = PyString_FromString( "validate" );
    if( !_validate )
        return;
    if( PyType_Ready( &MemberChange_Type ) )
        return;
    if( PyType_Ready( &CAtom_Type ) )
        return;
    if( PyType_Ready( &Member_Type ) )
        return;
    if( PyType_Ready( &PyNull_Type ) )
        return;
    _py_null = PyType_GenericNew( &PyNull_Type, 0, 0 );
    if( !_py_null )
        return;
    Py_INCREF( &MemberChange_Type );
    Py_INCREF( &CAtom_Type );
    Py_INCREF( &Member_Type );
    Py_INCREF( _py_null );
    PyModule_AddObject( mod, "MemberChange", reinterpret_cast<PyObject*>( &MemberChange_Type ) );
    PyModule_AddObject( mod, "CAtom", reinterpret_cast<PyObject*>( &CAtom_Type ) );
    PyModule_AddObject( mod, "Member", reinterpret_cast<PyObject*>( &Member_Type ) );
    PyModule_AddObject( mod, "null", _py_null );
}


} // extern "C"

