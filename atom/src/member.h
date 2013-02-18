/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include <vector>
#include "pythonhelpers.h"
#include "catom.h"


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
    ValidateUnicodePromote,
    ValidateTuple,
    ValidateList,
    ValidateDict,
    ValidateInstance,
    ValidateTyped,
    ValidateEnum,
    ValidateCallable,
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


class StaticModifyGuard
{

public:

    StaticModifyGuard( Member* member ) : m_member( member )
    {
        if( m_member && !m_member->modify_guard )
            m_member->modify_guard = this;
    }

    ~StaticModifyGuard()
    {
        if( m_member && m_member->modify_guard == this )
            m_member->modify_guard = 0;
    }

private:

    Member* m_member;

};


PyObject*
MemberChange_New( PyObject* object, PyObject* name, PyObject* oldval, PyObject* newval );


PyObject*
member_validate( Member* member, PyObject* owner, PyObject* oldvalue, PyObject* newvalue  );


PyObject*
member_default( Member* member, PyObject* owner );


int
notify_observers( Member* member, CAtom* atom, PyObjectPtr& args, PyObjectPtr& kwargs );


int import_member();


extern PyObject* _py_null;


extern PyObject* _undefined;


extern PyTypeObject Member_Type;


extern PyTypeObject MemberChange_Type;


inline int
Member_Check( PyObject* object )
{
    return PyObject_TypeCheck( object, &Member_Type );
}


}  // extern C

