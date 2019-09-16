#ifndef WEBSOCKET_SERVER
#define WEBSOCKET_SERVER

#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "webgl_render.h"
#include <functional>
#include "play_state.h"

namespace argos {

int my_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);

struct SMessage {
    CByteArray* data;
    lws_write_protocol type;
};

struct SPerSessionData {
    struct lws *m_psWSI;
    UInt32 m_uLastSpawnedNetId; // TODO: network id
    UInt32 m_uRingTail;
    bool m_bKicked;
    SMessage m_psCurrentMessage;
    size_t m_uSent;
};

namespace EClientMessageType {
    const UInt8 PAUSE=0;
    const UInt8 AUTO=1;
    const UInt8 STEP=2;
    const UInt8 RESET=3;
    const UInt8 MOVE=4;
};


class CWebsocketServer {
public:
    CWebsocketServer(std::string str_HostName, UInt16 un_Port,
        std::string str_Static, CPlayState* pc_paly_state,
        CWebGLRender* pc_visualization);
    
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
    void SendBinary(CByteArray* data);

    /**
     * Send text data
     **/
    void SendText(const std::string& send);

    int Callback(SPerSessionData *ps_wsi, lws_callback_reasons e_reason,
    UInt8* ch, size_t len);

    ~CWebsocketServer();

    typedef std::vector<SPerSessionData*>::iterator TIterCleints;

private:
    void ReceivedMessage(SMessage *ps_msg);
    void EnsureRingSpace();
    /**
     * return true if the message is entirely sent
     *
     **/
    bool WriteMessage(SPerSessionData*, const SMessage*);

private:
    std::string m_strHostName;
    std::string m_strStatic;
    UInt16 m_unPort;
    struct lws_context *m_psContext;
    bool m_bStop;
    std::vector<SPerSessionData*> m_vecClients;
    CPlayState* m_pcPalyState;
    CWebGLRender* m_pcVisualization;
    lws_ring* m_psRingBuffer;
};

struct SDataPerVhost {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    CWebsocketServer* server;
};

}


#endif