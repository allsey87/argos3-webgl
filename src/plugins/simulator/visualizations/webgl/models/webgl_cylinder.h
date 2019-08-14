#ifndef WEBGL_CYLINDER_H
#define WEBGL_CYLINDER_H
#include <argos3/plugins/simulator/visualizations/webgl/webgl_render.h>
namespace argos {
   class CWebGLCylinder;
   class CCylinderEntity;
}

namespace argos {

   class CWebGLCylinder {

   public:

      CWebGLCylinder();

      virtual ~CWebGLCylinder(){};

      virtual void UpdateInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity);
      virtual void SpawnInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity);

   private:
      std::map<std::string, std::pair<CVector3, CQuaternion>> m_mapTransforms;
   };

}

#endif
