/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "member.h"


typedef PyObject*
(*validate_func)( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue );


typedef PyObject*
(*default_func)( Member* member, PyObject* owner );


static PyObject*
validate_type_fail( Member* member, PyObject* owner, PyObject* newvalue, const char* type )
{
    PyErr_Format(
        PyExc_TypeError,
        "The '%s' member on the '%s' object must be of type '%s'. "
        "Got object of type '%s' instead.",
        PyString_AsString( member->name ),
        owner->ob_type->tp_name,
        type,
        newvalue->ob_type->tp_name
    );
    return 0;
}


static PyObject*
no_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    return newref( newvalue );
}


static PyObject*
validate_read_only( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( oldvalue != _py_null )
        return py_type_fail( "cannot change the value of a read-only member" );
    return newref( newvalue );
}


static PyObject*
validate_constant( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    return py_type_fail( "cannot change the value of a constant member" );
}


static PyObject*
validate_bool( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( newvalue == Py_True || newvalue == Py_False )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "bool" );
}


static PyObject*
validate_int( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyInt_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "int" );
}


static PyObject*
validate_long( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyLong_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "long" );
}


static PyObject*
validate_long_promote( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyLong_Check( newvalue ) )
        return newref( newvalue );
    if( PyInt_Check( newvalue ) )
        return PyLong_FromLong( PyInt_AS_LONG( newvalue ) );
    return validate_type_fail( member, owner, newvalue, "long" );
}


static PyObject*
validate_float( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyFloat_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "float" );
}


static PyObject*
validate_float_promote( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyFloat_Check( newvalue ) )
        return newref( newvalue );
    if( PyInt_Check( newvalue ) )
        return PyFloat_FromDouble( static_cast<double>( PyInt_AS_LONG( newvalue ) ) );
    if( PyLong_Check( newvalue ) )
    {
        double val = PyLong_AsDouble( newvalue );
        if( val < 0.0 && PyErr_Occurred() )
            return 0;
        return PyFloat_FromDouble( val );
    }
    return validate_type_fail( member, owner, newvalue, "float" );
}


static PyObject*
validate_string( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyString_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "string" );
}


static PyObject*
validate_unicode( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyUnicode_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "unicode" );
}


static PyObject*
validate_unicode_promote( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyUnicode_Check( newvalue ) )
        return newref( newvalue );
    if( PyString_Check( newvalue ) )
        return PyUnicode_FromObject( newvalue );
    return validate_type_fail( member, owner, newvalue, "unicode" );
}


static PyObject*
validate_tuple( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyTuple_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "tuple" );
}


static PyObject*
validate_list( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( !PyList_Check( newvalue ) )
        return validate_type_fail( member, owner, newvalue, "list" );
    Py_ssize_t size = PyList_GET_SIZE( newvalue );
    PyObjectPtr listcopy( PyList_GetSlice( newvalue, 0, size ) );
    if( !listcopy )
        return 0;
    if( member->validate_context != Py_None )
    {
        if( !Member_Check( member->validate_context ) )
            return py_bad_internal_call( "validate_list() context is not a Member or None" );
        Member* item_member = reinterpret_cast<Member*>( member->validate_context );
        for( Py_ssize_t i = 0; i < size; ++i )
        {
            PyObject* item = PyList_GET_ITEM( listcopy.get(), i );
            PyObject* valid_item = member_validate( item_member, owner, _py_null, item );
            if( !valid_item )
                return 0;
            if( valid_item != item )
                PyList_SetItem( listcopy.get(), i, valid_item ); // this steals the owned ref gained by member_validate
        }
    }
    return listcopy.release();
}


static PyObject*
validate_dict_key_value( Member* keymember, Member* valmember, PyObject* owner, PyObject* dict )
{
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    PyDictPtr newptr( PyDict_New() );
    if( !newptr )
        return 0;
    while( PyDict_Next( dict, &pos, &key, &value ) )
    {
        PyObjectPtr keyptr( member_validate( keymember, owner, _py_null, key ) );
        if( !keyptr )
            return 0;
        PyObjectPtr valptr( member_validate( valmember, owner, _py_null, value ) );
        if( !valptr )
            return 0;
        if( !newptr.set_item( keyptr, valptr ) )
            return 0;
    }
    return newptr.release();
}


static PyObject*
validate_dict_value( Member* valmember, PyObject* owner, PyObject* dict )
{
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    PyDictPtr newptr( PyDict_New() );
    if( !newptr )
        return 0;
    while( PyDict_Next( dict, &pos, &key, &value ) )
    {
        PyObjectPtr keyptr( newref( key ) );
        PyObjectPtr valptr( member_validate( valmember, owner, _py_null, value ) );
        if( !valptr )
            return 0;
        if( !newptr.set_item( keyptr, valptr ) )
            return 0;
    }
    return newptr.release();
}


