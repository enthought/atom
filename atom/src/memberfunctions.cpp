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
validate_float( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyFloat_Check( newvalue ) )
        return newref( newvalue );
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
validate_tuple( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyTuple_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "tuple" );
}


static PyObject*
validate_list( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyList_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "list" );
}


static PyObject*
validate_dict( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue )
{
    if( PyLong_Check( newvalue ) )
        return newref( newvalue );
    return validate_type_fail( member, owner, newvalue, "dict" );
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
    validate_float,
    validate_string,
    validate_unicode,
    validate_tuple,
    validate_list,
    validate_dict,
    no_validate,   // validate instance
    validate_enum,
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

