#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H

#include <vector>
#include <memory>
#include <argos3/core/utility/datatypes/datatypes.h>
#include "message.h"

namespace argos {

    struct SEntry {
        UInt32 m_uVersion;
        // This is unnecessarily thread safe
        std::shared_ptr<SMessage> m_psMessage;
    };


    class CSimulationState {
    private:
        // id is implicit and = index
        std::vector<SEntry> m_vecEntries;
    public:
        void UpdateVersion(UInt32 u_id, SMessage* ps_message) {
            m_vecEntries[u_id].m_psMessage = std::shared_ptr<SMessage>(ps_message);
            ++(m_vecEntries[u_id].m_uVersion);
        }

        UInt32 GetLastVersionValue(UInt32 u_id) {
            std::cout << "Last version of " << u_id << " is " << m_vecEntries[u_id].m_uVersion << std::endl;
            return m_vecEntries[u_id].m_uVersion;
        }

        std::shared_ptr<SMessage> GetLastVersionMessage(UInt32 u_id) {
            return m_vecEntries[u_id].m_psMessage;
        }
        // 0 should also be the version for the sperssiondata vector
        void AddNewEntry() {
            m_vecEntries.push_back(std::move(SEntry{0, std::shared_ptr<SMessage>(nullptr)}));
        }
    };


}
#endif