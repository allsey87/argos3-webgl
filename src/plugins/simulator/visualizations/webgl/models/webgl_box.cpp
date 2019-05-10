#include "webgl_box.h"
#include <argos3/plugins/simulator/entities/box_entity.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/datatypes/byte_array.h>

namespace argos {
const UInt16 BOX = 0;

    CWebGLBox::CWebGLBox() {

    }

    void CWebGLBox::UpdateInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity) {
        c_visualization.SendPosition(c_entity);
    }

    void CWebGLBox::SpawnInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity){
        CByteArray cData;
        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        auto sSize = c_entity.GetSize();
        cData << EMessageType::SPAWN
              << BOX
              << sSize.GetX() 
              << sSize.GetY()
              << sSize.GetZ();
        c_visualization.SendSpawn(cData, c_entity);
    }

    class CWebglBoxUpdateInfo: public CWebglUpdateInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CBoxEntity& c_entity) {
            static CWebGLBox m_cModel;
            m_cModel.UpdateInfo(c_visualization, c_entity);
        }
    };

    class CWebglBoxSpawnInfo: public CWebglSpawnInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CBoxEntity& c_entity) {
            static CWebGLBox m_cModel;
            m_cModel.SpawnInfo(c_visualization, c_entity);
        }
    };

    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfo, CWebglBoxUpdateInfo, CBoxEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglSpawnInfo, CWebglBoxSpawnInfo, CBoxEntity);
}