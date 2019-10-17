#ifndef WEBSOCKET_SERVER
#define WEBSOCKET_SERVER

#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "webgl_render.h"
#include <functional>
#include "play_state.h"
#include "ring_buffer.h"

namespace argos {

int my_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);

namespace EClientMessageType {
    const UInt8 PAUSE=0;
    const UInt8 AUTO=1;
    const UInt8 STEP=2;
    const UInt8 RESET=3;
    const UInt8 MOVE=4;
};


class CWebsocketServer {
    typedef std::vector<SPerSessionData*> TClients;
    typedef TClients::iterator TIterCleints;
public:
    CWebsocketServer(std::string str_HostName, UInt16 un_Port,
        std::string str_Static, CPlayState* pc_paly_state,
        CWebGLRender* pc_visualization, CLuaControllers* pc_LuaContainer);
    
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

    void AddSpawnMessage(std::unique_ptr<CByteArray> c_Data) {
        m_vecSpawnMessages.push_back(std::move(c_Data));
    }

    void Prepare();

    void UpdateLuaScripts();

    void WriteSpawn(SPerSessionData* ps_session);

    void WriteLua(SPerSessionData* ps_session);

private:
    void ReceivedMessage(SMessage *ps_msg);
    inline bool ShouldRecieveSpawn(SPerSessionData* ps_session) {
        return ps_session->m_uLastSpawnedNetId < m_pcVisualization->GetRootEntitiesCount();
    }
    CByteArray* GetSpawnMsg(UInt32 u_id) {
        return m_vecSpawnMessages[u_id].get();
    }

private:
    std::string m_strHostName;
    std::string m_strStatic;
    UInt16 m_unPort;
    struct lws_context *m_psContext;
    bool m_bStop;
    CPlayState* m_pcPalyState;
    CWebGLRender* m_pcVisualization;
    std::vector<std::unique_ptr<CByteArray>> m_vecSpawnMessages;
    /*
        New clients are not taken into account by the ring
    */
    std::vector<SPerSessionData*> m_vecSpawningClients;
    CRingBuffer m_cRingBuffer;
    CLuaControllers* m_pcLuaContainer;
    SMessage m_sLuaScripts;
};

struct SDataPerVhost {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    CWebsocketServer* server;
};

}


#endif