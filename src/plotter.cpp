#include "plotter.h"


namespace Plotter {
     
Plotter::Plotter(void)
{

}
   
void Plotter::init(Stream& stream)
{
    m_stream = &stream;
    m_current_state = HEADER_STATE;
}

void Plotter::loop(void)
{ 
    State_e next_state;
    switch(m_current_state)
    {
        case HEADER_STATE:
            next_state = header_state(m_current_state);
            break;
        case COMMAND_STATE:
            next_state = command_state(m_current_state);
            break;
        case DATA_STATE:
            next_state = data_state(m_current_state);
            break;
        case RESPONSE_STATE:
           next_state = response(m_current_state);
        default:
            next_state = HEADER_STATE;
    }
    m_current_state = next_state;
}

bool Plotter::valid_cmd(Commands_e cmd)
{
    if(cmd == PING || cmd == PONG ||
       cmd == DATA)
    {
        return true;
    }
    return false;   
}

State_e Plotter::header_state(State_e cState) 
{   
    State_e ret = HEADER_STATE;
    uint8_t* ptr = (uint8_t*)(&m_receive_header);

    if (m_stream->available() >= sizeof(Header_t))
    {
        Serial.println("... DATA Received... ");
        m_stream->readBytes(ptr, sizeof(Header_t));
        if(m_receive_header.start_sign == START_SIGN)
        {
            ret = COMMAND_STATE;   
            Serial.println("... Switc to Command State ...");
        }
    }
    return ret;
}


State_e Plotter::command_state(State_e cState)
{
    State_e next_state = HEADER_STATE;
    Serial.println("... Enter Command State ...");
    if(valid_cmd(m_receive_header.cmd))
    {
        switch(m_receive_header.cmd)    
        {
            case PING: 
                Serial.println("... Ping Detected ...");
                if(set_message_to_write(PONG))
                {
                   Serial.println("... Prepare Pong ...");
                   next_state = RESPONSE_STATE; 
                }
                break;
            case PONG: 
                
                break;
            case DATA: 

                break;
            default:
                break;
        }
    }
    return next_state;
}

State_e Plotter::data_state(State_e cState) 
{
    return HEADER_STATE; 
}

bool Plotter::write_message(void)
{

    size_t size = 0;
    uint32_t len = 0;
    bool ret = false;
    if(m_response_message.header.cmd != NO_CMD) 
    {
        len = m_response_message.header.length;
        size = m_stream->write((char*)&m_response_message.header, sizeof(Header_t));

        if(m_response_message.data != nullptr  && len > 0)
        {
            size += m_stream->write((char*)m_response_message.data, len);
        }
        
        if(size == sizeof(Header_t) + len);
        {
            ret = true;
        }
    }
    return ret;
}

bool Plotter::set_message_to_write(Commands_e cmd, uint32_t length, uint8_t* data)
{
    bool ret = false;
    if(valid_cmd(cmd))
    {
        m_response_message.header.start_sign = START_SIGN;
        m_response_message.header.cmd = cmd;
        m_response_message.header.length = length;
        m_response_message.data = data;
        ret = true;
    }
    return ret;
}

State_e Plotter::response(State_e cState)
{
    bool ret = write_message();
    if(ret == true)
    {
        return HEADER_STATE;
    }
}

}
