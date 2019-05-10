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
      virtual void SpawnInfo(CWebGLRender& c_visualization, CPrototypeEntity& c_entity);

   private:

   };

}

#endif
