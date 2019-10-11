#include "webgl_prototype.h"
#include <argos3/core/simulator/entity/composable_entity.h>
#include <argos3/core/simulator/entity/embodied_entity.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_entity.h>
#include <argos3/plugins/robots/prototype/simulator/prototype_link_equipped_entity.h>
#include <argos3/core/wrappers/lua/lua_controller.h>

namespace argos {
const UInt16 PROTOTYPE = 3;

CWebGLPrototype::CWebGLPrototype() {

    }

void CWebGLPrototype::UpdateInfo(CWebGLRender &c_visualization,
                                 CPrototypeEntity &c_entity) {
    CByteArray* pcData = new CByteArray();
    *pcData << EMessageType::UPDATE
          << c_visualization.getNetworkId(c_entity.GetId());

    bool bShouldSend(false);
    CEmbodiedEntity &cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
    const CVector3 &cBodyPosition = cBody.GetOriginAnchor().Position;
    const CQuaternion &cBodyOrientation = cBody.GetOriginAnchor().Orientation;
    std::pair<CVector3, CQuaternion> &ref = m_mapTransforms[c_entity.GetId()];

    if (ref.first != cBodyPosition || !(ref.second == cBodyOrientation)) {
        LOG << "OLD POSITION" << ref.first << std::endl;
        ref.first = cBodyPosition;
        LOG << "NEW POSITION" << ref.first << std::endl;
        ref.second = cBodyOrientation;
        bShouldSend = true;
        *pcData << (UInt8)1;
        WriteCord(*pcData, cBodyPosition, cBodyOrientation);
    } else {
        *pcData << (UInt8)0;
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

        std::pair<CVector3, CQuaternion> & cChildPair = vecTransforms[childId];
        if ( cChildPair.first != cPosition || !( cChildPair.second == cBodyOrientation)) {
             cChildPair.first = cPosition;
             cChildPair.second = cBodyOrientation;
            bShouldSend = true;
            *pcData << childId;
            WriteCord(*pcData, cPosition, cOrientation);
        }
    }

    if (bShouldSend)
        c_visualization.SendUpdates(pcData);
}

void CWebGLPrototype::SpawnInfo(CWebGLRender &c_visualization,
                                CPrototypeEntity &c_entity) {
    std::stringstream cJsonStream;
    cJsonStream << R"""({ "messageType": "spawn", "type": "prototype", "name":")"""
                << c_entity.GetId() << "\",";

    CEmbodiedEntity& cBody = c_entity.GetComponent<CEmbodiedEntity>("body");
    CVector3& cBodyPosition = cBody.GetOriginAnchor().Position;
    const CQuaternion& cBodyOrientation = cBody.GetOriginAnchor().Orientation;

    WriteCord(cJsonStream, cBodyPosition, cBodyOrientation);

    // c_entity.
    auto x = c_entity.GetComponentVector();
    std::for_each(x.begin(), x.end(), [](CEntity* e) {
        std::cout << "Controller check:" << std::endl;
        auto x = dynamic_cast<CControllableEntity*>(e);
        if (x == nullptr) std::cout << " Not a controller" << std::endl;
        else {
            auto cont = dynamic_cast<CLuaController*>(&(x->GetController()));
            cont->GetLuaState();
            if (cont == nullptr) std::cout << " Not a Lua controller" << std::endl;
            std::cout << " A lua controller" << std::endl;
        }
    });

    // Save root entity position
    m_mapTransforms[c_entity.GetId()] =
        std::pair<CVector3, CQuaternion>(cBodyPosition, cBodyOrientation);
    
    /* An array is an ordered sequence of zero or more values. (rfc 7159) */
    cJsonStream << ",\"children\":[";
    /*********************/
    std::vector<std::pair<CVector3, CQuaternion>> &vecChildren =
        m_mapChildrenTransforms[c_entity.GetId()] =
            std::vector<std::pair<CVector3, CQuaternion>>();
    for (CPrototypeLinkEntity *pcLink :
         c_entity.GetLinkEquippedEntity().GetLinks()) {
        cJsonStream << "{\"type\":";
        switch (pcLink->GetGeometry()) {
            case CPrototypeLinkEntity::EGeometry::BOX:
                cJsonStream << "\"box\",";
                break;
            case CPrototypeLinkEntity::EGeometry::CYLINDER:
                cJsonStream << "\"cylinder\",";
                break;
            case CPrototypeLinkEntity::EGeometry::SPHERE:
                cJsonStream << "\"sphere\",";
                break;
        }
        const CVector3 &cPosition = pcLink->GetAnchor().OffsetPosition;
        const CQuaternion &cOrientation = pcLink->GetAnchor().OffsetOrientation;

        WriteCord(cJsonStream, cPosition, cOrientation);
        cJsonStream << ",\"scale\": ["
                    << pcLink->GetExtents().GetX() << ','
                    << pcLink->GetExtents().GetY() << ','
                    << pcLink->GetExtents().GetZ() << "]},";
        // Save child position
        vecChildren.push_back(
            std::pair<CVector3, CQuaternion>(cPosition, cOrientation));
    }
    std::string strJson = cJsonStream.str();
    strJson.pop_back(); // remove leading comma
    strJson.append("]}");
    std::cout << "Prototype spawn message: " << strJson << std::endl;

    CByteArray* pcData = new CByteArray();
    (*pcData) << strJson;
    pcData->Resize(pcData->Size() - 1);
    c_visualization.SendSpawn(std::move(std::unique_ptr<CByteArray>(pcData)), c_entity);
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