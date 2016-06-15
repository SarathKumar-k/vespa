// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#pragma once

#include <vespa/vespalib/stllike/string.h>
#include "socket_handle.h"

namespace vespalib {

/**
 * Wrapper class for low-level TCP/IP socket addresses.
 **/
class SocketAddress
{
private:
    socklen_t        _size;
    sockaddr_storage _addr;

    const sockaddr *addr() const { return reinterpret_cast<const sockaddr *>(&_addr); }
    explicit SocketAddress(const sockaddr *addr_in, socklen_t addrlen_in);
public:
    SocketAddress() { memset(this, 0, sizeof(SocketAddress)); }
    SocketAddress(const SocketAddress &rhs) { memcpy(this, &rhs, sizeof(SocketAddress)); }
    SocketAddress &operator=(const SocketAddress &rhs) {
        memcpy(this, &rhs, sizeof(SocketAddress));
        return *this;
    }
    bool valid() const { return (_size >= sizeof(sa_family_t)); }
    bool is_ipv4() const { return (valid() && (_addr.ss_family == AF_INET)); }
    bool is_ipv6() const { return (valid() && (_addr.ss_family == AF_INET6)); }
    int port() const;
    vespalib::string ip_address() const;
    vespalib::string spec() const;
    SocketHandle connect() const;
    SocketHandle listen(int backlog = 500) const;
    static SocketAddress address_of(int sockfd);
    static SocketAddress peer_address(int sockfd);
    static std::vector<SocketAddress> resolve(int port, const char *node = nullptr);
    static SocketAddress select_local(int port, const char *node = nullptr);
    static SocketAddress select_remote(int port, const char *node = nullptr);
    template <typename SELECTOR>
    static SocketAddress select(const SELECTOR &replace, int port, const char *node = nullptr) {
        auto list = resolve(port, node);
        if (!list.empty()) {
            size_t best = 0;
            for (size_t i = 1; i < list.size(); ++i) {
                if (replace(list[best], list[i])) {
                    best = i;
                }
            }
            return list[best];
        }
        return SocketAddress();
    }
};

} // namespace vespalib
