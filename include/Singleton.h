#pragma once

template <class T>
struct Singleton
{
    static T& instance() {
        return *instance_ptr();
    }
    static T* instance_ptr() {
        if(!instance_)
            instance_ = new T;
        return instance_;
    }

    static T *instance_; // no inline pre-C++11
};