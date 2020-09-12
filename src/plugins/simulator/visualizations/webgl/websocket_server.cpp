#include "websocket_server.h"
#include <algorithm>

#define MY_PROTOCOL                                                            \
    { "spawn-objects", my_callback, sizeof(SPerSessionData), 1024, 0, NULL, 0 }

namespace argos {

static const struct lws_protocol_vhost_options GLB_MIMETYPE = {
        NULL, NULL, ".glb", "application/x-binary"};

static struct lws_http_mount MOUNT_SETTINGS = {
        /* .mount_next*/           NULL,        /* linked-list "next" */
        /* .mountpoint */        "/",        /* mountpoint URL */
        /* .origin */        nullptr,    /* serve from dir */
        /* .def */            "index.html",    /* default filename */
        /* .protocol */            NULL,
        /* .cgienv */            NULL,
        /* .extra_mimetypes */    &GLB_MIMETYPE,
        /* .interpret */        NULL,
        /* .cgi_timeout */        0,
        /* .cache_max_age */        0,
        /* .auth_mask */        0,
        /* .cache_reusable */        0,
        /* .cache_revalidate */        0,
        /* .cache_intermediaries */    0,
        /* .origin_protocol */        LWSMPRO_FILE,    /* files in a dir */
        /* .mountpoint_len */        1,        /* char count */
        /* .basic_auth_login_file */    NULL,
};

static struct lws_protocols PROTOCOLS[] = {
    {"http", lws_callback_http_dummy, 0, 0}, // serving static files
    MY_PROTOCOL,
    {NULL, NULL, 0, 0}
};

struct lws_protocol_vhost_options ACCESS_CONTROL_ALLOW_ORIGIN {
    nullptr, nullptr, "access-control-allow-origin:", "*"
};

struct lws_protocol_vhost_options ACCESS_CONTROL_REQUEST_HEADERS = {
    /*next*/ &ACCESS_CONTROL_ALLOW_ORIGIN,
    /*options*/ nullptr, "access-control-request-headers:", "*"};


CWebsocketServer::CWebsocketServer(std::string str_HostName, UInt16 un_Port,
                                   std::string str_Static, CPlayState* pc_paly_state,
                                   CWebGLRender* pc_visualization, CLuaControllers* pc_LuaContainer)
    : m_strHostName(str_HostName), m_strStatic(str_Static), m_unPort(un_Port),
    m_pcPalyState(pc_paly_state), m_pcVisualization(pc_visualization),
    m_pcLuaContainer(pc_LuaContainer),
    m_sLuaScriptsEntry{0},
    m_psPlayMsg(std::make_shared<SMessage>(LWS_WRITE_BINARY)),
    m_psPauseMsg(std::make_shared<SMessage>(LWS_WRITE_BINARY))
    {
}

void CWebsocketServer::Run() {
    while (!m_bStop) {
        Step();
    }
}

void CWebsocketServer::Step() { lws_service(m_psContext, 0); lws_service(m_psContext, 0); }

void CWebsocketServer::SendUpdate(UInt32 u_NetId, CByteArray* c_data) {
    // TODO lock
    m_cSimulationState.UpdateVersion(u_NetId, new SMessage(c_data, LWS_WRITE_BINARY));
    lws_cancel_service(m_psContext);
    // lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
}

void CWebsocketServer::SendUpdateText(UInt32 u_NetId, CByteArray* c_data) {
    // TODO lock
    m_cSimulationState.UpdateVersion(u_NetId, new SMessage(c_data, LWS_WRITE_TEXT));
    lws_cancel_service(m_psContext);
    // lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
}

/*void CWebsocketServer::SendText(const std::string& str_send) {
    m_cRingBuffer.AddTextMessage(str_send);
    lws_cancel_service(m_psContext);
}*/

void CWebsocketServer::WriteSpawn(SPerSessionData* ps_session) {
    if (WriteMessage(ps_session)) {
        ps_session->m_uLastSpawnedNetId += 1;
        ps_session->m_sCPP->m_vecUpdateVersions.push_back(0);
    }
    lws_cancel_service(m_psContext);
    // lws_callback_on_writable(ps_session->m_psWSI);
}

/*void CWebsocketServer::WriteLua(SPerSessionData* ps_session) {
    if (WriteMessage(ps_session)) ps_session->m_bHasLua = true;
    lws_callback_on_writable(ps_session->m_psWSI);
}*/


int CWebsocketServer::Callback(SPerSessionData *ps_session, lws_callback_reasons e_reason,
            UInt8* un_bytes, size_t u_len) {
    
    switch (e_reason) {
    case LWS_CALLBACK_ESTABLISHED:
        m_vecClients.push_back(ps_session);
        ps_session->m_uLastSpawnedNetId = 0; // maybe the already 0 at allocation
        ps_session->m_uLuaVersion = 0;
        ps_session->m_uSent = 0;
        ps_session->m_psCurrentRecvMessage.data = new CByteArray();
        ps_session->m_bIsPlaying = false;
        ps_session->m_sCPP = new SCPPPerSessionData;
        lws_callback_on_writable(ps_session->m_psWSI);
        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        m_bStop = true;
        break;
    case LWS_CALLBACK_CLOSED:
        delete ps_session->m_psCurrentRecvMessage.data; // delete
        delete ps_session->m_sCPP;

        m_vecClients.erase(std::find(m_vecClients.begin(), m_vecClients.end(), ps_session));

        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        //// With this version: incrementation of spawned_id, lua_od should happen before starting to write
        /* Continue sending */
        if (ps_session->m_sCPP->m_psCurrentSendMessage.get()/*ps_session->m_uSent != 0*/) {/////////////////////////
            WriteMessage(ps_session);
        } else { /* Prepare a new Message */
            /* Lua files will have rare race conditions for multiple clients */
            if (ShouldRecieveSpawn(ps_session)) {
                ps_session->m_sCPP->m_psCurrentSendMessage = GetSpawnMsg(ps_session->m_uLastSpawnedNetId);
                WriteSpawn(ps_session);
            } else if (ps_session->m_bIsPlaying != m_pcPalyState->isPlaying()) {
                if (ps_session->m_bIsPlaying) {
                    ps_session->m_sCPP->m_psCurrentSendMessage = m_psPlayMsg;
                } else {
                    ps_session->m_sCPP->m_psCurrentSendMessage = m_psPauseMsg;
                }
                ps_session->m_bIsPlaying = !ps_session->m_bIsPlaying;
            }
            else if (ps_session->m_uLuaVersion < m_sLuaScriptsEntry.m_uVersion) {
                ps_session->m_uLuaVersion = m_sLuaScriptsEntry.m_uVersion;
                ps_session->m_sCPP->m_psCurrentSendMessage = m_sLuaScriptsEntry.m_psMessage;
                WriteMessage(ps_session);
            } else {
                for (UInt32 i = ps_session->m_uNextUpdateId; i < ps_session->m_sCPP->m_vecUpdateVersions.size(); ++i) {
                    UInt32& uVersion = ps_session->m_sCPP->m_vecUpdateVersions[i];
                    SEntry sEntry = m_cSimulationState.GetLastVersionIfNewer(i, uVersion);
                    if (sEntry.m_psMessage) {
                        uVersion = sEntry.m_uVersion;
                        ps_session->m_sCPP->m_psCurrentSendMessage = std::move(sEntry.m_psMessage);
                        ps_session->m_uNextUpdateId = (i + 1) % ps_session->m_sCPP->m_vecUpdateVersions.size();
                        return 0;
                    }
                }
                for (UInt32 i = 0; i < ps_session->m_uNextUpdateId; ++i) {
                    UInt32& uVersion = ps_session->m_sCPP->m_vecUpdateVersions[i];
                    SEntry sEntry = m_cSimulationState.GetLastVersionIfNewer(i, uVersion);
                    if (sEntry.m_psMessage) {
                        uVersion = sEntry.m_uVersion;
                        ps_session->m_sCPP->m_psCurrentSendMessage = std::move(sEntry.m_psMessage);
                        ps_session->m_uNextUpdateId = (i + 1) % ps_session->m_sCPP->m_vecUpdateVersions.size();
                        return 0;
                    }
                }
            }
        }
        break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end(); ++tIter){
            // TODO: if (ps_session.sumversion < m_cSimulationState.sumversion)
            lws_callback_on_writable((*tIter)->m_psWSI);
        }
        break;
    case LWS_CALLBACK_RECEIVE:
        ps_session->m_psCurrentRecvMessage.data->AddBuffer(un_bytes, u_len);

        if (lws_is_final_fragment(ps_session->m_psWSI)) {
            ps_session->m_psCurrentRecvMessage.type = lws_frame_is_binary(ps_session->m_psWSI)
                ? LWS_WRITE_BINARY
                : LWS_WRITE_TEXT;
            ReceivedMessage(&ps_session->m_psCurrentRecvMessage, ps_session);
            ps_session->m_psCurrentRecvMessage.data->Clear();
        }
        break;
    default:
        // LOGERR << "[LWS] Unhandled case reason=" << e_reason << std::endl;
        break;
    }
    return 0;
}

