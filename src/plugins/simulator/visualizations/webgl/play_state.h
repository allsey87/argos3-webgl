#ifndef CPLAYSTATE_H
#define CPLAYSTATE_H

#include <mutex>
#include <argos3/core/utility/datatypes/datatypes.h>

namespace argos {
    class CPlayState {
    private:
        bool m_bIsAutomatic;
        UInt32 m_unFramesToPlay;
        std::mutex m_cLock;

    public:
        CPlayState(): m_bIsAutomatic(false), m_unFramesToPlay(0) {}

        void Pause() {
            std::lock_guard<std::mutex> _(m_cLock);
            m_unFramesToPlay = 0;
            m_bIsAutomatic = false;
        }

        void Frame() {
            std::lock_guard<std::mutex> _(m_cLock);
            m_bIsAutomatic = false;
            ++m_unFramesToPlay;
        }

        void Automatic() {
            std::lock_guard<std::mutex> _(m_cLock);
            m_unFramesToPlay = 0;
            m_bIsAutomatic = true;
        }

        bool SimulationShouldAdvance() {
            std::lock_guard<std::mutex> _(m_cLock);
            if (m_bIsAutomatic)
                return true;
            if (m_unFramesToPlay == 0)
                return false;
            --m_unFramesToPlay;
            return true;
        }

        bool isPlaying() {
            return m_bIsAutomatic;
        }
    };
} // namespace argos

#endif