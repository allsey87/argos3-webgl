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
    {"http", lws_callback_http_dummy, 0, 0}, MY_PROTOCOL, {NULL, NULL, 0, 0}};

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
    m_cRingBuffer(80), m_pcLuaContainer(pc_LuaContainer) {
    struct lws_context_creation_info sInfo;
    memset(&sInfo, 0, sizeof sInfo);
    MOUNT_SETTINGS.origin = m_strStatic.c_str();
    sInfo.mounts = &MOUNT_SETTINGS;
    sInfo.vhost_name = m_strHostName.c_str();
    sInfo.port = un_Port;
    sInfo.protocols = PROTOCOLS;
    sInfo.user = reinterpret_cast<void *>(this);
    sInfo.headers = &ACCESS_CONTROL_REQUEST_HEADERS;
    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    m_psContext = lws_create_context(&sInfo);
    if (!m_psContext) {
        LOGERR << "Context creation failed" << std::endl;
    }
}

void CWebsocketServer::Run() {
    while (!m_bStop) {
        Step();
    }
}

void CWebsocketServer::Step() { lws_service(m_psContext, 10); }

void CWebsocketServer::SendBinary(CByteArray* c_data) {
    m_cRingBuffer.AddBinaryMessage(c_data);
    lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
}

void CWebsocketServer::SendText(const std::string& str_send) {
    m_cRingBuffer.AddTextMessage(str_send);
    lws_cancel_service(m_psContext);
}

void CWebsocketServer::WriteSpawn(SPerSessionData* ps_session) {
    if (WriteMessage(ps_session)) {
        ps_session->m_uLastSpawnedNetId += 1;
        // the client finished spawning all entities
        if (!ShouldRecieveSpawn(ps_session) && !ps_session->m_bInRing) {
            m_vecSpawningClients.erase(std::find(m_vecSpawningClients.begin(), m_vecSpawningClients.end(), ps_session));
            ps_session->m_bInRing = true;
            m_cRingBuffer.AddClient(ps_session);
        }
    }
    lws_callback_on_writable(ps_session->m_psWSI);
}

void CWebsocketServer::WriteLua(SPerSessionData* ps_session) {
    if (WriteMessage(ps_session)) ps_session->m_bHasLua = true;
    lws_callback_on_writable(ps_session->m_psWSI);
}


int CWebsocketServer::Callback(SPerSessionData *ps_session, lws_callback_reasons e_reason,
            UInt8* un_bytes, size_t u_len) {
    
    switch (e_reason) {
    case LWS_CALLBACK_ESTABLISHED:
        m_vecSpawningClients.push_back(ps_session);
        ps_session->m_uLastSpawnedNetId = 0; // maybe the already 0 at allocation
        ps_session->m_bKicked = false;
        ps_session->m_bHasLua = false;
        ps_session->m_uSent = 0;
        ps_session->m_psCurrentRecvMessage.data = new CByteArray();
        lws_callback_on_writable(ps_session->m_psWSI);
        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        m_bStop = true;
        break;
    case LWS_CALLBACK_CLOSED:
        delete ps_session->m_psCurrentRecvMessage.data;
        /* kicked imply being member of the ring
           and not member of m_vecspawnedclient
           and the ring already did the necessary to remove*/
        if (ps_session->m_bKicked) break; 
        else {
            if (ps_session->m_bInRing) m_cRingBuffer.DisconectedClient(ps_session);
            else m_vecSpawningClients.erase(std::find(m_vecSpawningClients.begin(), m_vecSpawningClients.end(), ps_session));
        }
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (ps_session->m_bKicked) return -1;

        /* Continue sending */
        if (ps_session->m_uSent != 0) {/////////////////////////
            switch (ps_session->m_eSendType) {
                case SPerSessionData::ESendMessageStep::Lua:
                    WriteLua(ps_session);
                break;
                case SPerSessionData::ESendMessageStep::Ring:
                    m_cRingBuffer.CallbackWriteMessage(ps_session);
                break;
                case SPerSessionData::ESendMessageStep::Spawn:
                    WriteSpawn(ps_session);
                break;
            }
        } else { /* Prepare a new Message */
            /* Lua files will have rare race conditions for multiple clients */
            if (!ps_session->m_bHasLua) {
                ps_session->m_psCurrentSendMessage = m_sLuaScripts;
                ps_session->m_eSendType = SPerSessionData::ESendMessageStep::Lua;

                WriteLua(ps_session);
            } else if (ShouldRecieveSpawn(ps_session)) {
                CByteArray* pcData = GetSpawnMsg(ps_session->m_uLastSpawnedNetId);
                ps_session->m_psCurrentSendMessage = {pcData, LWS_WRITE_TEXT};
                ps_session->m_eSendType = SPerSessionData::ESendMessageStep::Spawn;

                WriteSpawn(ps_session);
            } else if (ps_session->m_bInRing) {
                ps_session->m_eSendType = SPerSessionData::ESendMessageStep::Ring;
                m_cRingBuffer.CallbackWriteMessage(ps_session);
            }
        }
        break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        m_cRingBuffer.CallbackCancel();
        break;
    case LWS_CALLBACK_RECEIVE:
        ps_session->m_psCurrentRecvMessage.data->AddBuffer(un_bytes, u_len);

        if (lws_is_final_fragment(ps_session->m_psWSI)) {
            ps_session->m_psCurrentRecvMessage.type = lws_frame_is_binary(ps_session->m_psWSI)
                ? LWS_WRITE_BINARY
                : LWS_WRITE_TEXT;
            ReceivedMessage(&ps_session->m_psCurrentRecvMessage);
            ps_session->m_psCurrentRecvMessage.data->Clear();
        }
        break;
    default:
        LOGERR << "[LWS] Unhandled case reason=" << e_reason << std::endl;
        break;
    }
    return 0;
}

void CWebsocketServer::ReceivedMessage(SMessage *ps_msg) {
        lwsl_user("Received size: %lu\n", ps_msg->data->Size());
        UInt8 unMessageType;
        *(ps_msg->data) >> unMessageType;
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
            break;
        }
    }

CWebsocketServer::~CWebsocketServer() {
    m_bStop = false;
    if (m_psContext)
        lws_context_destroy(m_psContext);
}

void CWebsocketServer::Prepare() {
    m_sLuaScripts.data = new CByteArray();
    m_sLuaScripts.type = LWS_WRITE_TEXT;
    UpdateLuaScripts();
}

void CWebsocketServer::UpdateLuaScripts() {
    CByteArray* pcData = m_sLuaScripts.data;
    (*pcData) << m_pcLuaContainer->GetJson();
    pcData->Resize(pcData->Size() - 1);
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
        LOG << ps_vhd << std::endl;
    } else {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_get(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi)));
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