static PyObject*
validate_dict_key( Member* keymember, PyObject* owner, PyObject* dict )
{
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    PyDictPtr newptr( PyDict_New() );
    if( !newptr )
        return 0;
    while( PyDict_Next( dict, &pos, &key, &value ) )
    {
        PyObjectPtr keyptr( member_validate( keymember, owner, _py_null, key ) );
        if( !keyptr )
            return 0;
        PyObjectPtr valptr( newref( value ) );
        if( !newptr.set_item( keyptr, valptr ) )
            return 0;
    }
    return newptr.release();
}


static PyObject*
validate_dict( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( !PyDict_Check( newvalue ) )
        return validate_type_fail( member, owner, newvalue, "dict" );
    if( !PyTuple_Check( member->validate_context ) )
        return py_bad_internal_call( "validate_dict() context is not a 2-tuple" );
    if( PyTuple_GET_SIZE( member->validate_context ) != 2 )
        return py_bad_internal_call( "validate_dict() context is not a 2-tuple" );
    PyObject* keymember = PyTuple_GET_ITEM( member->validate_context, 0 );
    PyObject* valmember = PyTuple_GET_ITEM( member->validate_context, 1 );
    if( keymember != Py_None && !Member_Check( keymember ) )
        return py_bad_internal_call( "validate_dict() context key is not a Member or None" );
    if( valmember != Py_None && !Member_Check( valmember ) )
        return py_bad_internal_call( "validate_dict() context value is not a Member or None" );
    if( keymember != Py_None && valmember != Py_None )
    {
        Member* k = reinterpret_cast<Member*>( keymember );
        Member* v = reinterpret_cast<Member*>( valmember );
        return validate_dict_key_value( k, v, owner, newvalue );
    }
    if( valmember != Py_None )
    {
        Member* v = reinterpret_cast<Member*>( valmember );
        return validate_dict_value( v, owner, newvalue );
    }
    if( keymember != Py_None )
    {
        Member* k = reinterpret_cast<Member*>( keymember );
        return validate_dict_key( k, owner, newvalue );
    }
    return PyDict_Copy( newvalue );
}


static PyObject*
validate_instance( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( newvalue == Py_None )
        return newref( newvalue );
    int res = PyObject_IsInstance( newvalue, member->validate_context );
    if( res < 0 )
        return 0;
    if( res == 1 )
        return newref( newvalue );
    return py_type_fail( "invalid instance type" );
}


static PyObject*
validate_typed( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( newvalue == _py_null )
        return newref( newvalue );
    if( !PyType_Check( member->validate_context ) )
        return py_type_fail( "validator context is not a type" );
    PyTypeObject* type = reinterpret_cast<PyTypeObject*>( member->validate_context );
    if( PyObject_TypeCheck( newvalue, type ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, type->tp_name );
}


static PyObject*
validate_enum( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    int res = PySequence_Contains( member->validate_context, newvalue );
    if( res < 0 )
        return 0;
    if( res == 1 )
        return newref( newvalue );
    return py_value_fail( "invalid enum value" );
}


static PyObject*
validate_callable( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyCallable_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "callable" );
}


static PyObject*
validate_range( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( !PyInt_Check( newvalue ) )
        return validate_type_fail( member, owner, newvalue, "int" );
    if( !PyTuple_Check( member->validate_context ) )
        return py_bad_internal_call( "validate_range() context is not a 2-tuple" );
    if( PyTuple_GET_SIZE( member->validate_context ) != 2 )
        return py_bad_internal_call( "validate_range() context is not a 2-tuple" );
    PyObject* low = PyTuple_GET_ITEM( member->validate_context, 0 );
    PyObject* high = PyTuple_GET_ITEM( member->validate_context, 1 );
    long value = PyInt_AS_LONG( newvalue );
    if( low != Py_None )
    {
        if( !PyInt_Check( low ) )
            return py_bad_internal_call( "validate_range() low is not an int" );
        if( PyInt_AS_LONG( low ) > value )
            return py_type_fail( "range value too small" );
    }
    if( high != Py_None )
    {
        if( !PyInt_Check( high ) )
            return py_bad_internal_call( "validate_range() high is not an int" );
        if( PyInt_AS_LONG( high ) < value )
            return py_type_fail( "range value too large" );
    }
    return newref( newvalue );
}


