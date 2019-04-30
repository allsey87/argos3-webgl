#include "webgl_cylinder.h"
#include <argos3/plugins/simulator/entities/cylinder_entity.h>
#include <argos3/core/utility/datatypes/byte_array.h>

namespace argos {
const UInt16 CYLINDER = 1;

    CWebGLCylinder::CWebGLCylinder(){}

    void CWebGLCylinder::UpdateInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {
        c_visualization.SendPosition(c_entity);
        // LOG << "UPDATE CYL: " << c_entity.GetId() << std::endl;
    }

    void CWebGLCylinder::SpawnInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {
        LOG << "SPAWN CYL: " << c_entity.GetId() << std::endl;
        CByteArray cData;
        cData << EMessageType::SPAWN
              << CYLINDER
              << c_entity.GetRadius()
              << c_entity.GetHeight();
        c_visualization.SendSpawn(cData, c_entity);
    }

    class CWebglCylinderUpdateInfo: public CWebglUpdateInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CCylinderEntity& c_entity) {
            static CWebGLCylinder m_cModel;
            m_cModel.UpdateInfo(c_visualization, c_entity);
        }
    };

    class CWebglCylinderSpawnInfo: public CWebglSpawnInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CCylinderEntity& c_entity) {
            static CWebGLCylinder m_cModel;
            m_cModel.SpawnInfo(c_visualization, c_entity);
        }
    };

    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfo, CWebglCylinderUpdateInfo, CCylinderEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglSpawnInfo, CWebglCylinderSpawnInfo, CCylinderEntity);
}