void CWebsocketServer::UpdatedLua() {
    // lws_cancel_service(m_psContext);
    lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
    // TODO check if lws_callback_on_writable is better
    /*for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end(); ++tIter) {
        lws_callback_on_writable((*tIter)->m_psWSI);
    }*/
}

void CWebsocketServer::ReceivedMessage(SMessage *ps_msg, SPerSessionData* ps_sender) {
        lwsl_user("Received size: %lu\n", ps_msg->data->Size());

        EClientMessageType unMessageType;

        if (ps_msg->type == LWS_WRITE_TEXT) {
            std::string strMessage;
            strMessage.reserve(ps_msg->data->Size());
            (*ps_msg->data) >> strMessage;
            size_t unSep = strMessage.find("///");
            m_pcLuaContainer->UpdateScriptContent(strMessage.substr(0, unSep), strMessage.substr(unSep + 3));
            UpdateLuaScripts();
            ++(ps_sender->m_uLuaVersion);
            UpdatedLua();
            unMessageType = EClientMessageType::PAUSE;
        } else {
            UInt8 unMessageTypeTemp;
            *(ps_msg->data) >> unMessageTypeTemp;
            unMessageType = (EClientMessageType) unMessageTypeTemp;
            switch(unMessageType) {
                case EClientMessageType::PAUSE:
                    m_pcPalyState->Pause();
                break;
                case EClientMessageType::AUTO:
                    m_pcPalyState->Automatic();
                break;
                case EClientMessageType::STEP:
                    m_pcPalyState->Frame();
                break;
                case EClientMessageType::MOVE:
                    m_pcVisualization->RecievedMove(*(ps_msg->data));
                    break;
            default:
                lwsl_err("Unknown message type\n");
                return;
            }
        }
        lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
    }

