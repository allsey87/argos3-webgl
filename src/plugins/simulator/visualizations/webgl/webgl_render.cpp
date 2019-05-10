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

         m_pcServer = new CWebsocketServer(m_strBind, m_unPort, m_strStatic);
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

   void CWebGLRender::SendSpawn(CByteArray c_Data, CComposableEntity& c_entity) {
      m_mapNetworkId[c_entity.GetId()] = m_mapNetworkId.size();
      m_pcServer->SendBinary(c_Data);
   }

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
      m_pcServer->waitForConnection();
      CEntity::TVector& vecEntities = m_cSimulator.GetSpace().GetRootEntityVector();
      for(CEntity::TVector::iterator itEntities = vecEntities.begin();
         itEntities != vecEntities.end();
         ++itEntities) {
         CallEntityOperation<CWebglSpawnInfo, CWebGLRender, void>(*this, **itEntities);
      }
      auto lastUpdate = std::chrono::steady_clock::now();
      /* loop here until experiment done */
      while(!m_cSimulator.IsExperimentFinished()) {
         m_cSimulator.UpdateSpace();
         auto elapsed = std::chrono::steady_clock::now() - lastUpdate;
         if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1200) {
            lastUpdate = std::chrono::steady_clock::now();

            CEntity::TVector& vecEntities = m_cSimulator.GetSpace().GetRootEntityVector();
            for(CEntity::TVector::iterator itEntities = vecEntities.begin();
               itEntities != vecEntities.end();
               ++itEntities) {
               CallEntityOperation<CWebglUpdateInfo, CWebGLRender, void>(*this, **itEntities);
            }
         }
        m_pcServer->Step();
      }
      /* at this point we should gracefully close any connections */
   }

   void CWebGLRender::SendPosition(CComposableEntity& c_entity, CByteArray c_data) {
      CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
      const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
      const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
      CRadians cZAngle, cYAngle, cXAngle;
      cBodyOrientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);

      CByteArray cData;
      cData << EMessageType::UPDATE;
           cData << m_mapNetworkId[c_entity.GetId()]
            << cBodyPosition.GetX()
            << cBodyPosition.GetY()
            << cBodyPosition.GetZ()
            << cXAngle.GetValue()
            << cYAngle.GetValue()
            << cZAngle.GetValue();

      size_t size = c_data.Size();
      UInt8 buffer[size];
      c_data.FetchBuffer(buffer, size);
      cData.AddBuffer(buffer, size);
      m_pcServer->SendBinary(cData);
   }

   void CWebGLRender::SendPosition(CComposableEntity& c_entity) {
      CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
      const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
      const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
      CRadians cZAngle, cYAngle, cXAngle;
      cBodyOrientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
      CByteArray cData;
      cData << EMessageType::UPDATE;
           cData << m_mapNetworkId[c_entity.GetId()]
            << cBodyPosition.GetX()
            << cBodyPosition.GetY()
            << cBodyPosition.GetZ() 
            << cXAngle.GetValue()
            << cYAngle.GetValue()
            << cZAngle.GetValue();
      // LOG << "DATA" << cData <<  std::endl << "Size" << cData.Size() << " " << sizeof(EMessageType::UPDATE) << std::endl;
      m_pcServer->SendBinary(cData);
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
