#ifndef WEBSOCKET_SERVER
#define WEBSOCKET_SERVER

#include <list>
#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
namespace argos {

int my_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);

struct SPerSessionData {
    struct lws *wsi;
};

struct ToSend {
    CByteArray data;
    lws_write_protocol type;
};



class CWebsocketServer {
public:
    CWebsocketServer(std::string str_HostName, UInt16 un_Port, std::string str_Static);
    
    /**
     * Will be used on multithread
    */
    void Run();

    /**
     * Run a step in the event loop of libwebsocket
     *
     **/ 
    void Step();

    /**
     * Send binary data
    */
    void SendBinary(CByteArray data);

    /**
     * Send text data
     **/
    void SendText(std::string send);

    int Callback(lws *ps_WSI, lws_callback_reasons e_Reason,
            SPerSessionData *ps_SessionData);
    
    void waitForConnection();
    
    ~CWebsocketServer();
private:
    std::string m_strHostName;
    std::string m_strStatic;
    UInt16 m_unPort;
    struct lws_context *m_psContext;
    bool m_bStop;
    SPerSessionData* client;
    std::list<ToSend> arrays;
};

struct SDataPerVhost {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    CWebsocketServer* server;
};

}


#endif