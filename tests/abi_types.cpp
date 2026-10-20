#include <BNM/Defaults.hpp>
#include <cstdint>
using namespace BNM;
static_assert(sizeof(Types::ulong) == 8);
static_assert(Defaults::Get<int64_t>()._reference == &Defaults::Internal::Int64);
static_assert(Defaults::Get<uint64_t>()._reference == &Defaults::Internal::UInt64);
static_assert(Defaults::Get<int32_t>()._reference == &Defaults::Internal::Int32);
static_assert(Defaults::Get<uint32_t>()._reference == &Defaults::Internal::UInt32);
static_assert(Defaults::Get<long long>()._reference == &Defaults::Internal::Int64);
static_assert(Defaults::Get<unsigned long long>()._reference == &Defaults::Internal::UInt64);
static_assert(Defaults::Get<long>()._reference == (sizeof(long) == 8 ? &Defaults::Internal::Int64 : &Defaults::Internal::Int32));
static_assert(Defaults::Get<unsigned long>()._reference == (sizeof(long) == 8 ? &Defaults::Internal::UInt64 : &Defaults::Internal::UInt32));
static_assert(Defaults::Get<Types::nint>()._reference == &Defaults::Internal::IntPtr);
static_assert(Defaults::Get<Types::nuint>()._reference == &Defaults::Internal::UIntPtr);
