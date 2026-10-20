#include <BNM/Delegates.hpp>

BNM::MethodBase BNM::DelegateBase::GetMethod() const {
    if (!CheckForNull(this)) return {};
    auto method = MethodBase(this->method);
    auto instance = GetInstance();
    if (instance) method.SetInstance(instance);
    return method;
}

BNM::DelegateBase *BNM::DelegateBase::Create(BNM::MethodBase method) {
    return reinterpret_cast<BNM::DelegateBase *>(BNM::Class(object.klass).CreateNewObjectParameters(method._instance, method._data));
}

std::vector<BNM::MethodBase> BNM::MulticastDelegateBase::GetMethods() const {
    if (!CheckForNull(this)) return {};

    auto delegates = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(this->delegates);
    if (!delegates) {
        DelegateBase self;
        memcpy(&self, this, sizeof(self));
        return {self.GetMethod()};
    }

    std::vector<MethodBase> ret{};
    ret.reserve(delegates->capacity);
    for (IL2CPP::il2cpp_array_size_t i = 0; i < delegates->capacity; ++i) {
        auto d = delegates->At(i);
        if (d) ret.push_back(d->GetMethod());
    }
    return ret;
}

namespace {
    // Clone the single delegate before changing its invocation list. Keep the new
    // allocation's object header and route each managed reference through a barrier.
    template<class D> bool CopyDelegate(D *to, const D *from) {
        constexpr auto header = sizeof(BNM::IL2CPP::Il2CppObject);
        std::memcpy(reinterpret_cast<char *>(to) + header,
                    reinterpret_cast<const char *>(from) + header, sizeof(D) - header);
        bool ok = BNM::StoreManagedValue(&to->object, &to->target, from->target);
        if constexpr (requires { to->method_info; })
            ok = BNM::StoreManagedValue(&to->object, &to->method_info, from->method_info) && ok;
        if constexpr (requires { to->original_method_info; })
            ok = BNM::StoreManagedValue(&to->object, &to->original_method_info, from->original_method_info) && ok;
        if constexpr (requires { to->data; })
            ok = BNM::StoreManagedValue(&to->object, &to->data, from->data) && ok;
        if constexpr (requires { to->invoke_impl_this; })
            ok = BNM::StoreManagedValue(&to->object, &to->invoke_impl_this, from->invoke_impl_this) && ok;
        return ok;
    }
    bool SameDelegate(const BNM::DelegateBase *a, const BNM::DelegateBase *b) {
        return a && b && (a == b || (a->method == b->method && a->target == b->target));
    }
}

bool BNM::MulticastDelegateBase::TryAdd(BNM::DelegateBase *added) {
    if (!CheckForNull(this) || !added || !this->delegate.object.klass) return false;
    auto *previous = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(delegates);
    const auto count = previous ? previous->capacity : (this->delegate.method ? 1 : 0);
    if (count == std::numeric_limits<IL2CPP::il2cpp_array_size_t>::max()) return false;
    auto *arr = Class(this->delegate.object.klass).NewArray<DelegateBase *>(count + 1);
    if (!arr) return false;
    if (previous) {
        if (!arr->CopyFrom(previous->m_Items, count)) return false;
    } else if (count) {
        auto *copy = reinterpret_cast<DelegateBase *>(Class(this->delegate.object.klass).CreateNewInstance());
        if (!copy || !CopyDelegate(static_cast<IL2CPP::Il2CppDelegate *>(copy), &this->delegate) ||
            !arr->Set(0, copy)) return false;
    }
    if (!arr->Set(count, added)) return false;
    return StoreManagedValue(&this->delegate.object, &delegates, reinterpret_cast<decltype(delegates)>(arr));
}

void BNM::MulticastDelegateBase::Add(BNM::DelegateBase *added) { (void)TryAdd(added); }

void BNM::MulticastDelegateBase::Remove(BNM::DelegateBase *delegate)  {
    if (!CheckForNull(this) || !delegate || !this->delegate.object.klass) return;
    auto *previous = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(delegates);
    if (!previous) {
        if (!SameDelegate(reinterpret_cast<DelegateBase *>(&this->delegate), delegate)) return;
        auto *empty = Class(this->delegate.object.klass).NewArray<DelegateBase *>(0);
        if (empty) (void)StoreManagedValue(&this->delegate.object, &delegates, reinterpret_cast<decltype(delegates)>(empty));
        return;
    }
    // Remove the last matching occurrence, including equivalent method/target pairs.
    auto index = previous->capacity;
    while (index && !SameDelegate(previous->m_Items[index - 1], delegate)) --index;
    if (!index) return;
    --index;
    auto *arr = Class(this->delegate.object.klass).NewArray<DelegateBase *>(previous->capacity - 1);
    if (!arr || !arr->CopyRange(previous->m_Items, index) ||
        !arr->CopyRange(previous->m_Items + index + 1, previous->capacity - index - 1, index)) return;
    (void)StoreManagedValue(&this->delegate.object, &delegates, reinterpret_cast<decltype(delegates)>(arr));
}

BNM::DelegateBase *BNM::MulticastDelegateBase::Add(BNM::MethodBase method) {
    if (!CheckForNull(this) || !method.IsValid()) return {};

    auto delegate = reinterpret_cast<BNM::DelegateBase *>(this)->Create(method);

    return TryAdd(delegate) ? delegate : nullptr;
}