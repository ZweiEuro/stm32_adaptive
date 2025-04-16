#pragma once
#include <malloc.h>
#include "usart.hpp"

template <typename T>
class Ringbuffer
{

private:
    int _size;
    T *_buffer;

    int _read_index = 0;  // next position to be read from
    int _write_index = 0; // next position to be written to

private:
    void inc_ri()
    {
        _read_index = (_read_index + 1) % _size;
    }

    void inc_wi()
    {
        if (_write_index == _read_index)
        {
            // if we just wrote to something the reader thands on, we need to push it forward by one
            inc_ri();
        }
        _write_index = (_write_index + 1) % _size;
    }

public:
    Ringbuffer(const int size)
    {
        this->_size = size;
        this->_buffer = (T *)calloc(size, sizeof(T));
    }

    int size() { return this->_size; }

    void push(const T new_val)
    {
        _buffer[_write_index] = new_val;
        inc_wi();
    }

    T peekNext()
    {
        return _buffer[_read_index];
    }

    T getNext()
    {
        int i = _read_index;
        inc_ri();
        return _buffer[i];
    }

    void print_all()
    {
        for (int i = 0; i < _size; i++)
        {
            send(_buffer[i]);
        }
        send('\n');

        for (int i = 0; i < _size; i++)
        {
            if (i == _write_index)
            {
                send('w');
            }
            else
            {
                send(' ');
            }
        }
        send('\n');

        for (int i = 0; i < _size; i++)
        {
            if (i == _read_index)
            {
                send('r');
            }
            else
            {
                send(' ');
            }
        }

        send('\n');
    }
};