#include "webgl_box.h"
#include <argos3/plugins/simulator/entities/box_entity.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <sstream>

namespace argos {
const UInt16 BOX = 0;

    CWebGLBox::CWebGLBox() {

    }

    void CWebGLBox::UpdateInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity) {


        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        std::pair<CVector3, CQuaternion>& sRef = m_mapTransforms[c_entity.GetId()];

        if (sRef.first != cBodyPosition || !(sRef.second == cBodyOrientation)) {
            sRef.first = cBodyPosition;
            sRef.second = cBodyOrientation;

            CByteArray* pcData = new CByteArray();
            *pcData << EMessageType::UPDATE << c_visualization.getNetworkId(c_entity.GetId());
            WriteCord(*pcData, cBodyPosition, cBodyOrientation);
            c_visualization.SendUpdates(pcData, c_entity);
        }
    }

    void CWebGLBox::UpdateInfoJSON(CWebGLRender& c_visualization, CBoxEntity& c_entity) {
        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        const CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        std::pair<CVector3, CQuaternion>& sRef = m_mapTransforms[c_entity.GetId()];

        if (sRef.first != cBodyPosition || !(sRef.second == cBodyOrientation)) {
            // Write json
            std::ostringstream cJsonStream;
            cJsonStream << R"""({"messageType": "update")""";
            cJsonStream << ",\"id\":" << c_visualization.getNetworkId(c_entity.GetId()) << ',';
            WriteCord(cJsonStream, cBodyPosition, cBodyOrientation);
            cJsonStream << '}';

            // update new position in m_mapTransforms
            sRef.first = cBodyPosition;
            sRef.second = cBodyOrientation;

            // put json in bytearray
            CByteArray* pcData = new CByteArray();
            (*pcData) << cJsonStream.str(); // using EscapeChar may be necessary
            pcData->Resize(pcData->Size() - 1);
            c_visualization.SendUpdatesText(pcData, c_entity);
        }
    }

    /**
     * { "messageType": "spawn", "type": "cube", "scale" [1, 1, 1],
     * "position": [1, 1, 1], "rotation": [1, 1, 1] }
     **/
    void CWebGLBox::SpawnInfo(CWebGLRender& c_visualization, CBoxEntity& c_entity){
        std::ostringstream cJsonStream;
        cJsonStream << R"""({ "messageType": "spawn", "type": "box", "name":")"""
                    << c_entity.GetId() << '"';

        CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
        CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
        const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;
        auto sSize = c_entity.GetSize();

        cJsonStream << ",\"scale\": [";
        cJsonStream << sSize.GetX() << ','
                << sSize.GetY() << ','
                << sSize.GetZ() << "],";

        WriteCord(cJsonStream, cBodyPosition, cBodyOrientation);
        cJsonStream << '}';
        m_mapTransforms[c_entity.GetId()] = std::pair<CVector3, CQuaternion>(cBodyPosition, cBodyOrientation);

        CByteArray* pcData = new CByteArray();
        (*pcData) << cJsonStream.str();
        pcData->Resize(pcData->Size() - 1);
        c_visualization.SendSpawn(pcData, c_entity);
    }

    class CWebglBoxUpdateInfo: public CWebglUpdateInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CBoxEntity& c_entity) {
            static CWebGLBox m_cModel;
            m_cModel.UpdateInfo(c_visualization, c_entity);
        }
    };

    class CWebglBoxUpdateInfoText: public CWebglUpdateInfoText {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CBoxEntity& c_entity) {
            static CWebGLBox m_cModel;
            m_cModel.UpdateInfoJSON(c_visualization, c_entity);
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
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfoText, CWebglBoxUpdateInfoText, CBoxEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglSpawnInfo, CWebglBoxSpawnInfo, CBoxEntity);
}