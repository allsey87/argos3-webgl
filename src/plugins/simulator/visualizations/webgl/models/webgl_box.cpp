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
            // !!! TODO sendposition or here?
            c_visualization.SendUpdates(pcData, c_entity);
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
        std::cout << "box spawn message: " << cJsonStream.str() << std::endl;
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