/**
 * @file <argos3/plugins/simulator/visualizations/webgl/webgl_render.cpp>
 *
 * @author Ahmed Abou Zaidi - <XXX@gmail.com>
 */

#include "webgl_render.h"

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
         GetNodeAttributeOrDefault(t_tree, "start_browser", m_bStartBrowser, m_bStartBrowser);
         GetNodeAttributeOrDefault(t_tree, "interactive", m_bInteractive, m_bInteractive);

         LOG << "[INFO] WebGL visualization started on: "
             << "https://" << m_strBind << ":" << m_unPort << "/" << std::endl;
      }
      catch(CARGoSException& ex) {
         THROW_ARGOSEXCEPTION_NESTED("Error initializing the WebGL visualisation plugin", ex);
      }
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
      /* loop here until experiment done */
      while(!m_cSimulator.IsExperimentFinished()) {
         LOGERR << "Executing step " << m_cSimulator.GetSpace().GetSimulationClock() << std::endl;
         /* advance the simulation by one tick */
         m_cSimulator.UpdateSpace();
         /* fetch the translations, rotations, ids of all simulated entities */
         for(CEntity* pc_entity : m_cSimulator.GetSpace().GetRootEntityVector()) {
            /* the follow code is a poor performing hack */
            /* TODO use proper disbatch methods */
            CComposableEntity* pc_composable = 
               dynamic_cast<CComposableEntity*>(pc_entity);
            if(pc_composable != nullptr && pc_composable->HasComponent("body")) {
               CEmbodiedEntity& cBody = pc_composable->GetComponent<CEmbodiedEntity>("body");
               const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
               const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
               LOG << pc_entity->GetId() << ": " << cBodyPosition << " / " << cBodyOrientation << std::endl;
               /*
               CByteArray cData;
               cData << cBodyPosition.GetX()
                     << cBodyPosition.GetY()
                     << cBodyPosition.GetZ()
                     << cBodyOrientation.GetW()
                     << cBodyOrientation.GetX()
                     << cBodyOrientation.GetY()
                     << cBodyOrientation.GetZ();
               LOG << "\tbytes (in network order): " << cData;
               */
            }
         }
      }
      /* at this point we should gracefully close any connections */
   }

   /****************************************/
   /****************************************/

   /* if possible, deallocate memory in this method */
   void CWebGLRender::Destroy() {


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
                          "A browser-based interactive renderer based on WebGL.",
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
