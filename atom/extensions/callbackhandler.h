/*-----------------------------------------------------------------------------
|  Copyright (c) 2012, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include <vector>
#include "pythonhelpers.h"


using namespace PythonHelpers;


class ModifyGuard;


class CallbackHandler
{

    friend class ModifyGuard;

public:

    CallbackHandler() : m_modify_guard( 0 ) {}

    CallbackHandler(const CallbackHandler& other) :
        m_modify_guard( 0 ), m_callbacks( other.m_callbacks ) {}

    ~CallbackHandler() {}

    void add_callback( PyObjectPtr& callback );

    void remove_callback( PyObjectPtr& callback );

    int invoke_callbacks( PyObjectPtr& args, PyObjectPtr& kwargs );

    int py_traverse( visitproc visit, void* arg );

    void py_clear() { m_callbacks.clear(); }

    CallbackHandler& operator=(const CallbackHandler& other)
    {
        m_modify_guard = 0;
        m_callbacks = other.m_callbacks;
        return *this;
    }

private:

    ModifyGuard* m_modify_guard;
    std::vector<PyObjectPtr> m_callbacks;

};

