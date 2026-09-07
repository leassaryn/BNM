## ByNameModding
#### English | [Русский](https://github.com/ByNameModding/BNM-Android/blob/master/README_RU.md)
ByNameModding is a library for modding of il2cpp games by classes, methods, field names. It includes everything you need for Android Unity games modding.<br>
Requires C++20 minimum.


## Getting Started
See documentation on [ByNameModding.github.io](https://bynamemodding.github.io/).

## Supported Unity versions: 5.6.4f1, 2017.x - 6000.5.x

Version selection is done in `include/BNM/UserSettings/GlobalSettings.hpp`:
uncomment one `UNITY_VER` and, for 2022.x+ / Unity 6 (6000.x) games, set `UNITY_PATCH_VER`
to the game's patch number (e.g. `2022.3.20f1` -> `20`).
The full table of supported ranges, patch boundaries and the included header files
is documented in that file and in `include/BNM/Il2CppHeaders.hpp`.

## Dependencies
[UTF8-CPP](https://github.com/nemtrif/utfcpp) used by il2cpp and by BNM too.<br>
[Open-hierarchy custom RTTI](https://github.com/royvandam/rtti/tree/cf0dee6fb3999573f45b0726a8d5739022e3dacf) used to optimize memory usage
### Android hookinng software for example:
[Dobby](https://github.com/jmpews/Dobby)<br>
[ShadowHook](https://github.com/bytedance/android-inline-hook)<br>
[Substrate](https://github.com/jbro129/Unity-Substrate-Hook-Android/tree/master/C%2B%2B/Substrate) with [And64InlineHook](https://github.com/Rprop/And64InlineHook) - do not support unhook