#pragma once

#include <list>
#include <mutex>
#include <condition_variable>

namespace com_port_api
{

    template <class T>
    class blocking_list_buffer
    {
    
    private:

        std::list<T> _list;
        std::size_t  _max_size;
        bool         _closed;

        std::mutex _mx;
        std::condition_variable _cv;

    public:

        blocking_list_buffer(std::size_t max_size)
            : _max_size(max_size)
            , _closed(false)
        {
        }

    public:

        bool try_push(std::list<T> & other)
        {
            std::lock_guard<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() < _max_size)
            {
                _list.splice(_list.end(), other);
                _cv.notify_all();
            }
            return true;
        }

        bool try_pop(std::list<T> & other)
        {
            std::lock_guard<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() > 0)
            {
                other.splice(other.end(), _list);
                _cv.notify_all();
            }
            return true;
        }

        bool push(std::list<T> & other)
        {
            std::unique_lock<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() < _max_size)
            {
                _list.splice(_list.end(), other);
                _cv.notify_all();
            }
            else
            {
                _cv.wait(lock, [&] () {
                    return (_closed || (_list.size() < _max_size));
                });
                if (_closed)
                {
                    return false;
                }
                _list.splice(_list.end(), other);
                _cv.notify_all();
            }
            return true;
        }
        
        bool pop(std::list<T> & other)
        {
            std::unique_lock<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() > 0)
            {
                other.splice(other.end(), _list);
                _cv.notify_all();
            }
            else
            {
                _cv.wait(lock, [&] () {
                    return (_closed || (_list.size() > 0));
                });
                if (_closed)
                {
                    return false;
                }
                other.splice(other.end(), _list);
                _cv.notify_all();
            }
            return true;
        }

        template<class _Rep, class _Period>
        bool push(const std::chrono::duration<_Rep, _Period>& _Rel_time, std::list<T> & other)
        {
            std::unique_lock<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() < _max_size)
            {
                _list.splice(_list.end(), other);
                _cv.notify_all();
            }
            else
            {
                bool success = _cv.wait_for(lock, _Rel_time, [&] () {
                    return (_closed || (_list.size() < _max_size));
                });
                if (_closed)
                {
                    return false;
                }
                if (success)
                {
                    _list.splice(_list.end(), other);
                    _cv.notify_all();
                }
            }
            return true;
        }
        
        template<class _Rep, class _Period>
        bool pop(const std::chrono::duration<_Rep, _Period>& _Rel_time, std::list<T> & other)
        {
            std::unique_lock<std::mutex> lock(_mx);
            if (_closed)
            {
                return false;
            }
            if (_list.size() > 0)
            {
                other.splice(other.end(), _list);
                _cv.notify_all();
            }
            else
            {
                bool success = _cv.wait_for(lock, _Rel_time, [&] () {
                    return (_closed || (_list.size() > 0));
                });
                if (_closed)
                {
                    return false;
                }
                if (success)
                {
                    _list.splice(_list.end(), other);
                    _cv.notify_all();
                }
                other.splice(other.end(), _list);
                _cv.notify_all();
            }
            return true;
        }

        void close()
        {
            std::lock_guard<std::mutex> guard(_mx);
            _closed = true;
            _cv.notify_all();
        }

    };

}