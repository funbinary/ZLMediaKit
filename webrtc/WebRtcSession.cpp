﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "WebRtcSession.h"
#include "Util/util.h"

WebRtcSession::WebRtcSession(const Socket::Ptr &sock) : UdpSession(sock) {
    socklen_t addr_len = sizeof(_peer_addr);
    getpeername(sock->rawFD(), &_peer_addr, &addr_len);
    InfoP(this);
}

WebRtcSession::~WebRtcSession() {
    InfoP(this);
}

static string getUserName(const Buffer::Ptr &buffer) {
    auto buf = buffer->data();
    auto len = buffer->size();
    if (!RTC::StunPacket::IsStun((const uint8_t *) buf, len)) {
        return "";
    }
    std::unique_ptr<RTC::StunPacket> packet(RTC::StunPacket::Parse((const uint8_t *) buf, len));
    if (!packet) {
        return "";
    }
    if (packet->GetClass() != RTC::StunPacket::Class::REQUEST ||
        packet->GetMethod() != RTC::StunPacket::Method::BINDING) {
        return "";
    }
    //收到binding request请求
    auto vec = split(packet->GetUsername(), ":");
    return vec[0];
}

EventPoller::Ptr WebRtcSession::getPoller(const Buffer::Ptr &buffer) {
    auto user_name = getUserName(buffer);
    if (user_name.empty()) {
        return nullptr;
    }
    auto ret = WebRtcTransportImp::getTransport(user_name);
    return ret ? ret->getPoller() : nullptr;
}

void WebRtcSession::onRecv(const Buffer::Ptr &buffer) {
    auto buf = buffer->data();
    auto len = buffer->size();

    if (!_transport) {
        auto user_name = getUserName(buffer);
        if (user_name.empty()) {
            //逻辑分支不太可能走到这里
            WarnL << user_name;
            return;
        }
        _transport = WebRtcTransportImp::getTransport(user_name);
        if (!_transport) {
            //逻辑分支不太可能走到这里
            WarnL << user_name;
            return;
        }
        _transport->setSession(this);
    }
    _transport->inputSockData(buf, len, &_peer_addr);
}

void WebRtcSession::onError(const SockException &err) {
    if (_transport) {
        _transport->unrefSelf(err);
        _transport = nullptr;
    }
}

void WebRtcSession::onManager() {

}