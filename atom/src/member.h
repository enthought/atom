/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include <vector>
#include "pythonhelpers.h"


using namespace PythonHelpers;


extern "C" {


enum MemberValidator
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
    UserValidate
};


enum MemberDefault
{
    NoDefault,
    DefaultValue,
    DefaultFactory,
    DefaultFactoryMethod,
    UserDefault
};


struct _Member_struct;


typedef PyObject*
(*validate_func)( _Member_struct* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue );


typedef PyObject*
(*default_func)( _Member_struct* member, PyObject* owner );


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


typedef struct _Member_struct {
    PyObject_HEAD
    uint32_t index;
    PyObject* name;
    MemberValidator f_validate_kind;
    MemberDefault f_default_kind;
    validate_func f_validate;
    default_func f_default;
    PyObject* f_validate_ctxt;
    PyObject* f_default_ctxt;
    std::vector<PyObjectPtr>* observers; // method names on the atom subclass
} Member;


int
Member_Check( PyObject* member );


int import_member();


extern PyObject* _py_null;


extern PyTypeObject Member_Type;


extern PyTypeObject MemberChange_Type;


}  // extern C

