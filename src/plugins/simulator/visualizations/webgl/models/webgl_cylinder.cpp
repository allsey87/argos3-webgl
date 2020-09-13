#include "webgl_cylinder.h"
#include <argos3/plugins/simulator/entities/cylinder_entity.h>
#include <argos3/core/utility/datatypes/byte_array.h>

namespace argos {
const UInt16 CYLINDER = 1;

    CWebGLCylinder::CWebGLCylinder(){}

    void CWebGLCylinder::UpdateInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {

        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        std::pair<CVector3, CQuaternion>& ref = m_mapTransforms[c_entity.GetId()];

        if (ref.first != cBodyPosition || !(ref.second == cBodyOrientation)) {
            CByteArray* pcData = new CByteArray();
            *pcData << EMessageType::UPDATE << c_visualization.getNetworkId(c_entity.GetId());
            ref.first = cBodyPosition;
            ref.second = cBodyOrientation;
            WriteCord(*pcData, cBodyPosition, cBodyOrientation);
            c_visualization.SendUpdates(pcData, c_entity);
        }
    }

    void CWebGLCylinder::UpdateInfoJSON(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {

        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        std::pair<CVector3, CQuaternion>& ref = m_mapTransforms[c_entity.GetId()];

        if (ref.first != cBodyPosition || !(ref.second == cBodyOrientation)) {
            // Write json
            std::ostringstream cJsonStream;
            cJsonStream << R"""({"messageType": "update")""";
            cJsonStream << ",\"id\":" << c_visualization.getNetworkId(c_entity.GetId()) << ',';
            WriteCord(cJsonStream, cBodyPosition, cBodyOrientation);
            cJsonStream << '}';

            // update new position in m_mapTransforms
            ref.first = cBodyPosition;
            ref.second = cBodyOrientation;

            // put json in bytearray
            CByteArray* pcData = new CByteArray();
            (*pcData) << cJsonStream.str(); // using EscapeChar may be necessary
            pcData->Resize(pcData->Size() - 1);
            c_visualization.SendUpdatesText(pcData, c_entity);
        }
    }

    /**
     * { "messageType": "spawn", "type": "cylinder", "scale" [1, 1, 1],
     * "position": [1, 1, 1], "rotation": [1, 1, 1] }
     **/
    void CWebGLCylinder::SpawnInfo(CWebGLRender& c_visualization, CCylinderEntity& c_entity) {
        std::ostringstream cJsonStream;
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
        c_visualization.SendSpawn(pcData, c_entity);
    }

    class CWebglCylinderUpdateInfo: public CWebglUpdateInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CCylinderEntity& c_entity) {
            static CWebGLCylinder m_cModel;
            m_cModel.UpdateInfo(c_visualization, c_entity);
        }
    };

    class CWebglCylinderUpdateInfoText: public CWebglUpdateInfoText {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CCylinderEntity& c_entity) {
            static CWebGLCylinder m_cModel;
            m_cModel.UpdateInfoJSON(c_visualization, c_entity);
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

    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfoText, CWebglCylinderUpdateInfoText, CCylinderEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfo, CWebglCylinderUpdateInfo, CCylinderEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglSpawnInfo, CWebglCylinderSpawnInfo, CCylinderEntity);
}