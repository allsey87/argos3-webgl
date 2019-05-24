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

namespace EClientMessageType {
    const UInt8 PAUSE=0;
    const UInt8 AUTO=1;
    const UInt8 STEP=2;
};

class CPlayState {
private:
    bool m_bIsAutomatic;
    UInt32 m_unFramesToPlay;
public:
    void Pause() {
        m_unFramesToPlay = 0;
        m_bIsAutomatic = false;
    }

    void Frame() {
        m_bIsAutomatic = false;
        ++m_unFramesToPlay;
    }

    bool Automatic() {
        m_unFramesToPlay = 0;
        m_bIsAutomatic = true;
    }

    bool SimulationShouldAdvance() {
        if (m_bIsAutomatic) return true;
        if (m_unFramesToPlay == 0) return false;
        --m_unFramesToPlay;
        return true;
    }
};


class CWebsocketServer {
public:
    CWebsocketServer(std::string str_HostName, UInt16 un_Port,
        std::string str_Static, CPlayState* pc_paly_state);
    
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

    int Callback(SPerSessionData *ps_session_data, lws_callback_reasons e_reason,
    UInt8* ch, size_t len);
    
    void waitForConnection();
    
    ~CWebsocketServer();

private:
    void RecievedMessage();

private:
    std::string m_strHostName;
    std::string m_strStatic;
    UInt16 m_unPort;
    struct lws_context *m_psContext;
    bool m_bStop;
    SPerSessionData* m_sClient;
    std::list<ToSend> m_MessageQueue;
    CPlayState* m_pcPalyState;
    CByteArray m_cCurrentMessage;
};

struct SDataPerVhost {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    CWebsocketServer* server;
};

}


#endif