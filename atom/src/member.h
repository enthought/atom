/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include <vector>
#include "pythonhelpers.h"


using namespace PythonHelpers;


extern "C" {


enum ValidateKind
{
    NoValidate,
    ValidateReadOnly,
    ValidateConstant,
    ValidateBool,
    ValidateInt,
    ValidateLong,
    ValidateFloat,
    ValidateStr,
    ValidateUnicode,
    ValidateTuple,
    ValidateList,
    ValidateDict,
    ValidateInstance,
    ValidateEnum,
    ValidateOwnerMethod,
    UserValidate
};


enum DefaultKind
{
    NoDefault,
    DefaultValue,
    DefaultFactory,
    DefaultOwnerMethod,
    UserDefault
};


typedef struct {
    PyObject_HEAD
} PyNull;


typedef struct {
    PyObject_HEAD
    PyObject* object;
    PyObject* name;
    PyObject* oldvalue;
    PyObject* newvalue;
} MemberChange;


class StaticModifyGuard;


typedef struct {
    PyObject_HEAD
    uint32_t index;
    PyObject* name;
    DefaultKind default_kind;
    ValidateKind validate_kind;
    PyObject* default_context;
    PyObject* validate_context;
    std::vector<PyObjectPtr>* static_observers; // method names on the atom subclass
    StaticModifyGuard* modify_guard;
} Member;


PyObject*
member_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue  );


PyObject*
member_default( Member* member, PyObject* owner );


int import_member();


extern PyObject* _py_null;


extern PyTypeObject Member_Type;


extern PyTypeObject MemberChange_Type;


inline int
Member_Check( PyObject* object )
{
    return PyObject_TypeCheck( object, &Member_Type );
}


}  // extern C

