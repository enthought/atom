/*-----------------------------------------------------------------------------
|  Copyright (c) 2012, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "catom.h"
#include "member.h"
#include "event.h"
#include "signal.h"


extern "C" {


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
    if( import_member() < 0 )
        return;
    if( import_catom() < 0 )
        return;
    if( import_event() < 0 )
        return;
    if( import_signal() < 0 )
        return;
    Py_INCREF( &MemberChange_Type );
    Py_INCREF( &Member_Type );
    Py_INCREF( &CAtom_Type );
    Py_INCREF( &Event_Type );
    Py_INCREF( &Signal_Type );
    Py_INCREF( _py_null );
    PyModule_AddObject( mod, "MemberChange", reinterpret_cast<PyObject*>( &MemberChange_Type ) );
    PyModule_AddObject( mod, "Member", reinterpret_cast<PyObject*>( &Member_Type ) );
    PyModule_AddObject( mod, "Event", reinterpret_cast<PyObject*>( &Event_Type ) );
    PyModule_AddObject( mod, "Signal", reinterpret_cast<PyObject*>( &Signal_Type ) );
    PyModule_AddObject( mod, "CAtom", reinterpret_cast<PyObject*>( &CAtom_Type ) );
    PyModule_AddObject( mod, "null", _py_null );
    PyModule_AddIntConstant( mod, "NO_VALIDATE", NoValidate );
    PyModule_AddIntConstant( mod, "VALIDATE_READ_ONLY", ValidateReadOnly );
    PyModule_AddIntConstant( mod, "VALIDATE_CONSTANT", ValidateConstant );
    PyModule_AddIntConstant( mod, "VALIDATE_BOOL", ValidateBool );
    PyModule_AddIntConstant( mod, "VALIDATE_INT", ValidateInt );
    PyModule_AddIntConstant( mod, "VALIDATE_LONG", ValidateLong );
    PyModule_AddIntConstant( mod, "VALIDATE_FLOAT", ValidateFloat );
    PyModule_AddIntConstant( mod, "VALIDATE_STR", ValidateStr );
    PyModule_AddIntConstant( mod, "VALIDATE_UNICODE", ValidateUnicode );
    PyModule_AddIntConstant( mod, "VALIDATE_UNICODE_PROMOTE", ValidateUnicodePromote );
    PyModule_AddIntConstant( mod, "VALIDATE_TUPLE", ValidateTuple );
    PyModule_AddIntConstant( mod, "VALIDATE_LIST", ValidateList );
    PyModule_AddIntConstant( mod, "VALIDATE_DICT", ValidateDict );
    PyModule_AddIntConstant( mod, "VALIDATE_INSTANCE", ValidateInstance );
    PyModule_AddIntConstant( mod, "VALIDATE_TYPED", ValidateTyped );
    PyModule_AddIntConstant( mod, "VALIDATE_ENUM", ValidateEnum );
    PyModule_AddIntConstant( mod, "VALIDATE_CALLABLE", ValidateCallable );
    PyModule_AddIntConstant( mod, "VALIDATE_OWNER_METHOD", ValidateOwnerMethod );
    PyModule_AddIntConstant( mod, "USER_VALIDATE", UserValidate );
    PyModule_AddIntConstant( mod, "NO_DEFAULT", NoDefault );
    PyModule_AddIntConstant( mod, "DEFAULT_VALUE", DefaultValue );
    PyModule_AddIntConstant( mod, "DEFAULT_LIST", DefaultList );
    PyModule_AddIntConstant( mod, "DEFAULT_DICT", DefaultDict );
    PyModule_AddIntConstant( mod, "DEFAULT_FACTORY", DefaultFactory );
    PyModule_AddIntConstant( mod, "DEFAULT_OWNER_METHOD", DefaultOwnerMethod );
    PyModule_AddIntConstant( mod, "USER_DEFAULT", UserDefault );
}


} // extern C

