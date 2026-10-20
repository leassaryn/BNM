#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"

/**
 * @brief Il2Cpp structure headers.
 *
 * This file selects the correct `Il2CppHeaders/<version>.h` for the
 * `UNITY_VER` / `UNITY_PATCH_VER` configured in `UserSettings/GlobalSettings.hpp`.
 *
 * @details
 * Configure the game's full Unity ENGINE version in GlobalSettings.hpp, not its
 * IL2CPP metadata version. Example: 2022.3.50f1 -> UNITY_VER 223, UNITY_PATCH_VER 50.
 * The patch is the number after the second dot; f1/f2/f3 is not part of the settings.
 * See GlobalSettings.hpp for the setup examples and older-version exceptions.
 *
 * This selector uses representative headers, not one include branch per filename.
 * For the bundled Unity 2022+ files, every per-patch file is byte-for-byte identical
 * to the representative selected for that patch. Thus 223 / 50 intentionally includes
 * `2022.3.33f1.h`, whose contents match `2022.3.50f1.h`. Keep the actual patch (50),
 * not the threshold (33); no manual include change is needed.
 *
 * Every distinct bundled header is represented by a valid version branch below.
 * An unlisted filename therefore does not mean that version is omitted: its contents
 * are already selected through a representative. Adding another branch for identical
 * contents would not extend coverage. A new, different header needs its own mapping
 * and a review of BNM code that accesses the changed structures.
 *
 * Notable differences between bundled headers (not all changes alter structure size):
 * - 2022.1.8: adds the kIl2CppMetadataUsageFieldRva enum value.
 * - 2022.2.9: adds the Il2CppAndroidUpStateFunc typedef.
 * - 2022.3.8 and 2023.1.10: Il2CppInternalThread::static_data changes from void** to void*.
 * - 2022.3.33 and 6000.0.10: metadata 29 -> 31; adds
 *   Il2CppMethodDefinition::returnParameterToken. These are separate branch thresholds.
 * - 6000.0.44: adds Il2CppMethodDebugInfo::line.
 * - 6000.0.57 and 6000.2.2: adds Il2CppGenericMethodKey.
 * - 6000.3.0: metadata 31 -> 39; Il2CppGlobalMetadataHeader uses Il2CppSectionMetadata.
 * - 6000.3.3: introduces Il2CppClass_InitDataUnion and Il2CppClass::init_data
 *   in place of the direct rgctx_data member; BNM also handles this in ClassesManagement.cpp.
 * - 6000.5.0: metadata 39 -> 106; Il2CppClass moves events/properties/nestedTypes
 *   and declares them as const void*. BNM accesses them through helpers in src/private/Internals.hpp.
 * - 6000.5.1: changes METADATA_VERSION from 106 to 107; the rest of the bundled file is identical.
 *
 * Easily confused single-header branches:
 * - All bundled 2023.2 patches (0-22) match 2023.2.22f1.h.
 * - All bundled 6000.1 patches (0-17) match 6000.1.17f1.h.
 * - All bundled 6000.4 patches (0-12) match 6000.4.12f1.h.
 * Those filenames are representatives, not minimum supported patches.
 * Ranges here refer to files present in this checkout, not every possible release number.
 *
 * Unity < 2022 uses the older mappings below. Unity 2020.3.0-19 requires code 202,
 * while 2020.3.20+ requires 203. Unity 2021.1 uses code 211 and the actual patch:
 * `2021.1.h` switches internally at patch 24, although the included filename is unchanged.
 *
 * The thresholds below describe the current selector. They do not validate whether
 * a patch exists or guarantee compatibility with future/unbundled patches. Compare new
 * headers before extending support; equal metadata versions alone do not prove equal layouts.
 * Header selection and successful compilation establish declaration compatibility only.
 * Runtime support also requires checking BNM's field accesses, runtime symbol/hook resolution,
 * and behavior in a target game (including custom class creation when enabled).
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
// Legacy mapping for Unity 2017.2-2017.4; the filename is not a minimum version.
#include "Il2CppHeaders/2017.4.h"
#elif UNITY_VER == 181
#include "Il2CppHeaders/2018.1.h"
#elif UNITY_VER == 182
#include "Il2CppHeaders/2018.2.h"
#elif UNITY_VER > 182 && UNITY_VER < 185
// Legacy mapping for Unity 2018.3-2018.4.
#include "Il2CppHeaders/2018.4.h"
#elif UNITY_VER > 190 && UNITY_VER < 193
// Legacy mapping for Unity 2019.1-2019.2.
#include "Il2CppHeaders/2019.2.h"
#elif UNITY_VER == 193
#include "Il2CppHeaders/2019.3.h"
#elif UNITY_VER == 194
#include "Il2CppHeaders/2019.4.h"
#elif UNITY_VER == 201
#include "Il2CppHeaders/2020.1.h"
#elif UNITY_VER == 202
// Also used for Unity 2020.3.0-19: select code 202 explicitly.
#include "Il2CppHeaders/2020.2.h"
#elif UNITY_VER == 203
// Unity 2020.3.20+: select code 203 explicitly; patch alone does not select this file.
#include "Il2CppHeaders/2020.3.h"
#elif UNITY_VER == 211
// Same filename, but internal #if conditions use the actual patch (<24 / >=24).
#include "Il2CppHeaders/2021.1.h"
#elif UNITY_VER == 212
#include "Il2CppHeaders/2021.2.h"
#elif UNITY_VER == 213
#include "Il2CppHeaders/2021.3.h"
#elif UNITY_VER == 221
#if UNITY_PATCH_VER >= 8
// Patch 8 adds kIl2CppMetadataUsageFieldRva; bundled patches 8-25 share this file.
#include "Il2CppHeaders/2022.1.8f1.h"
#else
#include "Il2CppHeaders/2022.1.0f1.h"
#endif
#elif UNITY_VER == 222
#if UNITY_PATCH_VER >= 9
// Patch 9 adds Il2CppAndroidUpStateFunc; bundled patches 9-23 share this file.
#include "Il2CppHeaders/2022.2.9f1.h"
#else
#include "Il2CppHeaders/2022.2.0f1.h"
#endif
#elif UNITY_VER == 223
#if UNITY_PATCH_VER >= 33
// Metadata 31 + returnParameterToken. Bundled patches 33-76 match, including patch 50.
#include "Il2CppHeaders/2022.3.33f1.h"
#elif UNITY_PATCH_VER >= 8
// Patches 8-32: Il2CppInternalThread::static_data is void* instead of void**.
#include "Il2CppHeaders/2022.3.8f1.h"
#else
#include "Il2CppHeaders/2022.3.0f1.h"
#endif
#elif UNITY_VER == 231
#if UNITY_PATCH_VER >= 10
// Patch 10 changes static_data to void*; bundled patches 10-22 share this file.
#include "Il2CppHeaders/2023.1.10f1.h"
#else
#include "Il2CppHeaders/2023.1.0f1.h"
#endif
#elif UNITY_VER == 232
// All bundled 2023.2 patches match this file; patch 22 is not a minimum requirement.
#include "Il2CppHeaders/2023.2.22f1.h"
#elif UNITY_VER == 6000
#if UNITY_PATCH_VER >= 57
// Adds Il2CppGenericMethodKey; bundled patches 57-79 share this file.
#include "Il2CppHeaders/6000.0.57f1.h"
#elif UNITY_PATCH_VER >= 44
// Adds Il2CppMethodDebugInfo::line; patches 44-56 share this file.
#include "Il2CppHeaders/6000.0.44f1.h"
#elif UNITY_PATCH_VER >= 10
// Metadata 31 + returnParameterToken; patches 10-43 share this file.
#include "Il2CppHeaders/6000.0.10f1.h"
#else
#include "Il2CppHeaders/6000.0.0f1.h"
#endif
#elif UNITY_VER == 6001
// All bundled patches 0-17 match; patch 17 is not a minimum requirement.
#include "Il2CppHeaders/6000.1.17f1.h"
#elif UNITY_VER == 6002
#if UNITY_PATCH_VER >= 2
// Adds Il2CppGenericMethodKey; bundled patches 2-15 share this file.
#include "Il2CppHeaders/6000.2.2f1.h"
#else
#include "Il2CppHeaders/6000.2.0f1.h"
#endif
#elif UNITY_VER == 6003
#if UNITY_PATCH_VER >= 3
// Il2CppClass uses init_data; bundled patches 3-19 share this file.
#include "Il2CppHeaders/6000.3.3f1.h"
#else
#include "Il2CppHeaders/6000.3.0f1.h"
#endif
#elif UNITY_VER == 6004
// All bundled patches 0-12 match; patch 12 is not a minimum requirement.
#include "Il2CppHeaders/6000.4.12f1.h"
#elif UNITY_VER == 6005
#if UNITY_PATCH_VER >= 1
// METADATA_VERSION becomes 107; bundled patches 1-3 share this file.
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

#pragma clang diagnostic pop
