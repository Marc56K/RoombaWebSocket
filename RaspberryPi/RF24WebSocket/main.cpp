#include <iostream>
#include <string>
#include <unistd.h>
#include <RF24/nRF24L01.h>
#include <RF24/RF24.h>
  
//#include <server_ws.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#define CMD_WAKEUP 1
#define CMD_START 128
#define CMD_SAFE_MODE 131
#define CMD_CLEAN 135

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24);//, BCM2835_SPI_SPEED_8MHZ);
const uint8_t data_pipe[6] = "00001";

struct Message
{
  unsigned char cmd;
  unsigned char dataSize;
  unsigned char data[30];
};

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

bool send(Message& msg)
{
    if (!radio.write(&msg, sizeof(Message)))
    {
      std::cout<<"sending failed"<<std::endl;
      return false;
    }
    return true;
}

bool sendCommand(unsigned char cmd)
{
    Message msg = {};
    msg.cmd = cmd;
    return send(msg);
}

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) 
{
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;
              
    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening")
    {
        s->stop_listening();
        return;
    }

    unsigned char value = static_cast<unsigned char>(std::stoi(msg->get_payload()));
    std::cout << (int)value << std::endl;
    sendCommand(value);

    try 
    {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } 
    catch (websocketpp::exception const & e) 
    {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

void setup(void)
{   
    radio.begin();   
    radio.setRetries(15, 15);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(data_pipe);

    if (!radio.isChipConnected())
    {
        std::cout<<"chip not connected!"<<std::endl;
    }
    
    if (radio.failureDetected)
    {
        std::cout<<"failureDetected"<<std::endl;
    }
    
    radio.printDetails();
}

int main(int argc, char** argv) 
{
    setup();
    sleep(1);
    
    /*
    sendCommand(CMD_WAKEUP);
    sendCommand(CMD_START);
    sendCommand(CMD_SAFE_MODE);
    sendCommand(CMD_CLEAN);
    
    return 0;
    */
    
    server echo_server;
    
    try 
    {
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(bind(&on_message,&echo_server,::_1,::_2));

        // Listen on port 9002
        echo_server.listen(9002);

        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}