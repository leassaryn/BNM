#pragma once

#include <string>

#include "Vector3.hpp"
#include "Vector2.hpp"

namespace BNM::Structures::Unity {
    struct RaycastHit {
        Vector3 point{}, normal{};
        unsigned int faceID{};
        float distance{};
        Vector2 UV{};
#if UNITY_VER > 174
        int m_Collider{};
#else
        void *m_Collider{};
#endif
        void *GetCollider() const;

        inline std::string str() const {
            std::string result = std::string(BNM_OBFUSCATE("Point: ")) + point.str() + std::string(BNM_OBFUSCATE(", Normal: ")) + normal.str() +
                std::string(BNM_OBFUSCATE(", FaceID: ")) + std::to_string(faceID) + std::string(BNM_OBFUSCATE(", Distance: ")) + std::to_string(distance) +
                std::string(BNM_OBFUSCATE(", UV: ")) + UV.str() + std::string(BNM_OBFUSCATE(", m_Collider: "));
#if UNITY_VER > 174
            result += std::to_string(m_Collider);
#else
            result += std::to_string(reinterpret_cast<BNM::BNM_PTR>(m_Collider));
#endif
            return result;
        }
    };
}
