/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "memberfunctions.h"


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
    int res = PySequence_Contains( member->f_validate_ctxt, newvalue );
    if( res < 0 )
        return 0;
    if( res == 1 )
        return newref( newvalue );
    return py_value_fail( "invalid enum value" );
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


static validate_func validators[] = {
    0,                    // no validate
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
    0,
    validate_enum,
    user_validate
};


validate_func
get_validate_func( MemberValidator type )
{
    if( type >= sizeof( validators ) )
        return 0;
    return validators[ type ];
}


static PyObject*
default_value( Member* member, PyObject* owner )
{
    return newref( member->f_default_ctxt );
}


static PyObject*
default_factory( Member* member, PyObject* owner )
{
    PyTuplePtr args( PyTuple_New( 0 ) );
    if( !args )
        return 0;
    PyObjectPtr callable( newref( member->f_default_ctxt ) );
    return callable( args ).release();
}


static PyObject*
default_factory_method( Member* member, PyObject* owner )
{
    if( !PyString_Check( member->f_default_ctxt ) )
        return py_type_fail( "default factory method name must be a string" );
    PyObjectPtr callable( PyObject_GetAttr( owner, member->f_default_ctxt ) );
    if( !callable )
        return 0;
    PyObjectPtr args( PyTuple_New( 0 ) );
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
    PyObjectPtr defaultptr( PyObject_GetAttr( reinterpret_cast<PyObject*>( member ), dstr ) );
    if( !defaultptr )
        return 0;
    PyTuplePtr args( PyTuple_New( 2 ) );
    if( !args )
        return 0;
    args.set_item( 0, newref( owner ) );
    args.set_item( 1, newref( member->name ) );
    return defaultptr( args ).release();
}


static default_func defaultors[] = {
    0,                    // no default
    default_value,
    default_factory,
    default_factory_method,
    user_default
};


default_func
get_default_func( MemberDefault type )
{
    if( type >= sizeof( defaultors ) )
        return 0;
    return defaultors[ type ];
}

