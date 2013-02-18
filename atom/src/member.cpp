/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma clang diagnostic ignored "-Wdeprecated-writable-strings"
#include "catom.h"
#include "member.h"


extern "C" {

#define FREELIST_MAX 128
static int numfree = 0;
static MemberChange* freelist[ FREELIST_MAX ];


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
    static char* kwds[] = { "object", "name", "old", "new", 0 };
    if( !PyArg_ParseTupleAndKeywords(
        args, kwargs, "OOOO", kwds, &object, &name, &oldvalue, &newvalue ) )
        return 0;
    return MemberChange_New( object, name, oldvalue, newvalue );
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
    if( numfree < FREELIST_MAX )
        freelist[ numfree++ ] = self;
    else
        self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
MemberChange_get_object( MemberChange* self, void* context )
{
    if( !self->object )
        return newref( _py_null );
    return newref( self->object );
}


static PyObject*
MemberChange_get_name( MemberChange* self, void* context )
{
    if( !self->name )
        return newref( _py_null );
    return newref( self->name );
}


static PyObject*
MemberChange_get_oldvalue( MemberChange* self, void* context )
{
    if( !self->oldvalue )
        return newref( _py_null );
    return newref( self->oldvalue );
}


static PyObject*
MemberChange_get_newvalue( MemberChange* self, void* context )
{
    if( !self->newvalue )
        return newref( _py_null );
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


PyObject*
MemberChange_New( PyObject* object, PyObject* name, PyObject* oldvalue, PyObject* newvalue )
{
    PyObject* pychange;
    if( numfree > 0 )
    {
        PyObject* pychange = reinterpret_cast<PyObject*>( freelist[ --numfree ] );
        _Py_NewReference( pychange );
    }
    else
    {
        pychange = PyType_GenericAlloc( &MemberChange_Type, 0 );
        if( !pychange )
            return 0;
    }
    MemberChange* change = reinterpret_cast<MemberChange*>( pychange );
    change->object = newref( object );
    change->name = newref( name );
    change->oldvalue = newref( oldvalue );
    change->newvalue = newref( newvalue );
    return pychange;
}


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
Member_clear( Member* self )
{
    Py_CLEAR( self->name );
    Py_CLEAR( self->default_context );
    Py_CLEAR( self->validate_context );
    if( self->static_observers )
        self->static_observers->clear();
}


static int
Member_traverse( Member* self, visitproc visit, void* arg )
{
    Py_VISIT( self->name );
    Py_VISIT( self->default_context );
    Py_VISIT( self->validate_context );
    if( self->static_observers )
    {
        std::vector<PyObjectPtr>::iterator it;
        std::vector<PyObjectPtr>::iterator end = self->static_observers->end();
        for( it = self->static_observers->begin(); it != end; ++it )
        {
            Py_VISIT( it->get() );
        }
    }
    return 0;
}


static void
Member_dealloc( Member* self )
{
    PyObject_GC_UnTrack( self );
    Member_clear( self );
    delete self->static_observers;
    self->ob_type->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
Member_get_value( Member* self, PyObject* owner )
{
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    if( self->index >= get_atom_count( atom ) )
        return py_no_attr_fail( owner, PyString_AsString( self->name ) );
    PyObjectPtr value( atom->data[ self->index ] );
    if( value )
        return value.incref_release();
    return newref( _py_null );
}


static PyObject*
Member_set_value( Member* self, PyObject* args )
{
    if( PyTuple_GET_SIZE( args ) != 2 )
        return py_type_fail( "'set_value' takes exactly 2 arguments" );
    PyObject* owner = PyTuple_GET_ITEM( args, 0 );
    PyObject* value = PyTuple_GET_ITEM( args, 1 );
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    if( self->index >= get_atom_count( atom ) )
        return py_no_attr_fail( owner, PyString_AsString( self->name ) );
    if( value == _py_null )
        value = 0;
    PyObject* old = atom->data[ self->index ];
    atom->data[ self->index ] = xnewref( value );
    Py_XDECREF( old );
    Py_RETURN_NONE;
}


static PyObject*
Member_copy_static_observers( Member* self, PyObject* other )
{
    if( !Member_Check( other ) )
        return py_expected_type_fail( other, "Member" );
    Member* member = reinterpret_cast<Member*>( other );
    if( self == member )
        Py_RETURN_NONE;
    if( !member->static_observers )
    {
        delete self->static_observers;
        self->static_observers = 0;
    }
    else
    {
        if( !self->static_observers )
            self->static_observers = new std::vector<PyObjectPtr>( member->static_observers->size() );
        *self->static_observers = *member->static_observers;
    }
    Py_RETURN_NONE;
}


static PyObject*
Member_static_observers( Member* self )
{
    if( !self->static_observers )
        return PyTuple_New( 0 );
    std::vector<PyObjectPtr>& observers( *self->static_observers );
    size_t size = observers.size();
    PyObject* items = PyTuple_New( size );
    if( !items )
        return 0;
    for( size_t i = 0; i < size; ++i )
        PyTuple_SET_ITEM( items, i, observers[ i ].newref() );
    return items;
}


static PyObject*
Member_add_static_observer( Member* self, PyObject* name )
{
    if( self->modify_guard )
        return py_runtime_fail( "attempted to modify static observers during notification" );
    if( !PyString_Check( name ) )
        return py_expected_type_fail( name, "str" );
    if( !self->static_observers )
        self->static_observers = new std::vector<PyObjectPtr>();
    PyObjectPtr nameptr( newref( name ) );
    std::vector<PyObjectPtr>::iterator it;
    std::vector<PyObjectPtr>::iterator end = self->static_observers->end();
    for( it = self->static_observers->begin(); it != end; ++it )
    {
        if( *it == nameptr || it->richcompare( nameptr, Py_EQ ) )
            Py_RETURN_NONE;
    }
    self->static_observers->push_back( nameptr );
    Py_RETURN_NONE;
}


static PyObject*
Member_remove_static_observer( Member* self, PyObject* name )
{
    if( self->modify_guard )
        return py_runtime_fail( "attempted to modify static observers during notification" );
    if( !PyString_Check( name ) )
        return py_expected_type_fail( name, "str" );
    if( self->static_observers )
    {
        PyObjectPtr nameptr( newref( name ) );
        std::vector<PyObjectPtr>::iterator it;
        std::vector<PyObjectPtr>::iterator end = self->static_observers->end();
        for( it = self->static_observers->begin(); it != end; ++it )
        {
            if( *it == nameptr || it->richcompare( nameptr, Py_EQ ) )
            {
                self->static_observers->erase( it );
                if( self->static_observers->size() == 0 )
                {
                    delete self->static_observers;
                    self->static_observers = 0;
                }
                break;
            }
        }
    }
    Py_RETURN_NONE;
}


static PyObject*
Member_do_default( Member* self, PyObject* owner )
{
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    if( self->default_kind )
        return member_default( self, owner );
    return newref( _py_null );
}


static PyObject*
Member_do_validate( Member* self, PyObject* args )
{
    if( PyTuple_GET_SIZE( args ) != 3 )
        return py_type_fail( "do_validate() takes 3 arguments" );
    PyObject* owner = PyTuple_GET_ITEM( args, 0 );
    PyObject* oldvalue = PyTuple_GET_ITEM( args, 1 );
    PyObject* newvalue = PyTuple_GET_ITEM( args, 2 );
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    if( self->validate_kind )
        return member_validate( self, owner, oldvalue, newvalue );
    return newref( newvalue );
}


static PyObject*
Member_default( Member* self, PyObject* args )
{
    // reimplement in a subclass for Python-land user defaults.
    // This exists here purely for completeness.
    PyObject* owner;
    PyObject* name;
    if( !PyArg_ParseTuple( args, "OO", &owner, &name ) )
        return 0;
    return newref( _py_null );
}


static PyObject*
Member_validate( Member* self, PyObject* args )
{
    // reimplement in a subclass for Python-land user validation.
    // This exists here purely for completeness.
    PyObject* owner;
    PyObject* name;
    PyObject* oldvalue;
    PyObject* newvalue;
    if( !PyArg_ParseTuple( args, "OOOO", &owner, &name, &oldvalue, &newvalue) )
        return 0;
    return newref( newvalue );
}


static PyObject*
Member_clone( Member* self )
{
    // reimplement in a subclass if more control is needed.
    PyObject* pyclone = PyType_GenericNew( self->ob_type, 0, 0 );
    if( !pyclone )
        return 0;
    Member* clone = reinterpret_cast<Member*>( pyclone );
    clone->index = self->index;
    clone->name = newref( self->name );
    clone->default_kind = self->default_kind;
    clone->validate_kind = self->validate_kind;
    clone->default_context = xnewref( self->default_context );
    clone->validate_context = xnewref( self->validate_context );
    if( self->static_observers )
    {
        size_t size = self->static_observers->size();
        clone->static_observers = new std::vector<PyObjectPtr>( size );
        *clone->static_observers = *self->static_observers;
    }
    return pyclone;
}


static PyObject*
Member__get__( PyObject* self, PyObject* owner, PyObject* type )
{
    if( !owner )
        return newref( self );
    if( !CAtom_Check( owner ) )
        return py_expected_type_fail( owner, "CAtom" );
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    if( member->index >= get_atom_count( atom ) )
        return py_no_attr_fail( owner, PyString_AsString( member->name ) );
    PyObjectPtr value( atom->data[ member->index ] );  // borrow ref
    if( value )
        return value.incref_release();                 // take owned ref
    if( member->default_kind )
    {
        value = member_default( member, owner );       // value is 0 before assignment
        if( !value )
            return 0;
        if( value != _py_null )
            atom->data[ member->index ] = value.newref();  // take owned internal ref
        return value.release();                            // return owned ref to caller
    }
    return newref( _py_null );
}


static int
Member__set__( PyObject* self, PyObject* owner, PyObject* value )
{
    if( !CAtom_Check( owner ) )
    {
        py_expected_type_fail( owner, "CAtom" );
        return -1;
    }
    CAtom* atom = reinterpret_cast<CAtom*>( owner );
    Member* member = reinterpret_cast<Member*>( self );
    if( member->index >= get_atom_count( atom ) )
    {
        py_no_attr_fail( owner, PyString_AsString( member->name ) );
        return -1;
    }
    PyObjectPtr oldptr( atom->data[ member->index ] );    // borrow ref
    PyObjectPtr newptr( value != _py_null ? value : 0 );  // borrow ref
    if( oldptr == newptr )
    {
        oldptr.release();  // return borrowed ref
        newptr.release();  // return borrowed ref
        return 0;
    }
    oldptr.xincref();  // take owned ref
    newptr.xincref();  // take owned ref
    if( member->validate_kind )
    {
        if( !oldptr )
            oldptr.set( newref( _py_null ) );
        if( !newptr )
            newptr.set( newref( _py_null ) );
        newptr = member_validate( member, owner, oldptr.get(), newptr.get() );
        if( !newptr )
            return -1;
        if( newptr == _py_null )
            newptr.decref_release();
    }
    Py_XDECREF( atom->data[ member->index] );        // release internally owned ref
    atom->data[ member->index ] = newptr.xnewref();  // take internally owned ref
    if( get_atom_notify_bit( atom ) )
    {
        PyObjectPtr changeptr;
        if( member->static_observers )
        {
            if( !oldptr )
                oldptr.set( newref( _py_null ) );
            if( !newptr )
                newptr.set( newref( _py_null ) );
            if( oldptr == newptr || oldptr.richcompare( newptr, Py_EQ ) )
                return 0;
            changeptr.set( MemberChange_New( owner, member->name, oldptr.get(), newptr.get() ) );
            if( !changeptr )
                return -1;
            PyTuplePtr argsptr( PyTuple_New( 1 ) );
            if( !argsptr )
                return -1;
            argsptr.initialize( 0, changeptr );
            PyObjectPtr ownerptr( newref( owner ) );
            StaticModifyGuard guard( member );
            std::vector<PyObjectPtr>::iterator it;
            std::vector<PyObjectPtr>::iterator end = member->static_observers->end();
            for( it = member->static_observers->begin(); it != end; ++it )
            {
                PyObjectPtr method( ownerptr.get_attr( *it ) );
                if( !method )
                    return -1;
                if( !method( argsptr ) )
                    return -1;
            }
        }
        if( atom->observers )
        {
            PyObjectPtr nameptr( newref( member->name ) );
            if( atom->observers->has_topic( nameptr ) )
            {
                if( !changeptr )
                {
                    if( !oldptr )
                        oldptr.set( newref( _py_null ) );
                    if( !newptr )
                        newptr.set( newref( _py_null ) );
                    if( oldptr == newptr || oldptr.richcompare( newptr, Py_EQ ) )
                        return 0;
                    changeptr.set( MemberChange_New( owner, member->name, oldptr.get(), newptr.get() ) );
                    if( !changeptr )
                        return -1;
                }
                PyTuplePtr argsptr( PyTuple_New( 1 ) );
                if( !argsptr )
                    return -1;
                argsptr.initialize( 0, changeptr );
                PyObjectPtr kwargsptr( 0 );
                if( atom->observers->notify( nameptr, argsptr, kwargsptr ) < 0 )
                    return -1;
            }
        }
    }
    return 0;
}


static PyObject*
Member_get_name( Member* self, void* context )
{
    return newref( self->name );
}


static PyObject*
Member_set_member_name( Member* self, PyObject* value )
{
    if( !PyString_CheckExact( value ) )
    {
        py_expected_type_fail( value, "string" );
        return 0;
    }
    Py_INCREF( value ); // incref before interning or segfault!!!
    PyString_InternInPlace( &value );
    PyObject* old = self->name;
    self->name = value;
    Py_DECREF( old );
    Py_RETURN_NONE;
}


static PyObject*
Member_get_index( Member* self, void* context )
{
    return PyInt_FromSsize_t( static_cast<Py_ssize_t>( self->index ) );
}


static PyObject*
Member_set_member_index( Member* self, PyObject* value )
{
    if( !PyInt_Check( value ) )
    {
        py_expected_type_fail( value, "int" );
        return 0;
    }
    Py_ssize_t index = PyInt_AsSsize_t( value );
    if( index < 0 && PyErr_Occurred() )
        return 0;
    self->index = static_cast<uint32_t>( index < 0 ? 0 : index );
    Py_RETURN_NONE;
}


static PyObject*
Member_get_default_kind( Member* self, void* ctxt )
{

    PyTuplePtr tuple( PyTuple_New( 2 ) );
    if( !tuple )
        return 0;
    tuple.set_item( 0, PyInt_FromLong( self->default_kind ) );
    PyObject* context = self->default_context;
    tuple.set_item( 1, newref( context ? context : _py_null ) );
    return tuple.release();
}


static int
Member_set_default_kind( Member* self, PyObject* value, void* ctxt )
{
    if( !value )
    {
        self->default_kind = NoDefault;
        Py_XDECREF( self->default_context );
        self->default_context = 0;
        return 0;
    }
    if( !PyTuple_Check( value ) )
    {
        py_expected_type_fail( value, "tuple" );
        return -1;
    }
    DefaultKind kind;
    PyObject* context;
    if( !PyArg_ParseTuple( value, "lO", &kind, &context ) )
        return -1;
    if( kind < NoDefault || kind > UserDefault )
    {
        py_value_fail( "invalid default kind" );
        return -1;
    }
    self->default_kind = kind;
    if( kind == NoDefault )
    {
        Py_XDECREF( self->default_context );
        self->default_context = 0;
    }
    else
    {
        Py_INCREF( context );
        Py_XDECREF( self->default_context );
        self->default_context = context;
    }
    return 0;
}


static PyObject*
Member_get_validate_kind( Member* self, void* ctxt )
{

    PyTuplePtr tuple( PyTuple_New( 2 ) );
    if( !tuple )
        return 0;
    tuple.set_item( 0, PyInt_FromLong( self->validate_kind ) );
    PyObject* context = self->validate_context;
    tuple.set_item( 1, newref( context ? context : _py_null ) );
    return tuple.release();
}


static int
Member_set_validate_kind( Member* self, PyObject* value, void* ctxt )
{
    if( !value )
    {
        self->validate_kind = NoValidate;
        Py_XDECREF( self->validate_context );
        self->validate_context = 0;
        return 0;
    }
    if( !PyTuple_Check( value ) )
    {
        py_expected_type_fail( value, "tuple" );
        return -1;
    }
    ValidateKind kind;
    PyObject* context;
    if( !PyArg_ParseTuple( value, "lO", &kind, &context ) )
        return -1;
    if( kind < NoValidate || kind > UserValidate )
    {
        py_value_fail( "invalid validate kind" );
        return -1;
    }
    self->validate_kind = kind;
    if( kind == NoValidate )
    {
        Py_XDECREF( self->validate_context );
        self->validate_context = 0;
    }
    else
    {
        Py_INCREF( context );
        Py_XDECREF( self->validate_context );
        self->validate_context = context;
    }
    return 0;
}


static PyGetSetDef
Member_getset[] = {
    { "name", ( getter )Member_get_name, 0,
      "Get the name to which the member is bound." },
    { "index", ( getter )Member_get_index, 0,
      "Get the index to which the member is bound" },
    { "default_kind", ( getter )Member_get_default_kind, ( setter )Member_set_default_kind,
      "Get and set the default kind for the member." },
    { "validate_kind", ( getter )Member_get_validate_kind, ( setter )Member_set_validate_kind,
      "Get and set the validate kind for the member." },
    { 0 } // sentinel
};


static PyMethodDef
Member_methods[] = {
    { "get_value", ( PyCFunction )Member_get_value, METH_O,
      "Get the value for the atom directly, skipping the default lookup semantics." },
    { "set_value", ( PyCFunction )Member_set_value, METH_VARARGS,
      "Set the value for the atom directly, skipping the default update semantics." },
    { "copy_static_observers", ( PyCFunction )Member_copy_static_observers, METH_O,
      "Copy the static observers from one member into this member." },
    { "static_observers", ( PyCFunction )Member_static_observers, METH_NOARGS,
      "Get a tuple of the static observers defined for this member" },
    { "add_static_observer", ( PyCFunction )Member_add_static_observer, METH_O,
      "Add the name of a method to call on all atoms when the member changes." },
    { "remove_static_observer", ( PyCFunction )Member_remove_static_observer, METH_O,
      "Remove the name of a method to call on all atoms when the member changes." },
    { "do_default", ( PyCFunction )Member_do_default, METH_O,
      "Run the default value handler for member." },
    { "do_validate", ( PyCFunction )Member_do_validate, METH_VARARGS,
      "Run the validation handler for the member." },
    { "default", ( PyCFunction )Member_default, METH_VARARGS,
      "Compute the default value for the member." },
    { "validate", ( PyCFunction )Member_validate, METH_VARARGS,
      "Validate the value for the pending member change." },
    { "clone", ( PyCFunction )Member_clone, METH_NOARGS,
      "Create a clone of this member." },
    { "_set_member_name", ( PyCFunction )Member_set_member_name, METH_O,
      "Set the name to which the member is bound. Use with extreme caution!" },
    { "_set_member_index", ( PyCFunction )Member_set_member_index, METH_O,
      "Set the index to which the member is bound. Use with extreme caution!" },
    { 0 } // sentinel
};


PyTypeObject Member_Type = {
    PyObject_HEAD_INIT( &PyType_Type )
    0,                                      /* ob_size */
    "catom.Member",                         /* tp_name */
    sizeof( Member ),                       /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)Member_dealloc,             /* tp_dealloc */
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
    (traverseproc)Member_traverse,          /* tp_traverse */
    (inquiry)Member_clear,                  /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)Member_methods,    /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    Member_getset,                          /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)Member__get__,            /* tp_descr_get */
    (descrsetfunc)Member__set__,            /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)Member_new,                    /* tp_new */
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
notify_observers( Member* member, CAtom* atom, PyObjectPtr& args, PyObjectPtr& kwargs )
{
    if( !get_atom_notify_bit( atom ) )
        return 0;
    if( member->static_observers )
    {
        PyObjectPtr ownerptr( newref( reinterpret_cast<PyObject*>( atom ) ) );
        StaticModifyGuard guard( member );
        std::vector<PyObjectPtr>::iterator it;
        std::vector<PyObjectPtr>::iterator end = member->static_observers->end();
        for( it = member->static_observers->begin(); it != end; ++it )
        {
            PyObjectPtr method( ownerptr.get_attr( *it ) );
            if( !method )
                return -1;
            if( !method( args, kwargs ) )
                return -1;
        }
    }
    if( atom->observers )
    {
        PyObjectPtr name( newref( member->name ) );
        if( atom->observers->notify( name, args, kwargs ) < 0 )
            return -1;
    }
    return 0;
}


PyObject* _py_null;


PyObject* _undefined;


int import_member()
{
    if( PyType_Ready( &PyNull_Type ) < 0 )
        return -1;
    if( PyType_Ready( &MemberChange_Type ) < 0 )
        return -1;
    if( PyType_Ready( &Member_Type ) < 0 )
        return -1;
    _undefined = PyString_FromString( "<undefined>" );
    if( !_undefined )
        return -1;
    _py_null = PyType_GenericNew( &PyNull_Type, 0, 0 );
    if( !_py_null )
        return -1;
    return 0;
}


}  // extern C

