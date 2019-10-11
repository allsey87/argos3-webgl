/**
 * @file <argos3/plugins/simulator/visualizations/webgl/webgl_render.cpp>
 *
 * @author Ahmed Abou Zaidi - <XXX@gmail.com>
 */

#include "webgl_render.h"

#include <set>
#include <map>
#include <chrono>
#include <argos3/core/simulator/entity/embodied_entity.h>
#include <argos3/core/simulator/entity/composable_entity.h>
#include <argos3/core/simulator/space/space.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/string_utilities.h>
#include "websocket_server.h"

namespace argos {

   /****************************************/
   /****************************************/

   /* if possible, allocate memory in this method */
   void CWebGLRender::Init(TConfigurationNode& t_tree) {
      try {
         /* Overwrite defaults if they exist in the "webgl" XML configuration node */
         GetNodeAttributeOrDefault(t_tree, "bind", m_strBind, m_strBind);
         GetNodeAttributeOrDefault(t_tree, "port", m_unPort, m_unPort);
         GetNodeAttributeOrDefault(t_tree, "static", m_strStatic, m_strStatic);
         GetNodeAttributeOrDefault(t_tree, "start_browser", m_bStartBrowser, m_bStartBrowser);
         GetNodeAttributeOrDefault(t_tree, "interactive", m_bInteractive, m_bInteractive);
         GetNodeAttributeOrDefault(t_tree, "period", m_unPeriod, m_unPeriod);

         m_pcServer = new CWebsocketServer(m_strBind, m_unPort, m_strStatic, &m_cPlayState, this);
         LOG << "[INFO] WebGL visualization started on: "
             << "https://" << m_strBind << ":" << m_unPort << "/" << std::endl
             << "[INFO] Serving the static folder: " << m_strStatic << std::endl;
      }
      catch(CARGoSException& ex) {
         THROW_ARGOSEXCEPTION_NESTED("Error initializing the WebGL visualisation plugin", ex);
      }
   }

   /****************************************/
   /****************************************/

   CByteArray* CWebGLRender::GetSpawnMsg(networkId_t u_id) {
      return m_vecSpawnMessages[u_id].get();
   }

   /****************************************/
   /****************************************/

   void CWebGLRender::SendSpawn(std::unique_ptr<CByteArray> c_Data, CComposableEntity& c_entity) {
      m_mapNetworkId[c_entity.GetId()] = m_mapNetworkId.size();
      m_vecSpawnMessages.push_back(std::move(c_Data));
      m_vecEntites.push_back(&c_entity);
   }

   /****************************************/
   /****************************************/

   /* if possible, do not allocate or deallocate memory in this method */
   void CWebGLRender::Execute() {
      if(m_bStartBrowser) {
#ifdef __APPLE__
         std::string strStartBrowser("open");
#else
         std::string strStartBrowser("xdg-open");
#endif
         strStartBrowser += (" https://" + m_strBind + ":" + std::to_string(m_unPort) + "/");
         ::system(strStartBrowser.c_str()); 
      }
      CSpace& cSpace = m_cSimulator.GetSpace();
      CEntity::TVector& vecEntities = cSpace.GetRootEntityVector();
      for(CEntity::TVector::iterator itEntities = vecEntities.begin();
         itEntities != vecEntities.end();
         ++itEntities) {
         CallEntityOperation<CWebglSpawnInfo, CWebGLRender, void>(*this, **itEntities);
      }
      auto lastUpdate = std::chrono::steady_clock::now();
      /* loop here until experiment done */
      while(!m_cSimulator.IsExperimentFinished()) {
         if (m_cPlayState.SimulationShouldAdvance()){
            m_cSimulator.UpdateSpace();
            m_bDirty = true;
         }

         if (m_bDirty) {
            auto elapsed = std::chrono::steady_clock::now() - lastUpdate;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > m_unPeriod) {
               lastUpdate = std::chrono::steady_clock::now();
               m_bDirty = false;
               CEntity::TVector& vecEntities = cSpace.GetRootEntityVector();
               for(CEntity::TVector::iterator itEntities = vecEntities.begin();
                  itEntities != vecEntities.end();
                  ++itEntities) {
                  //
                  CallEntityOperation<CWebglUpdateInfo, CWebGLRender, void>(*this, **itEntities);
               }
            }
         }
        m_pcServer->Step();
      }
      /* at this point we should gracefully close any connections */
   }

   /****************************************/
   /****************************************/

   void CWebGLRender::SendUpdates(CByteArray* c_data) {// todo change
      m_pcServer->SendBinary(c_data);
   }

   /****************************************/
   /****************************************/

   /* if possible, deallocate memory in this method */
   void CWebGLRender::Destroy() {
      delete m_pcServer;
   }

   /****************************************/
   /****************************************/

   /* this method should be used to reset the user interface */
   void CWebGLRender::Reset() {


   }

   /****************************************/
   /****************************************/

   void CWebGLRender::RecievedMove(CByteArray& c_Data) {
      lwsl_user("MOVING ELEMENT\n");
      UInt32 uNetworkId;
      c_Data >> uNetworkId;
      std::string strId = m_vecEntites[uNetworkId]->GetId();
      LOG << "MOVING ELEMENT: " << strId << std::endl;
      Real fX, fY, fZ;
      c_Data >> fX;
      c_Data >> fY;
      // TODO remove fZ because it does not need to
      // be sent
      c_Data >> fZ;

      CVector3 cNewPos(fX, fY, fZ);
      CEntity* pcEntity = m_cSimulator.GetSpace().GetEntityMapPerId()[strId];
      CEmbodiedEntity* pcBodyEntity = dynamic_cast<CEmbodiedEntity*>(pcEntity);
      if (pcBodyEntity == NULL) {
         CComposableEntity* pcCompEntity = dynamic_cast<CComposableEntity*>(pcEntity);
         if(pcCompEntity != NULL && pcCompEntity->HasComponent("body")) {
               pcBodyEntity = &pcCompEntity->GetComponent<CEmbodiedEntity>("body");
         } else {
            return;
         }
      }

      lwsl_user("(%f, %f, %f)", fX, fY, fZ);

      pcBodyEntity->MoveTo(cNewPos,
                             pcBodyEntity->GetOriginAnchor().Orientation);
      m_bDirty = true;
   }

   /****************************************/
   /****************************************/

   /* Any documentation goes here */
   REGISTER_VISUALIZATION(CWebGLRender,
                          "webgl",
                          "Ahmed Abou Zaidi [XXX@gmail.com]",
                          "1.0",
                          "A browser-based interactive renderer based on WebGL. This plugin is based\n"
                          "in part on the work of the libwebsockets project (https://libwebsockets.org)",
                          "REQUIRED XML CONFIGURATION\n\n"
                          "  <visualization>\n"
                          "    <webgl />\n"
                          "  </visualization>\n\n"
                          "OPTIONAL XML CONFIGURATION\n\n"
                          "  <visualization>\n"
                          "    <webgl />\n"
                          "  </visualization>\n\n",
                          "Usable"
   );

   /****************************************/
   /****************************************/

}
