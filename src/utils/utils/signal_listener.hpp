/******************************************************************************
 * Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of Cargo.
 *
 * Cargo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cargo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cargo.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef UTILS_SIGNAL_LISTENER_HPP
#define UTILS_SIGNAL_LISTENER_HPP

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>

namespace {
template <class F, class... Args>
void
do_for(F f, Args... args) {
    [[maybe_unused]] int x[] = {(f(args), 0)...};
}
} // namespace

namespace utils {

namespace ba = boost::asio;

struct signal_listener {

    using SignalHandlerType = std::function<void(int)>;

    signal_listener() : m_ios(), m_signals(m_ios) {}

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
        std::thread([&]() {
            do_accept();
            m_ios.run();
        }).detach();
    }

    void
    stop() {
        m_ios.stop();
    }

private:
    void
    signal_handler(boost::system::error_code ec, int signal_number) {
        // a signal occurred, invoke installed handler
        if(!ec) {
            m_user_handler(signal_number);
        }

        // reinstall handler
        m_signals.async_wait(
                std::bind(&signal_listener::signal_handler, this, // NOLINT
                          std::placeholders::_1, std::placeholders::_2));
    }


    ba::io_service m_ios;
    ba::signal_set m_signals;
    SignalHandlerType m_user_handler;
};

} // namespace utils

#endif // UTILS_SIGNAL_LISTENER_HPP
