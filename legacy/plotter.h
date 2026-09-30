#ifndef PLOTTER_H
#define PLOTTER_H

#include <stdio.h>
#include "Stream.h"
#include "SwTimer.h"

namespace Plotter {


#define COMMANDS_QUANTITY 3u

#define START_SIGN 0xA5A5

enum State_e  {
    HEADER_STATE = 0x00,
    COMMAND_STATE,
    DATA_STATE,
    RESPONSE_STATE,

};

enum Commands_e {
    NO_CMD = 0x00000000,
    PING   = 0x00000010,
    PONG   = 0x00000020,
    DATA   = 0x00000001,
};


#pragma pack(push, 1)
struct Header_t {
    uint16_t start_sign;
    Commands_e cmd;
    uint16_t length;
};

struct Message_t {
    Header_t header;
    uint8_t *data;
};

#pragma pack(pop)


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

        void loop(void);
        
        
    private:
        bool write_message(void);
        bool set_message_to_write(Commands_e cmd, uint32_t length = 0, uint8_t * data = nullptr);
        bool valid_cmd(Commands_e cmd);

        State_e header_state(State_e cState);
        State_e command_state(State_e cState);
        State_e data_state(State_e cState);
        State_e response(State_e cState);

    private:
        Stream* m_stream;
        bool m_connected;
        State_e m_current_state;
        State_e m_old_state;
        Header_t m_receive_header;
        SwTimer *m_connection_timeout;
        Message_t m_response_message;

};



}


#endif