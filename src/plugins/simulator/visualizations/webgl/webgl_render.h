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

#include <argos3/core/simulator/visualization/visualization.h>

namespace argos {

   class CWebGLRender : public CVisualization {

   public:

      CWebGLRender() :
         m_cSimulator(CSimulator::GetInstance()),
         m_strBind("127.0.0.1"),
         m_unPort(8000),
         m_bInteractive(false),
         m_bStartBrowser(false) {}

      virtual ~CWebGLRender() {}

      virtual void Init(TConfigurationNode& t_tree);

      virtual void Execute();

      virtual void Reset();

      virtual void Destroy();

   private:
      CSimulator& m_cSimulator;

      std::string m_strBind;
      UInt16 m_unPort;
      bool m_bInteractive;
      bool m_bStartBrowser;



   };

}

#endif