static PyObject*
validate_owner_method( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( !PyString_Check( member->validate_context ) )
        return py_type_fail( "validate owner method name must be a string" );
    PyObjectPtr callable( PyObject_GetAttr( owner, member->validate_context ) );
    if( !callable )
        return 0;
    PyTuplePtr args( PyTuple_New( 2 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( oldvalue ) );
    args.set_item( 1, newref( newvalue ) );
    return callable( args ).release();
}


static PyObject*
user_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    static PyObject* vstr = 0;
    if( !vstr )
        vstr = PyString_FromString( "validate" );
    PyObjectPtr validate( PyObject_GetAttr( reinterpret_cast<PyObject*>( member ), vstr ) );
    if( !validate )
        return 0;
    PyTuplePtr args( PyTuple_New( 4 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( owner ) );
    args.set_item( 1, newref( member->name ) );
    args.set_item( 2, newref( oldvalue ) );
    args.set_item( 3, newref( newvalue ) );
    return validate( args ).release();
}


static validate_func
validators[] = {
    no_validate,
    validate_read_only,
    validate_constant,
    validate_bool,
    validate_int,
    validate_long,
    validate_long_promote,
    validate_float,
    validate_float_promote,
    validate_string,
    validate_unicode,
    validate_unicode_promote,
    validate_tuple,
    validate_list,
    validate_dict,
    validate_instance,
    validate_typed,
    validate_enum,
    validate_callable,
    validate_range,
    validate_owner_method,
    user_validate
};


PyObject*
member_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( member->validate_kind >= sizeof( validators ) )
        return no_validate( member, owner, oldvalue, newvalue );
    return validators[ member->validate_kind ]( member, owner, oldvalue, newvalue );
}


static PyObject*
post_validate_owner_method( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( !PyString_Check( member->post_validate_context ) )
        return py_type_fail( "post validate owner method name must be a string" );
    PyObjectPtr callable( PyObject_GetAttr( owner, member->post_validate_context ) );
    if( !callable )
        return 0;
    PyTuplePtr args( PyTuple_New( 2 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( oldvalue ) );
    args.set_item( 1, newref( newvalue ) );
    return callable( args ).release();
}


static PyObject*
user_post_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    static PyObject* vstr = 0;
    if( !vstr )
        vstr = PyString_FromString( "post_validate" );
    PyObjectPtr validate( PyObject_GetAttr( reinterpret_cast<PyObject*>( member ), vstr ) );
    if( !validate )
        return 0;
    PyTuplePtr args( PyTuple_New( 4 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( owner ) );
    args.set_item( 1, newref( member->name ) );
    args.set_item( 2, newref( oldvalue ) );
    args.set_item( 3, newref( newvalue ) );
    return validate( args ).release();
}


static validate_func
post_validators[] = {
    no_validate,
    post_validate_owner_method,
    user_post_validate
};


PyObject*
member_post_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( member->post_validate_kind >= sizeof( post_validators ) )
        return no_validate( member, owner, oldvalue, newvalue );
    return post_validators[ member->post_validate_kind ]( member, owner, oldvalue, newvalue );
}


static PyObject*
no_default( Member* member, PyObject* owner )
{
    return newref( _py_null );
}


static PyObject*
default_value( Member* member, PyObject* owner )
{
    return newref( member->default_context );
}


static PyObject*
default_list( Member* member, PyObject* owner )
{
    if( member->default_context == Py_None )
        return PyList_New( 0 );
    if( !PyList_Check( member->default_context ) )
        return py_type_fail( "expect a list as default context" );
    Py_ssize_t size = PyList_GET_SIZE( member->default_context );
    return PyList_GetSlice( member->default_context, 0, size );
}


static PyObject*
default_dict( Member* member, PyObject* owner )
{
    if( member->default_context == Py_None )
        return PyDict_New();
    if( !PyDict_Check( member->default_context ) )
        return py_type_fail( "expect a dict as default context" );
    return PyDict_Copy( member->default_context );
}


static PyObject*
default_factory( Member* member, PyObject* owner )
{
    PyTuplePtr args( PyTuple_New( 0 ) );
    if( !args )
        return 0;
    PyObjectPtr callable( newref( member->default_context ) );
    return callable( args ).release();
}


static PyObject*
default_owner_method( Member* member, PyObject* owner )
{
    if( !PyString_Check( member->default_context ) )
        return py_type_fail( "default owner method name must be a string" );
    PyObjectPtr callable( PyObject_GetAttr( owner, member->default_context ) );
    if( !callable )
        return 0;
    PyTuplePtr args( PyTuple_New( 0 ) );
    if( !args )
        return 0;
    return callable( args ).release();
}


static PyObject*
user_default( Member* member, PyObject* owner )
{
    static PyObject* dstr = 0;
    if( !dstr )
        dstr = PyString_FromString( "default" );
    PyObject* pymember = reinterpret_cast<PyObject*>( member );
    PyObjectPtr defaultptr( PyObject_GetAttr( pymember, dstr ) );
    if( !defaultptr )
        return 0;
    PyTuplePtr args( PyTuple_New( 2 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( owner ) );
    args.set_item( 1, newref( member->name ) );
    return defaultptr( args ).release();
}


static default_func
defaults[] = {
    no_default,
    default_value,
    default_list,
    default_dict,
    default_factory,
    default_owner_method,
    user_default
};


PyObject*
member_default( Member* member, PyObject* owner )
{
    if( member->default_kind >= sizeof( defaults ) )
        return no_default( member, owner );
    return defaults[ member->default_kind ]( member, owner );
}

