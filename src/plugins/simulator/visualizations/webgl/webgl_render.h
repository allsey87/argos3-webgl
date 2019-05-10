/**
 * @file <argos3/plugins/simulator/visualizations/webgl/webgl_render.h>
 *
 * @author Ahmed Abou Zaidi - <XXX@gmail.com>
 */

#ifndef WEBGL_RENDER_H
#define WEBGL_RENDER_H

namespace argos {
   class CWebGLRender;
}

#include <map>
#include <argos3/core/simulator/visualization/visualization.h>
#include <argos3/core/simulator/entity/entity.h>
#include "websocket_server.h"

namespace argos {

   class CWebglUpdateInfo: public CEntityOperation<CWebglUpdateInfo, CWebGLRender, void> {
   public:

      virtual ~CWebglUpdateInfo(){};
   };

   class CWebglSpawnInfo: public CEntityOperation<CWebglSpawnInfo, CWebGLRender, void> {
   public:

      virtual ~CWebglSpawnInfo(){};
   };
   
#define REGISTER_WEBGL_ENTITY_OPERATION(ACTION, OPERATION, ENTITY)   \
   REGISTER_ENTITY_OPERATION(ACTION, CWebGLRender, OPERATION, void, ENTITY);

   class CWebGLRender : public CVisualization {

   public:

      CWebGLRender() :
         m_cSimulator(CSimulator::GetInstance()),
         m_strBind("localhost"),
         m_unPort(8000),
         m_bInteractive(false),
         m_bStartBrowser(false) {}

      virtual ~CWebGLRender() {}

      virtual void Init(TConfigurationNode& t_tree);

      virtual void Execute();

      virtual void Reset();

      virtual void Destroy();

      virtual void SendPosition(CComposableEntity& c_entity);
      virtual void SendPosition(CComposableEntity& c_entity, CByteArray c_data);
      virtual void SendSpawn(CByteArray c_Data, CComposableEntity& c_entity);

      
   private:
      CSimulator& m_cSimulator;
      typedef UInt32 networkId_t;

      std::string m_strBind;
      std::string m_strStatic;
      std::map<std::string, networkId_t> m_mapNetworkId;
      UInt16 m_unPort;
      bool m_bInteractive;
      bool m_bStartBrowser;
      CWebsocketServer* m_pcServer;

      


   };
   // enumeration problem 
   namespace EMessageType {
         const UInt8 SPAWN=0u;
         const UInt8 UPDATE=1u;
      }

}

#endif
