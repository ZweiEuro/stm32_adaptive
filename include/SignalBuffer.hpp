

#include <string.h>
#include <inttypes.h>
#include <malloc.h>

namespace sb
{

    /**
     * A circular buffer for storing uint16_vals
     */
    class SignalBuffer
    {

    private:
        uint16_t *_buffer = nullptr;
        uint16_t _next_index = 0;

    public:
        bool dirty = false;

        SignalBuffer(int size)
        {
            _buffer = (uint16_t *)calloc(size, sizeof(uint16_t));
        }

        void push(uint16_t val)
        {
        }
    };
}