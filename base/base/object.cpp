#include "object.h"
#include "autoreleasepool.h"
#include <iostream>
#include <typeinfo>
#include "logger.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>
#include <lua/lua.hpp>

namespace base
{
    using namespace std;


    ObjectTracker* gObjectTracker = nullptr;

    ObjectTracker* ObjectTracker::Create()
    {
        if (gObjectTracker == nullptr) {
            gObjectTracker = new ObjectTracker();
        }
        return gObjectTracker;
    }

    void ObjectTracker::Destroy()
    {
        SAFE_DELETE(gObjectTracker);
    }

    void ObjectTracker::AddObject(Object* ptr)
    {
        m_objects.insert(ptr);
    }

    void ObjectTracker::RemoveObject(Object* ptr)
    {
        m_objects.erase(ptr);
    }

    string ObjectTracker::SqlCountsToJson()
    {
        m_objectCounts.clear();
        for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
            std::string name = (*it)->GetObjectName();
            auto itName = m_objectCounts.find(name);
            if (itName != m_objectCounts.end()) {
                ++itName->second;
            } else {
                m_objectCounts.emplace(name, 1);
            }
        }

        string jsonString;
        try {
            rapidjson::StringBuffer jsonbuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
            writer.StartArray();
            for (auto it = m_objectCounts.begin(); it != m_objectCounts.end(); ++it) {
                writer.StartObject();
                writer.String("name");
                writer.String(it->first.c_str());
                writer.String("count");
                writer.Int64(it->second);
                writer.EndObject();
            }
            writer.EndArray();
            jsonString = jsonbuffer.GetString();
        } catch (std::exception& ex) {
            LOG_ERROR("ObjectTracker::SqlCountsToJson fail: %s\n", ex.what());
        }
        return jsonString;
    }


    uint32_t g_object_count = 0;

    Object::Object()
        : reference_count_(1)
    {
        ++g_object_count;

        //gObjectTracker->AddObject(this);
    }

    Object::~Object()
    {
        --g_object_count;

        //gObjectTracker->RemoveObject(this);
    }

    void Object::Retain()
    {
        ++reference_count_;
    }

    void Object::Release()
    {
        --reference_count_;
        if (reference_count_ == 0) {
            delete this;
        }
    }

    void Object::AutoRelease()
    {
        PoolManager::instance()->AddObject(this);
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
