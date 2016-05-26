#pragma once

#include <algorithm>
#include <vector>

namespace asio {
    template<typename... Args>
    class Slot;

    template<typename... Args>
    class Signal {
        std::vector<Slot<Args...> *> slots;
    public:
        Signal() { }

        Signal(const Signal<Args...> &orig) = delete;

        Signal(Signal<Args...> &&orig) = delete;

        ~Signal() {
            for (Slot<Args...> *s : slots) {
                this->remove(*s);
            }
        }

        Signal<Args...> &operator=(const Signal<Args...> &orig) = delete;

        Signal<Args...> &operator=(Signal<Args...> &&orig) = delete;

        void trigger(Args... args) {
            for (Slot<Args...> *s : slots) {
                s->action()(args...);
            }
        }

        void operator()(Args... args) {
            this->trigger(args...);
        }

        void add(Slot<Args...> &slot) {
            slots.push_back(&slot);
            slot.signals.push_back(this);
        }

        void remove(Slot<Args...> &slot) {
            auto sli = find(slots.begin(), slots.end(), &slot);
            auto sii = find(slot.signals.begin(), slot.signals.end(), this);
            assert(sli != slots.end() && sii != slot.signals.end());

            slots.erase(sli);
            slot.signals.erase(sii);
        }
    };

    template<typename... Args>
    class Slot {
        std::function<void(Args...)> _action;
        std::vector<Signal<Args...> *> signals;

    public:
        Slot() : _action([](Args... args){}) {}
        Slot(std::function<void(Args...)> action) : _action(action) { }

        Slot(const Slot<Args...> &orig) = delete;

        Slot(Slot<Args...> &&orig) = delete;

        ~Slot() {
            for (Signal<Args...> *s : signals) {
                s->remove(*this);
            }
        }

        Slot<Args...> &operator=(const Slot<Args...> &orig) = delete;
        Slot<Args...> &operator=(Slot<Args...> &&orig) = delete;

        void action(std::function<void(Args...)> callback) {
            _action = callback;
        }

        std::function<void(Args...)> action() {
            return _action;
        }

        friend class Signal<Args...>;
    };
}