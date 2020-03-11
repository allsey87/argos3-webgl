#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H

#include <vector>
#include <memory>
#include <mutex>
#include <argos3/core/utility/datatypes/datatypes.h>
#include "message.h"

namespace argos {

    struct SEntry {
        UInt32 m_uVersion;
        std::shared_ptr<SMessage> m_psMessage;
    };


    class CSimulationState {
    private:
        // id is implicit and = index
        std::vector<SEntry> m_vecEntries;
        std::mutex m_cMutex;
    public:
        void UpdateVersion(UInt32 u_id, SMessage* ps_message) {
            std::lock_guard<std::mutex> cSync(m_cMutex);
            m_vecEntries[u_id].m_psMessage = std::shared_ptr<SMessage>(ps_message);
            ++(m_vecEntries[u_id].m_uVersion);
        }

        /**
         * If the version is not newer the shared pointer is null
         * to avoid creating an unused shared_ptr owning the message
         * (std::shared_ptr<T>() is a constexpr)
        */
        SEntry GetLastVersionIfNewer(UInt32 u_id, UInt32 version) {
            std::lock_guard<std::mutex> cSync(m_cMutex);
            SEntry& sEntry = m_vecEntries[u_id];
            UInt32 uVersion = sEntry.m_uVersion;
            if (sEntry.m_uVersion > version) {
                return sEntry;
            }
            return SEntry{uVersion, std::shared_ptr<SMessage>()};
        }

        // 0 should also be the version for the sperssiondata vector
        void AddNewEntry() {
            std::lock_guard<std::mutex> cSync(m_cMutex);
            m_vecEntries.push_back(std::move(SEntry{0, std::shared_ptr<SMessage>(nullptr)}));
        }
    };


}
#endif