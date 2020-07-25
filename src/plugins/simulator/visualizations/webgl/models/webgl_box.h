#ifndef WEBGL_BOX_H
#define WEBGL_BOX_H
#include <argos3/plugins/simulator/visualizations/webgl/webgl_render.h>
namespace argos {
   class WebGLBox;
   class CBoxEntity;
}

namespace argos {

   class CWebGLBox {

   public:

      CWebGLBox();

      virtual ~CWebGLBox(){};

      virtual void UpdateInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity);
      virtual void UpdateInfoJSON(CWebGLRender& c_visualization, CBoxEntity& c_entity);
      virtual void SpawnInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity);

   private:
      std::map<std::string, std::pair<CVector3, CQuaternion>> m_mapTransforms;
   };

}

#endif
