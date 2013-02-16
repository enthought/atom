/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#pragma once
#include <vector>
#include "pythonhelpers.h"


using namespace PythonHelpers;


class ModifyGuard;


class ObserverPool
{

    struct Topic
    {
        Topic( PyObjectPtr& topic ) : m_topic( topic ), m_count( 0 ) {}
        Topic( PyObjectPtr& topic, uint32_t count ) : m_topic( topic ), m_count( count ) {}
        ~Topic() {}
        bool match( PyObjectPtr& topic )
        {
            return m_topic == topic || m_topic.richcompare( topic, Py_EQ );
        }
        PyObjectPtr m_topic;
        uint32_t m_count;
    };

    friend class ModifyGuard;

public:

    ObserverPool() : m_modify_guard( 0 ) {}

    ~ObserverPool() {}

    bool has_topic( PyObjectPtr& topic );

    void add( PyObjectPtr& topic, PyObjectPtr& observer );

    void remove( PyObjectPtr& topic, PyObjectPtr& observer );

    int notify( PyObjectPtr& topic, PyObjectPtr& args, PyObjectPtr& kwargs );

    Py_ssize_t py_sizeof() { return 0; };

    int py_traverse( visitproc visit, void* arg );

    void py_clear() { m_topics.clear(); m_observers.clear(); }

private:

    ModifyGuard* m_modify_guard;
    std::vector<Topic> m_topics;
    std::vector<PyObjectPtr> m_observers;
    ObserverPool(const ObserverPool& other);
    ObserverPool& operator=(const ObserverPool&);

};

