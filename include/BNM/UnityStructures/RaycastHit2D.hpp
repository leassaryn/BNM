#pragma once

#include <string>

#include "Vector2.hpp"

namespace BNM::Structures::Unity {
    struct RaycastHit2D {
        Vector2 centroid, point, normal;
        float distance, fraction;
#if UNITY_VER > 174
        int m_Collider{};
#else
        void *m_Collider{};
#endif
        void *GetCollider() const;

        inline std::string str() const {
            std::string result = std::string(BNM_OBFUSCATE("centroid: ")) + centroid.str() + std::string(BNM_OBFUSCATE(", point: ")) + point.str() +
                std::string(BNM_OBFUSCATE(", normal: ")) + normal.str() + std::string(BNM_OBFUSCATE(", Distance: ")) + std::to_string(distance) +
                std::string(BNM_OBFUSCATE(", fraction: ")) + std::to_string(fraction) + std::string(BNM_OBFUSCATE(", m_Collider: "));
#if UNITY_VER > 174
            result += std::to_string(m_Collider);
#else
            result += std::to_string(reinterpret_cast<BNM::BNM_PTR>(m_Collider));
#endif
            return result;
        }
    };
}