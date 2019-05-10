#include "webgl_prototype.h"
#include <argos3/plugins/robots/prototype/simulator/prototype_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_equipped_entity.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/simulator/entity/embodied_entity.h>
#include <argos3/core/simulator/entity/composable_entity.h>
namespace argos {
const UInt16 PROTOTYPE = 3;

    CWebGLPrototype::CWebGLPrototype() {

    }

    void CWebGLPrototype::UpdateInfo(CWebGLRender& c_visualization, CPrototypeEntity& c_entity) {
        CByteArray cData;
        for(CPrototypeLinkEntity* pcLink : c_entity.GetLinkEquippedEntity().GetLinks()){
            /* Get the position of the link */
            const CVector3& cPosition = pcLink->GetAnchor().OffsetPosition;
            /* Get the orientation of the link */
            const CQuaternion& cOrientation = pcLink->GetAnchor().OffsetOrientation;
            CRadians cZAngle, cYAngle, cXAngle;
            cOrientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
            cData << cPosition.GetX()
                << cPosition.GetY()
                << cPosition.GetZ()
                << cXAngle.GetValue()
                << cYAngle.GetValue()
                << cZAngle.GetValue();
        }
        c_visualization.SendPosition(c_entity, cData);
    }

    void CWebGLPrototype::SpawnInfo(CWebGLRender& c_visualization, CPrototypeEntity& c_entity){
        // c_entity. 
        CByteArray cData;
        cData << EMessageType::SPAWN
              << PROTOTYPE;
        for(CPrototypeLinkEntity* pcLink : c_entity.GetLinkEquippedEntity().GetLinks()){
           switch(pcLink->GetGeometry()) {
            case CPrototypeLinkEntity::EGeometry::BOX:
                cData << (UInt8)0;
                break;
            case CPrototypeLinkEntity::EGeometry::CYLINDER:
                cData << (UInt8)1;
                break;
            case CPrototypeLinkEntity::EGeometry::SPHERE:
                cData << (UInt8)2;
                break;
         }
         cData << pcLink->GetExtents().GetX()
               << pcLink->GetExtents().GetY()
               << pcLink->GetExtents().GetZ();
       }
       c_visualization.SendSpawn(cData, c_entity);
    }

    class CWebGLPrototypeUpdateInfo: public CWebglUpdateInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CPrototypeEntity& c_entity) {
            static CWebGLPrototype m_cModel;
            m_cModel.UpdateInfo(c_visualization, c_entity);
        }
    };

    class CWebGLPrototypeSpawnInfo: public CWebglSpawnInfo {
    public:
        void ApplyTo(CWebGLRender& c_visualization,
                   CPrototypeEntity& c_entity) {
            static CWebGLPrototype m_cModel;
            m_cModel.SpawnInfo(c_visualization, c_entity);
        }
    };

    REGISTER_WEBGL_ENTITY_OPERATION(CWebglUpdateInfo, CWebGLPrototypeUpdateInfo, CPrototypeEntity);
    REGISTER_WEBGL_ENTITY_OPERATION(CWebglSpawnInfo, CWebGLPrototypeSpawnInfo, CPrototypeEntity);
}