void CWebsocketServer::CreateContext() {
    struct lws_context_creation_info sInfo;
    memset(&sInfo, 0, sizeof sInfo);
    MOUNT_SETTINGS.origin = m_strStatic.c_str();
    sInfo.mounts = &MOUNT_SETTINGS;
    sInfo.vhost_name = m_strHostName.c_str();
    sInfo.port = m_unPort;
    sInfo.protocols = PROTOCOLS;
    sInfo.user = reinterpret_cast<void *>(this);
    sInfo.headers = &ACCESS_CONTROL_REQUEST_HEADERS;
    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    m_psContext = lws_create_context(&sInfo);
    if (!m_psContext) {
	    std::cout << "Context creation failed" << std::endl;
    } else {
	    std::cout << "Context creation success" << std::endl;
    }

    SMessage* psMsg = m_psPlayMsg.get();
    *(psMsg->data) << EMessageType::PLAYSTATE << (UInt8) EClientMessageType::AUTO;
    psMsg = m_psPauseMsg.get();
    *(psMsg->data) << EMessageType::PLAYSTATE << (UInt8) EClientMessageType::PAUSE;
}


CWebsocketServer::~CWebsocketServer() {
    m_bStop = true;
    if (m_psContext)
        lws_context_destroy(m_psContext);
}

void CWebsocketServer::Prepare() {
    UpdateLuaScripts();
}

void CWebsocketServer::UpdateLuaScripts() {
    ++(m_sLuaScriptsEntry.m_uVersion);
    m_sLuaScriptsEntry.m_psMessage = std::make_shared<SMessage>(LWS_WRITE_TEXT);
    m_sLuaScriptsEntry.m_psMessage->data->operator<<(m_pcLuaContainer->GetJson());
    m_sLuaScriptsEntry.m_psMessage->data->Resize(m_sLuaScriptsEntry.m_psMessage->data->Size() - 1);
}

int my_callback(lws *ps_wsi, enum lws_callback_reasons e_reason, void *user,
                void *in, size_t un_len) {
    if (e_reason == LWS_CALLBACK_PROTOCOL_INIT) {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_zalloc(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi),
                sizeof(SDataPerVhost)));
        ps_vhd->context = lws_get_context(ps_wsi);
        ps_vhd->protocol = lws_get_protocol(ps_wsi);
        ps_vhd->vhost = lws_get_vhost(ps_wsi);
        ps_vhd->server = reinterpret_cast<CWebsocketServer *>(
            lws_context_user(ps_vhd->context));
        ps_vhd->server->Prepare();
    } else {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_get(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi)));
    if (!ps_vhd) return 0;
        SPerSessionData *ps_SessionData =
            reinterpret_cast<SPerSessionData *>(user);
        if (e_reason == LWS_CALLBACK_ESTABLISHED) {
            ps_SessionData->m_psWSI = ps_wsi;
        }
        return ps_vhd->server->Callback(ps_SessionData, e_reason, (UInt8*) in, un_len);
    }
    return 0;
}

} // namespace argos
