#include "webgl_prototype.h"
#include <argos3/core/simulator/entity/composable_entity.h>
#include <argos3/core/simulator/entity/embodied_entity.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_equipped_entity.h>
namespace argos {
const UInt16 PROTOTYPE = 3;

CWebGLPrototype::CWebGLPrototype() {

    }

void CWebGLPrototype::UpdateInfo(CWebGLRender &c_visualization,
                                 CPrototypeEntity &c_entity) {
    CByteArray cData;
    cData << EMessageType::UPDATE
          << c_visualization.getNetworkId(c_entity.GetId());

    bool bShouldSend(false);
    CEmbodiedEntity &cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
    const CVector3 &cBodyPosition = cBody.GetOriginAnchor().Position;
    const CQuaternion &cBodyOrientation = cBody.GetOriginAnchor().Orientation;
    std::pair<CVector3, CQuaternion> &ref = m_mapTransforms[c_entity.GetId()];

    if (ref.first != cBodyPosition || !(ref.second == cBodyOrientation)) {
        ref.first = cBodyPosition;
        ref.second = cBodyOrientation;
        bShouldSend = true;
        cData << (UInt8)1;
        WriteCord(cData, cBodyPosition, cBodyOrientation);
    } else {
        cData << (UInt8)0;
    }

    // TODO check same order guarantee?
    CPrototypeLinkEntity::TVector &vecLinks =
        c_entity.GetLinkEquippedEntity().GetLinks();
    std::vector<std::pair<CVector3, CQuaternion>> &vecTransforms =
        m_mapChildrenTransforms[c_entity.GetId()];
    for (UInt8 childId = 0; childId < vecTransforms.size(); ++childId) {
        CPrototypeLinkEntity *pcLink = vecLinks[childId];
        /* Get the position of the link */
        const CVector3 &cPosition = pcLink->GetAnchor().OffsetPosition;
        /* Get the orientation of the link */
        const CQuaternion &cOrientation = pcLink->GetAnchor().OffsetOrientation;

        std::pair<CVector3, CQuaternion> &ref = vecTransforms[childId];
        if (ref.first != cPosition || !(ref.second == cBodyOrientation)) {
            ref.first = cPosition;
            ref.second = cBodyOrientation;
            bShouldSend = true;
            cData << childId;
            WriteCord(cData, cPosition, cOrientation);
        }
    }

    if (bShouldSend)
        c_visualization.SendUpdates(cData);
}

void CWebGLPrototype::SpawnInfo(CWebGLRender &c_visualization,
                                CPrototypeEntity &c_entity) {
    // c_entity.
    CEmbodiedEntity &cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
    const CVector3 &cBodyPosition = cBody.GetOriginAnchor().Position;
    const CQuaternion &cBodyOrientation = cBody.GetOriginAnchor().Orientation;

    CByteArray cData;
    cData << EMessageType::SPAWN << PROTOTYPE;
    WriteCord(cData, cBodyPosition, cBodyOrientation);

    // Save root entity position
    m_mapTransforms[c_entity.GetId()] =
        std::pair<CVector3, CQuaternion>(cBodyPosition, cBodyOrientation);
    std::vector<std::pair<CVector3, CQuaternion>> &vecChildren =
        m_mapChildrenTransforms[c_entity.GetId()] =
            std::vector<std::pair<CVector3, CQuaternion>>();
    for (CPrototypeLinkEntity *pcLink :
         c_entity.GetLinkEquippedEntity().GetLinks()) {
        switch (pcLink->GetGeometry()) {
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
        const CVector3 &cPosition = pcLink->GetAnchor().OffsetPosition;
        const CQuaternion &cOrientation = pcLink->GetAnchor().OffsetOrientation;

        // write the scale
        cData << pcLink->GetExtents().GetX() // Scale
              << pcLink->GetExtents().GetY() << pcLink->GetExtents().GetZ();
        WriteCord(cData, cPosition, cOrientation);

        // Save child position
        vecChildren.push_back(
            std::pair<CVector3, CQuaternion>(cPosition, cOrientation));
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