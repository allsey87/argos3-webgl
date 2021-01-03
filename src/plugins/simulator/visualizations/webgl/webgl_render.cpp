/**
 * @file <argos3/plugins/simulator/visualizations/webgl/webgl_render.cpp>
 *
 * @author Ahmed Abou Zaidi - <XXX@gmail.com>
 */

#include "webgl_render.h"

#include <set>
#include <map>
#include <chrono>
#include <memory>
#include <thread>
#include <argos3/core/simulator/entity/embodied_entity.h>
#include <argos3/core/simulator/entity/composable_entity.h>
#include <argos3/core/simulator/space/space.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/string_utilities.h>
#include "websocket_server.h"
#include <argos3/core/wrappers/lua/lua_controller.h>

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

         m_pcServer = new CWebsocketServer(m_strBind, m_unPort, m_strStatic, &m_cPlayState, this, &m_cLuaContainer);
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

   void CWebGLRender::SendSpawn(CByteArray* c_Data, CComposableEntity& c_entity) {
      m_mapNetworkId[c_entity.GetId()] = m_mapNetworkId.size();
      m_pcServer->AddSpawnMessage(c_Data);
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

      CSpace::TMapPerType& tControllables = cSpace.GetEntitiesByType("controller");
      /* Go through them and keep a pointer to each Lua controller */
      for(CSpace::TMapPerType::iterator it = tControllables.begin();
          it != tControllables.end();
          ++it) {
         /* Try to convert the controller into a Lua controller */
         CControllableEntity* pcControllable = any_cast<CControllableEntity*>(it->second);
         CLuaController* pcLuaController = dynamic_cast<CLuaController*>(&(pcControllable->GetController()));
         if(pcLuaController) {
            /* Conversion succeeded, add to indices */
            m_cLuaContainer.AddController(pcLuaController);
         }
      }
      bool bGuard = false;
      std::thread cWebScocketThread([&]() {
         m_pcServer->CreateContext();
         bGuard = true;
         m_pcServer->Run();
      });
      while (!bGuard) {}

      CEntity::TVector& vecEntities = cSpace.GetRootEntityVector();
      for(CEntity::TVector::iterator itEntities = vecEntities.begin();
         itEntities != vecEntities.end();
         ++itEntities) {
         CallEntityOperation<CWebglSpawnInfo, CWebGLRender, void>(*this, **itEntities);
      }
      /* loop here until experiment done */
      while(!m_cSimulator.IsExperimentFinished()) {
         if (m_cPlayState.SimulationShouldAdvance()){
            m_cSimulator.UpdateSpace();
         }
         CEntity::TVector& vecEntities = cSpace.GetRootEntityVector();
         for(CEntity::TVector::iterator itEntities = vecEntities.begin();
            itEntities != vecEntities.end();
            ++itEntities) {
            CallEntityOperation<CWebglUpdateInfo, CWebGLRender, void>(*this, **itEntities);
         }

         if (m_vecMoves.size()) {
            std::lock_guard<std::mutex> cSync(m_cMovesMutex);
            for (auto it = m_vecMoves.begin(); it != m_vecMoves.end(); ++it) {
               std::string strId = m_vecEntites[it->first]->GetId();
               CEntity* pcEntity = m_cSimulator.GetSpace().GetEntityMapPerId()[strId];

               CEmbodiedEntity* pcBodyEntity = dynamic_cast<CEmbodiedEntity*>(pcEntity);
               if (pcBodyEntity == NULL) {
                  CComposableEntity* pcCompEntity = dynamic_cast<CComposableEntity*>(pcEntity);
                  if(pcCompEntity != NULL && pcCompEntity->HasComponent("body")) {
                        pcBodyEntity = &pcCompEntity->GetComponent<CEmbodiedEntity>("body");
                  } else {
                     continue;
                  }
               }

               pcBodyEntity->MoveTo(it->second,
                                    pcBodyEntity->GetOriginAnchor().Orientation);
               m_bDirty = true;
            }
         }
      }
      m_pcServer->Stop();
      cWebScocketThread.join();
      /* at this point we should gracefully close any connections */
   }

   /****************************************/
   /****************************************/

   void CWebGLRender::SendUpdates(CByteArray* c_Data, CComposableEntity& c_entity) {// todo change
      m_pcServer->SendUpdate(m_mapNetworkId[c_entity.GetId()], c_Data);
   }

   /****************************************/
   /****************************************/

   void CWebGLRender::SendUpdatesText(CByteArray* c_Data, CComposableEntity& c_entity) {
      m_pcServer->SendUpdateText(m_mapNetworkId[c_entity.GetId()], c_Data);
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

   void CWebGLRender::RecievedMove(std::pair<UInt32, CVector3>& c_Data) {
      lwsl_user("MOVING ELEMENT\n");
      std::lock_guard<std::mutex> cSync(m_cMovesMutex);
      m_vecMoves.push_back(c_Data);
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
