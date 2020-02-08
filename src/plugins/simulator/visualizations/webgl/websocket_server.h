#ifndef WEBSOCKET_SERVER
#define WEBSOCKET_SERVER

#include <mutex>
#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "webgl_render.h"
#include <functional>
#include "play_state.h"
#include "simulation_state.h"

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

    void Stop() {
        this->m_bStop = true;
    }
    /**
     * Send update data
    */
    void SendUpdate(UInt32 u_netId, CByteArray* c_data);

    /**
     * Send text data
     **/
    // void SendText(const std::string& send);

    int Callback(SPerSessionData *ps_wsi, lws_callback_reasons e_reason,
    UInt8* ch, size_t len);

    ~CWebsocketServer();

    void AddSpawnMessage(CByteArray* c_Data) {
        m_cSimulationState.AddNewEntry();
        m_vecSpawnMessages.push_back(std::make_shared<SMessage>(SMessage{std::unique_ptr<CByteArray>(c_Data), LWS_WRITE_TEXT}));
    }

    void Prepare();

    void UpdateLuaScripts();

    void WriteSpawn(SPerSessionData* ps_session);

    // void WriteLua(SPerSessionData* ps_session);
    void UpdatedLua();

private:
    void ReceivedMessage(SMessage *ps_msg, SPerSessionData* ps_sender);
    inline bool ShouldRecieveSpawn(SPerSessionData* ps_session) {
        return ps_session->m_uLastSpawnedNetId < m_vecSpawnMessages.size();
    }
    std::shared_ptr<SMessage> GetSpawnMsg(UInt32 u_id) {
        return m_vecSpawnMessages[u_id];
    }

private:
    std::string m_strHostName;
    std::string m_strStatic;
    UInt16 m_unPort;
    struct lws_context *m_psContext;
    bool m_bStop;
    CPlayState* m_pcPalyState;
    CWebGLRender* m_pcVisualization;
    std::vector<std::shared_ptr<SMessage>> m_vecSpawnMessages;

    std::vector<SPerSessionData*> m_vecClients;
    CSimulationState m_cSimulationState;
    CLuaControllers* m_pcLuaContainer;
    SEntry m_sLuaScriptsEntry;
    
    // shared_ptr here is just to be compatible with the session attribute
    // of current message, they will never be replaced...
    std::shared_ptr<SMessage> m_psPlayMsg;
    std::shared_ptr<SMessage> m_psPauseMsg;
};

struct SDataPerVhost {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    CWebsocketServer* server;
};

}


#endif