#pragma once
#include "utility/message_queue.h"
#include "utility/helpers.h"

namespace beam {

/// Inter-thread bridge template
template <typename Interface> class Bridge : public Interface {
public:
    /// Functions with captured args go into the queue
    using BridgeMessage = std::function<void(Interface& receiver)>;

    /// Sets up the channel
    Bridge(Interface& _forwardTo, const io::Reactor::Ptr& _reactor) :
        receiver(_forwardTo),
        rx(_reactor, BIND_THIS_MEMFN(on_rx)),
        tx(rx.get_tx())
    {}

    /// Shutdowns receiver side of channel
    void stop_rx() {
        rx.close();
    }

// Use in derived classes to implement Interface's functions like this:
// E.g. to implement
// struct ISomeInterface { ...
//      virtual void send_req_1(uint64_t to, Req1&& r)=0;
//      ... };
// do this in proxy class derived from Bridge<IsomeInterface>:
// struct SomeInterfaceBridge { ...
//      BRIDGE_FORWARD_IMPL(ISomeInterface, send_req_1, Req1); ... }
#define BRIDGE_FORWARD_IMPL(Interface, Func, MessageObject) \
    void Func(uint64_t to, MessageObject&& r) { \
        tx.send( \
            [to, r{std::move(r)} ](Interface& receiver) mutable { \
                receiver.Func(to, std::move(r)); \
            } \
        ); \
    }

protected:
    Interface& receiver;
    RX<BridgeMessage> rx;
    TX<BridgeMessage> tx;

private:
    void on_rx(BridgeMessage&& msg) {
        msg(receiver);
    }
};

} //namespace
