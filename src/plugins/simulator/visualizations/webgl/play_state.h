#ifndef CPLAYSTATE_H
#define CPLAYSTATE_H

#include <argos3/core/utility/datatypes/datatypes.h>

namespace argos {
    class CPlayState {
    private:
        bool m_bIsAutomatic;
        UInt32 m_unFramesToPlay;

    public:
        CPlayState(): m_bIsAutomatic(false), m_unFramesToPlay(0) {}

        void Pause() {
            m_unFramesToPlay = 0;
            m_bIsAutomatic = false;
        }

        void Frame() {
            m_bIsAutomatic = false;
            ++m_unFramesToPlay;
        }

        void Automatic() {
            m_unFramesToPlay = 0;
            m_bIsAutomatic = true;
        }

        bool SimulationShouldAdvance() {
            if (m_bIsAutomatic)
                return true;
            if (m_unFramesToPlay == 0)
                return false;
            --m_unFramesToPlay;
            return true;
        }
    };
} // namespace argos

#endif