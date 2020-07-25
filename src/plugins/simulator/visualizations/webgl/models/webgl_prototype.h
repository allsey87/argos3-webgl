#ifndef WEBGL_ENTITY_H
#define WEBGL_ENTITY_H
#include <argos3/plugins/simulator/visualizations/webgl/webgl_render.h>
namespace argos {
   class CWebGLPrototype;
   class CPrototypeEntity;
}

namespace argos {

   class CWebGLPrototype {

   public:

      CWebGLPrototype();

      virtual ~CWebGLPrototype(){};

      virtual void UpdateInfo(CWebGLRender& c_visualization, CPrototypeEntity& c_entity);
      virtual void UpdateInfoJSON(CWebGLRender& c_visualization, CPrototypeEntity& c_entity);
      virtual void SpawnInfo(CWebGLRender& c_visualization, CPrototypeEntity& c_entity);

   private:
      std::map<std::string, std::pair<CVector3, CQuaternion>> m_mapTransforms;
      std::map<std::string, std::vector<std::pair<CVector3, CQuaternion>>> m_mapChildrenTransforms;
   };

}

#endif
