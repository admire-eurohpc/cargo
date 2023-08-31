/******************************************************************************
 * Copyright 2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of mochi-rpc-server-cxx.
 *
 * mochi-rpc-server-cxx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mochi-rpc-server-cxx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mochi-rpc-server-cxx.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef SIGNAL_LISTENER_HPP
#define SIGNAL_LISTENER_HPP

#include <asio.hpp>
#include <asio/signal_set.hpp>

namespace {
template <class F, class... Args>
void
do_for(F f, Args... args) {
    [[maybe_unused]] int x[] = {(f(args), 0)...};
}
} // namespace

struct signal_listener {

    using fork_event = asio::execution_context::fork_event;

    using SignalHandlerType = std::function<void(int)>;

    signal_listener() : m_ios(), m_signals(m_ios) {}

    ~signal_listener() {
        m_signals.cancel();
        m_ios.stop();
        if(m_thread.joinable()) {
            m_thread.join();
        }
    }

    template <typename SignalHandlerType, typename... Args>
    void
    set_handler(SignalHandlerType&& handler, Args... signums) {

        m_user_handler = std::forward<SignalHandlerType>(handler);
        m_signals.clear();

        ::do_for([&](int signum) { m_signals.add(signum); }, signums...);
    }

    void
    clear_handler() {
        m_user_handler = nullptr;
        m_signals.clear();
    }

    void
    do_accept() {

        if(m_user_handler) {
            m_signals.async_wait(std::bind( // NOLINT
                    &signal_listener::signal_handler, this,
                    std::placeholders::_1, std::placeholders::_2));
        }
    }

    void
    run() {
        m_thread = std::thread([&]() {
            do_accept();
            m_ios.run();
        });
    }

    void
    notify_fork(signal_listener::fork_event event) {
        m_ios.notify_fork(event);
    }

    void
    stop() {
        m_ios.stop();
    }

private:
    void
    signal_handler(asio::error_code ec, int signal_number) {
        // a signal occurred, invoke installed handler
        if(!ec) {
            m_user_handler(signal_number);

            // reinstall handler
            m_signals.async_wait(
                    std::bind(&signal_listener::signal_handler, this, // NOLINT
                              std::placeholders::_1, std::placeholders::_2));
        }
    }


    asio::io_service m_ios;
    asio::signal_set m_signals;
    std::thread m_thread;
    SignalHandlerType m_user_handler;
};

#endif // SIGNAL_LISTENER_HPP
