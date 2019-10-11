#include "webgl_cylinder.h"
#include <argos3/plugins/simulator/entities/cylinder_entity.h>
#include <argos3/core/utility/datatypes/byte_array.h>

namespace argos {
const UInt16 CYLINDER = 1;

    CWebGLCylinder::CWebGLCylinder(){}

    void CWebGLCylinder::UpdateInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {
        CByteArray* pcData = new CByteArray();
        *pcData << EMessageType::UPDATE << c_visualization.getNetworkId(c_entity.GetId());

        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        std::pair<CVector3, CQuaternion>& ref = m_mapTransforms[c_entity.GetId()];

        if (ref.first != cBodyPosition || !(ref.second == cBodyOrientation)) {
            ref.first = cBodyPosition;
            ref.second = cBodyOrientation;
            WriteCord(*pcData, cBodyPosition, cBodyOrientation);
            c_visualization.SendUpdates(pcData);
        }
    }

    /**
     * { "messageType": "spawn", "type": "cylinder", "scale" [1, 1, 1],
     * "position": [1, 1, 1], "rotation": [1, 1, 1] }
     **/
    void CWebGLCylinder::SpawnInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {
        std::stringstream cJsonStream;
        cJsonStream << R"""({ "messageType": "spawn", "type": "cylinder", "name":")"""
                    << c_entity.GetId() << '"';

        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;

        cJsonStream << ",\"scale\": [";
        cJsonStream << c_entity.GetRadius() << ','
                    << c_entity.GetHeight() << "],";

        WriteCord(cJsonStream, cBodyPosition, cBodyOrientation);
        cJsonStream << '}';
        m_mapTransforms[c_entity.GetId()] = std::pair<CVector3, CQuaternion>(cBodyPosition, cBodyOrientation);

        CByteArray* pcData = new CByteArray();
        (*pcData) << cJsonStream.str();
        pcData->Resize(pcData->Size() - 1);
        std::cout << "cylinder spawn message: " << cJsonStream.str() << std::endl;
        c_visualization.SendSpawn(std::move(std::unique_ptr<CByteArray>(pcData)), c_entity);
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