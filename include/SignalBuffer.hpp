

#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include "usart.hpp"

namespace sb
{

    /**
     * A circular buffer for storing uint16_vals
     */
    class SignalBuffer
    {

    private:
        uint16_t _buffer[500] = {0};
        uint16_t _write_head = 0;
        int _size;

        int _read_head = 0;

    public:
        SignalBuffer(int size)
        {
            _size = size;
            memset(_buffer, 0, sizeof(_buffer));
        }

        ~SignalBuffer()
        {
            free(_buffer);
        }

        bool push(uint16_t val)
        {

            if (((_write_head + 1) % _size) == _read_head)
            {
                send("[ERR] Write head hit read head\n");
                // the write head has caught up with the read head
                // this should never really happen
                return false;
            }

            _buffer[_write_head] = val;
            _write_head = (_write_head + 1) % _size;
            return true;
        }

        /**
         * Returns false if not enough values exist to fill entire window
         */
        bool getWindow(uint16_t window[], int window_length)
        {

            for (int i = 0; i < window_length; i++)
            {
                if (_buffer[(_read_head + i) % _size] == 0 ||
                    ((_read_head + i) % _size == _write_head) // hit the write head (this means we are "done")
                )
                {
                    return false;
                }
                window[i] = _buffer[(_read_head + i) % _size];
            }

            return true;
        }

        void shift_read_head(int amount = 1)
        {
            for (int i = 0; i < amount; i++)
            {
                _buffer[(_read_head + i) % _size] = 0;
            }
            _read_head = (_read_head + amount) % _size;
        }

        void print()
        {

            send("[");

            for (int i = 0; i < _size; i++)
            {
                if (_buffer[i] == 0)
                {
                    break;
                }

                if (i == _write_head)
                {
                    send('^');
                }

                send(_buffer[i]);
                if (i < (_size - 1))
                {
                    send(", ");
                }
            }

            send("]");
        }
    };
}