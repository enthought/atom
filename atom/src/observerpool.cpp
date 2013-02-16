/*-----------------------------------------------------------------------------
|  Copyright (c) 2013, Enthought, Inc.
|  All rights reserved.
|----------------------------------------------------------------------------*/
#include "observerpool.h"


struct ModifyTask
{
    ModifyTask( ObserverPool& pool, PyObjectPtr& topic, PyObjectPtr& observer ) :
        m_pool( pool ), m_topic( topic ), m_observer( observer ) {}
    virtual ~ModifyTask() {}
    virtual void run() = 0;
    ObserverPool& m_pool;
    PyObjectPtr m_topic;
    PyObjectPtr m_observer;
};


struct AddTask : public ModifyTask
{
    AddTask( ObserverPool& pool, PyObjectPtr& topic, PyObjectPtr& observer ) :
        ModifyTask( pool, topic, observer ) {}
    void run() { m_pool.add( m_topic, m_observer ); }
};


struct RemoveTask : public ModifyTask
{
    RemoveTask( ObserverPool& pool, PyObjectPtr& topic, PyObjectPtr& observer ) :
        ModifyTask( pool, topic, observer ) {}
    void run() { m_pool.remove( m_topic, m_observer ); }
};


class ModifyGuard
{

public:

    ModifyGuard( ObserverPool& pool ) : m_pool( pool )
    {
        if( !m_pool.m_modify_guard )
            m_pool.m_modify_guard = this;
    }

    ~ModifyGuard()
    {
        if( m_pool.m_modify_guard == this )
        {
            m_pool.m_modify_guard = 0;
            std::vector<ModifyTask*>::iterator it;
            std::vector<ModifyTask*>::iterator end = m_tasks.end();
            for( it = m_tasks.begin(); it != end; ++it )
            {
                ( *it )->run();
                delete *it;
            }
        }
    }

    void add_task( ModifyTask* task ) { m_tasks.push_back( task ); }

private:

    ObserverPool& m_pool;
    std::vector<ModifyTask*> m_tasks;

};


bool ObserverPool::has_topic( PyObjectPtr& topic )
{
    std::vector<Topic>::iterator topic_it;
    std::vector<Topic>::iterator topic_end = m_topics.end();
    for( topic_it = m_topics.begin(); topic_it != topic_end; ++topic_it )
    {
        if( topic_it->match( topic ) )
            return true;
    }
    return false;
}


void ObserverPool::add( PyObjectPtr& topic, PyObjectPtr& observer )
{
    if( m_modify_guard )
    {
        ModifyTask* task = new AddTask( *this, topic, observer );
        m_modify_guard->add_task( task );
        return;
    }
    uint32_t obs_offset = 0;
    std::vector<Topic>::iterator topic_it;
    std::vector<Topic>::iterator topic_end = m_topics.end();
    for( topic_it = m_topics.begin(); topic_it != topic_end; ++topic_it )
    {
        if( topic_it->match( topic ) )
        {
            std::vector<PyObjectPtr>::iterator obs_it;
            std::vector<PyObjectPtr>::iterator obs_end;
            obs_it = m_observers.begin() + obs_offset;
            obs_end = obs_it + topic_it->m_count;
            for( ; obs_it != obs_end; ++obs_it )
            {
                if( *obs_it == observer || obs_it->richcompare( observer, Py_EQ ) )
                    return;
            }
            m_observers.insert( obs_end, observer );
            ++topic_it->m_count;
            return;
        }
        obs_offset += topic_it->m_count;
    }
    m_topics.push_back( Topic( topic, 1 ) );
    m_observers.push_back( observer );
}


void ObserverPool::remove( PyObjectPtr& topic, PyObjectPtr& observer )
{
    if( m_modify_guard )
    {
        ModifyTask* task = new RemoveTask( *this, topic, observer );
        m_modify_guard->add_task( task );
        return;
    }
    uint32_t obs_offset = 0;
    std::vector<Topic>::iterator topic_it;
    std::vector<Topic>::iterator topic_end = m_topics.end();
    for( topic_it = m_topics.begin(); topic_it != topic_end; ++topic_it )
    {
        if( topic_it->match( topic ) )
        {
            std::vector<PyObjectPtr>::iterator obs_it;
            std::vector<PyObjectPtr>::iterator obs_end;
            obs_it = m_observers.begin() + obs_offset;
            obs_end = obs_it + topic_it->m_count;
            for( ; obs_it != obs_end; ++obs_it )
            {
                if( *obs_it == observer || obs_it->richcompare( observer, Py_EQ ) )
                {
                    m_observers.erase( obs_it );
                    if( ( --topic_it->m_count ) == 0 )
                        m_topics.erase( topic_it );
                    return;
                }
            }
            return;
        }
        obs_offset += topic_it->m_count;
    }
}


int ObserverPool::notify( PyObjectPtr& topic, PyObjectPtr& args, PyObjectPtr& kwargs )
{
    ModifyGuard guard( *this );
    uint32_t obs_offset = 0;
    std::vector<Topic>::iterator topic_it;
    std::vector<Topic>::iterator topic_end = m_topics.end();
    for( topic_it = m_topics.begin(); topic_it != topic_end; ++topic_it )
    {
        if( topic_it->match( topic ) )
        {
            std::vector<PyObjectPtr>::iterator obs_it;
            std::vector<PyObjectPtr>::iterator obs_end;
            obs_it = m_observers.begin() + obs_offset;
            obs_end = obs_it + topic_it->m_count;
            for( ; obs_it != obs_end; ++obs_it )
            {
                if( obs_it->is_true() )
                {
                    if( !obs_it->operator()( args, kwargs ) )
                        return -1;
                }
                else
                {
                    ModifyTask* task = new RemoveTask( *this, topic, *obs_it );
                    m_modify_guard->add_task( task );
                }
            }
            return 0;
        }
        obs_offset += topic_it->m_count;
    }
    return 0;
}


int ObserverPool::py_traverse( visitproc visit, void* arg )
{
    int vret;
    std::vector<Topic>::iterator topic_it;
    std::vector<Topic>::iterator topic_end = m_topics.end();
    for( topic_it = m_topics.begin(); topic_it != topic_end; ++topic_it )
    {
        vret = visit( topic_it->m_topic.get(), arg );
        if( vret )
            return vret;
    }
    std::vector<PyObjectPtr>::iterator obs_it;
    std::vector<PyObjectPtr>::iterator obs_end = m_observers.end();
    for( obs_it = m_observers.begin(); obs_it != obs_end; ++obs_it )
    {
        vret = visit( obs_it->get(), arg );
        if( vret )
            return vret;
    }
    return 0;
}

