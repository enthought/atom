/*-----------------------------------------------------------------------------
|  Copyright (c) 2012, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "callbackhandler.h"


struct ModifyTask
{
    ModifyTask( CallbackHandler& handler, PyObjectPtr& callback ) :
        m_handler( handler ), m_callback( callback ) {}
    virtual ~ModifyTask() {}
    virtual void run() = 0;
    CallbackHandler& m_handler;
    PyObjectPtr m_callback;
};


struct AddCallbackTask : public ModifyTask
{
    AddCallbackTask( CallbackHandler& handler, PyObjectPtr& callback ) :
        ModifyTask( handler, callback ) {}
    void run() { m_handler.add_callback( m_callback ); }
};


struct RemoveCallbackTask : public ModifyTask
{
    RemoveCallbackTask( CallbackHandler& handler, PyObjectPtr& callback ) :
        ModifyTask( handler, callback ) {}
    void run() { m_handler.remove_callback( m_callback ); }
};


class ModifyGuard
{

public:

    ModifyGuard( CallbackHandler& handler ) : m_handler( handler )
    {
        if( !m_handler.m_modify_guard )
            m_handler.m_modify_guard = this;
    }

    ~ModifyGuard()
    {
        if( m_handler.m_modify_guard == this )
        {
            m_handler.m_modify_guard = 0;
            std::vector<ModifyTask*>::iterator it;
            std::vector<ModifyTask*>::iterator end = m_tasks.end();
            for( it = m_tasks.begin(); it != end; ++it )
            {
                ( *it )->run();
                delete *it;
            }
        }
    }

    void add_task( ModifyTask* task )
    {
        if( task )
            m_tasks.push_back( task );
    }

private:

    CallbackHandler& m_handler;
    std::vector<ModifyTask*> m_tasks;

};


void CallbackHandler::add_callback( PyObjectPtr& callback )
{
    if( !callback )
        return;
    if( m_modify_guard )
    {
        ModifyTask* task = new AddCallbackTask( *this, callback );
        m_modify_guard->add_task( task );
        return;
    }
    std::vector<PyObjectPtr>::iterator it;
    std::vector<PyObjectPtr>::iterator end = m_callbacks.end();
    for( it = m_callbacks.begin(); it != end; ++it )
    {
        if( *it == callback || it->richcompare( callback, Py_EQ ) )
            return;
    }
    m_callbacks.push_back( callback );
}


void CallbackHandler::remove_callback( PyObjectPtr& callback )
{
    if( !callback )
        return;
    if( m_modify_guard )
    {
        ModifyTask* task = new RemoveCallbackTask( *this, callback );
        m_modify_guard->add_task( task );
        return;
    }
    std::vector<PyObjectPtr>::iterator it;
    std::vector<PyObjectPtr>::iterator end = m_callbacks.end();
    for( it = m_callbacks.begin(); it != end; ++it )
    {
        if( *it == callback || it->richcompare( callback, Py_EQ ) )
        {
            m_callbacks.erase( it );
            return;
        }
    }
}


int CallbackHandler::invoke_callbacks( PyObjectPtr& args, PyObjectPtr& kwargs )
{
    if( !args )
    {
        PyErr_BadInternalCall();
        return -1;
    }
    ModifyGuard guard( *this );
    std::vector<PyObjectPtr>::iterator it;
    std::vector<PyObjectPtr>::iterator end = m_callbacks.end();
    for( it = m_callbacks.begin(); it != end; ++it )
    {
        if( it->is_true() )
        {
            if( !it->operator()( args, kwargs ) )
                return -1;
        }
        else
        {
            ModifyTask* task = new RemoveCallbackTask( *this, *it );
            m_modify_guard->add_task( task );
        }
    }
    return 0;
}


int CallbackHandler::py_traverse( visitproc visit, void* arg )
{
    int vret;
    std::vector<PyObjectPtr>::iterator it;
    std::vector<PyObjectPtr>::iterator end = m_callbacks.end();
    for( it = m_callbacks.begin(); it != end; ++it )
    {
        vret = visit( it->get(), arg );
        if( vret )
            return vret;
    }
    return 0;
}

