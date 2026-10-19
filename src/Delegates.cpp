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

void BNM::MulticastDelegateBase::Add(BNM::DelegateBase *delegate)  {
    if (!CheckForNull(this)) return;

    auto delegates = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(this->delegates);
    if (!delegates) return;
    auto arr = BNM::Class(delegates->klass->element_class).NewArray<DelegateBase *>(this->delegates->max_length + 1);
    if (!arr) return;
    if (!arr->CopyFrom(delegates->m_Items, delegates->capacity)) return;
    if (!arr->Set(delegates->capacity, delegate)) return;
    (void)StoreManagedValue(&this->delegate.object, &this->delegates, reinterpret_cast<decltype(this->delegates)>(arr));
}

void BNM::MulticastDelegateBase::Remove(BNM::DelegateBase *delegate)  {
    if (!CheckForNull(this)) return;

    auto delegates = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(this->delegates);
    if (!delegates) return;
    IL2CPP::il2cpp_array_size_t index = 0;
    bool found = false;
    for (IL2CPP::il2cpp_array_size_t i = 0; i < delegates->capacity; ++i) {
        if (delegates->m_Items[i] != delegate) continue;

        found = true;
        index = i;
        break;
    }
    if (!found) return;
    auto arr = BNM::Class(delegates->klass->element_class).NewArray<DelegateBase *>(delegates->capacity - 1);
    if (!arr) return;
    // Copy elements before the removed index
    if (index > 0 && !arr->CopyRange(delegates->m_Items, index)) return;
    // Copy elements after the removed index
    if (index < delegates->capacity - 1 && !arr->CopyRange(delegates->m_Items + index + 1, delegates->capacity - index - 1, index)) return;
    (void)StoreManagedValue(&this->delegate.object, &this->delegates, reinterpret_cast<decltype(this->delegates)>(arr));
}

BNM::DelegateBase *BNM::MulticastDelegateBase::Add(BNM::MethodBase method) {
    if (!CheckForNull(this) || !method.IsValid()) return {};

    auto delegate = reinterpret_cast<BNM::DelegateBase *>(this)->Create(method);

    Add(delegate);

    return delegate;
}