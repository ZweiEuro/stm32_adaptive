

#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include "usart.hpp"
#include <deque>

namespace sb
{

    /**
     * A circular buffer for storing uint16_vals
     */
    class SignalBuffer
    {

    private:
        std::deque<uint16_t> _queue;

    public:
        SignalBuffer()
        {
        }

        ~SignalBuffer()
        {
        }

        void push(uint16_t val)
        {
            if (_queue.size() >= 500)
            {
                return;
            }

            _queue.push_back(val);
        }

        /**
         * Returns false if not enough values exist to fill entire window
         */
        bool getWindow(uint16_t window[], int window_length)
        {
            if (_queue.size() < window_length)
            {
                return false;
            }

            for (int i = 0; i < window_length; i++)

            {
                window[i] = _queue.at(i);
            }

            return true;
        }

        void shift_read_head(int amount = 1)
        {
            for (int i = 0; i < amount; i++)
            {
                _queue.pop_front();
            }
        }

        void print()
        {

            send("[");

            for (auto v : _queue)
            {

                send(v);
                send(", ");
            }

            send("]\n");
        }
    };
}