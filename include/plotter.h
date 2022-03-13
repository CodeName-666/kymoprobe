#ifndef PLOTTER_H
#define PLOTTER_H

#include <stdio.h>
#include "Stream.h"


namespace Plotter {


enum Commands_e {
    PING = 0x1000,
    PONG = 0x2000,
    DATA = 0x0100,
};
 
struct Header_t {
    uint32_t start_sign;
    Commands_e cmd;
    uint32_t length;
};


struct Message_t {
    Header_t header;
    uint32_t *data;
};

class Plotter
{
    public:
        /**
         * @brief Construct a new Plotter object
         * 
         */
        explicit Plotter(void);
        /**
         * @brief 
         * 
         * @param stream 
         */
        void init(Stream& stream);


        



    private:
        Stream* m_stream;
        bool m_connected;
};



}


#endif