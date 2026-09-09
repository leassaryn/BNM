#pragma once

/**
 * @brief Il2Cpp structure headers.
 *
 * This file selects the correct `Il2CppHeaders/<version>.h` for the
 * `UNITY_VER` / `UNITY_PATCH_VER` configured in `UserSettings/GlobalSettings.hpp`.
 *
 * @details
 * - Unity < 2022: one header per minor version (e.g. `2017.4.h`). Every released
 *   f build of the minor uses the same runtime layout.
 * - Unity >= 2022 and Unity 6 (6000.x): full per-patch headers (e.g. `2022.3.33f1.h`,
 *   `6000.0.57f1.h`). `UNITY_PATCH_VER` picks the layout that matches the game's
 *   patch, using the thresholds in the table below.
 * - `Il2CppApi/<version>.h` (same names) holds the il2cpp function list of each
 *   version and is provided for reference only - it is never included automatically.
 *
 * | UNITY_VER | `UNITY_PATCH_VER`        | Included header        |
 * |-----------|--------------------------|------------------------|
 * | 221       | < 8 / >= 8               | 2022.1.0f1.h / 2022.1.8f1.h  |
 * | 222       | < 9 / >= 9               | 2022.2.0f1.h / 2022.2.9f1.h  |
 * | 223       | < 8 / 8-32 / >= 33       | 2022.3.0f1.h / 2022.3.8f1.h / 2022.3.33f1.h |
 * | 231       | < 10 / >= 10             | 2023.1.0f1.h / 2023.1.10f1.h |
 * | 232       | any                      | 2023.2.22f1.h                  |
 * | 6000      | < 10 / 10-43 / 44-56 / >= 57 | 6000.0.0f1.h / 6000.0.10f1.h / 6000.0.44f1.h / 6000.0.57f1.h |
 * | 6001      | any                      | 6000.1.17f1.h                  |
 * | 6002      | < 2 / >= 2               | 6000.2.0f1.h / 6000.2.2f1.h   |
 * | 6003      | < 3 / >= 3               | 6000.3.0f1.h / 6000.3.3f1.h   |
 * | 6004      | any                      | 6000.4.12f1.h                  |
 * | 6005      | < 1 / >= 1               | 6000.5.0f1.h / 6000.5.1f1.h   |
 *
 * The full supported-version table (including Unity < 2022) is documented in
 * `UserSettings/GlobalSettings.hpp`.
 */
namespace BNM {
    /// @cond
    namespace IL2CPP {
#if UNITY_VER == 56
#include "Il2CppHeaders/5.6.4f1.h"
#elif UNITY_VER == 171
#include "Il2CppHeaders/2017.1.h"
#elif UNITY_VER > 171 && UNITY_VER < 181
#include "Il2CppHeaders/2017.4.h"
#elif UNITY_VER == 181
#include "Il2CppHeaders/2018.1.h"
#elif UNITY_VER == 182
#include "Il2CppHeaders/2018.2.h"
#elif UNITY_VER > 182 && UNITY_VER < 185
#include "Il2CppHeaders/2018.4.h"
#elif UNITY_VER > 190 && UNITY_VER < 193
#include "Il2CppHeaders/2019.2.h"
#elif UNITY_VER == 193
#include "Il2CppHeaders/2019.3.h"
#elif UNITY_VER == 194
#include "Il2CppHeaders/2019.4.h"
#elif UNITY_VER == 201
#include "Il2CppHeaders/2020.1.h"
#elif UNITY_VER == 202
#include "Il2CppHeaders/2020.2.h"
#elif UNITY_VER == 203
#include "Il2CppHeaders/2020.3.h"
#elif UNITY_VER == 211
#include "Il2CppHeaders/2021.1.h"
#elif UNITY_VER == 212
#include "Il2CppHeaders/2021.2.h"
#elif UNITY_VER == 213
#include "Il2CppHeaders/2021.3.h"
#elif UNITY_VER == 221
#if UNITY_PATCH_VER >= 8
#include "Il2CppHeaders/2022.1.8f1.h"
#else
#include "Il2CppHeaders/2022.1.0f1.h"
#endif
#elif UNITY_VER == 222
#if UNITY_PATCH_VER >= 9
#include "Il2CppHeaders/2022.2.9f1.h"
#else
#include "Il2CppHeaders/2022.2.0f1.h"
#endif
#elif UNITY_VER == 223
#if UNITY_PATCH_VER >= 33
#include "Il2CppHeaders/2022.3.33f1.h"
#elif UNITY_PATCH_VER >= 8
#include "Il2CppHeaders/2022.3.8f1.h"
#else
#include "Il2CppHeaders/2022.3.0f1.h"
#endif
#elif UNITY_VER == 231
#if UNITY_PATCH_VER >= 10
#include "Il2CppHeaders/2023.1.10f1.h"
#else
#include "Il2CppHeaders/2023.1.0f1.h"
#endif
#elif UNITY_VER == 232
#include "Il2CppHeaders/2023.2.22f1.h"
#elif UNITY_VER == 6000
#if UNITY_PATCH_VER >= 57
#include "Il2CppHeaders/6000.0.57f1.h"
#elif UNITY_PATCH_VER >= 44
#include "Il2CppHeaders/6000.0.44f1.h"
#elif UNITY_PATCH_VER >= 10
#include "Il2CppHeaders/6000.0.10f1.h"
#else
#include "Il2CppHeaders/6000.0.0f1.h"
#endif
#elif UNITY_VER == 6001
#include "Il2CppHeaders/6000.1.17f1.h"
#elif UNITY_VER == 6002
#if UNITY_PATCH_VER >= 2
#include "Il2CppHeaders/6000.2.2f1.h"
#else
#include "Il2CppHeaders/6000.2.0f1.h"
#endif
#elif UNITY_VER == 6003
#if UNITY_PATCH_VER >= 3
#include "Il2CppHeaders/6000.3.3f1.h"
#else
#include "Il2CppHeaders/6000.3.0f1.h"
#endif
#elif UNITY_VER == 6004
#include "Il2CppHeaders/6000.4.12f1.h"
#elif UNITY_VER == 6005
#if UNITY_PATCH_VER >= 1
#include "Il2CppHeaders/6000.5.1f1.h"
#else
#include "Il2CppHeaders/6000.5.0f1.h"
#endif
#else
#include "Il2CppHeaders/2023.1.22f1.h"
static_assert(false, "Setup UNITY_VER in GlobalSettings.hpp");
#endif
    }
    /// @endcond

    /**
        @brief Redefinition of Il2CppReflectionType for shorter code
    */
    using MonoType = IL2CPP::Il2CppReflectionType;
}
