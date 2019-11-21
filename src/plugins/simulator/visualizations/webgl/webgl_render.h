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
#include <memory>
#include <argos3/core/simulator/visualization/visualization.h>
#include <argos3/core/simulator/entity/entity.h>
#include <argos3/core/utility/math/vector3.h>
#include <argos3/core/utility/math/quaternion.h>
#include "play_state.h"
#include <argos3/core/utility/datatypes/byte_array.h>
#include "controllers_container.h"

namespace argos {

   class CWebsocketServer;

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
      typedef UInt32 networkId_t;

      CWebGLRender() :
         m_cSimulator(CSimulator::GetInstance()),
         m_strBind("localhost"),
         m_unPort(8000),
         m_unPeriod(30),
         m_bInteractive(false),
         m_bStartBrowser(false),
         m_bDirty(true) {}

      virtual ~CWebGLRender() {}

      virtual void Init(TConfigurationNode& t_tree);

      virtual void Execute();

      virtual void Reset();

      virtual void Destroy();

      virtual void SendUpdates(CByteArray* c_entity);
      
      virtual void SendSpawn(std::unique_ptr<CByteArray> c_Data, CComposableEntity& c_entity);

      void RecievedMove(CByteArray& c_Data);

      networkId_t getNetworkId(const std::string& str_id) const {
         return m_mapNetworkId.at(str_id);
      }

      size_t GetRootEntitiesCount() const {
         return m_vecEntites.size();
      }

      void WriteSpawn(CByteArray&, networkId_t);
      
   private:
      CSimulator& m_cSimulator;

      std::string m_strBind;
      std::string m_strStatic;
      std::vector<CComposableEntity*> m_vecEntites;
      std::map<std::string, networkId_t> m_mapNetworkId;
      UInt16 m_unPort;
      UInt16 m_unPeriod;
      bool m_bInteractive;
      bool m_bStartBrowser;
      bool m_bDirty;
      CWebsocketServer* m_pcServer;
      CPlayState m_cPlayState;
      CLuaControllers m_cLuaContainer;
   };

   inline void WriteCord(CByteArray& cData, const CVector3& c_position, const CQuaternion& c_orientation) {
      CRadians cZAngle, cYAngle, cXAngle;
      c_orientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
      cData << c_position.GetX()
            << c_position.GetY()
            << c_position.GetZ()
            << cXAngle.GetValue()
            << cYAngle.GetValue()
            << cZAngle.GetValue();
    }
   
   inline void WriteCord(std::ostringstream& cData, const CVector3& c_position, const CQuaternion& c_orientation) {
      CRadians cZAngle, cYAngle, cXAngle;
      c_orientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
      cData << "\"position\":[" 
            << c_position.GetX() << ','
            << c_position.GetY() << ','
            << c_position.GetZ() << "],"
            << "\"rotation\":["
            << cXAngle.GetValue() << ','
            << cYAngle.GetValue() << ','
            << cZAngle.GetValue() << ']';
    }

   // enumeration problem 
   namespace EMessageType {
         const UInt8 SPAWN=0u;
         const UInt8 UPDATE=1u;
         const UInt8 PLAYSTATE=2u;
      }

}

#endif
