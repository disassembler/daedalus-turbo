/* This file is part of Daedalus Turbo project: https://github.com/sierkov/daedalus-turbo/
 * Copyright (c) 2022-2023 Alex Sierkov (alex dot sierkov at gmail dot com)
 * This code is distributed under the license specified in:
 * https://github.com/sierkov/daedalus-turbo/blob/main/LICENSE */
#ifndef DAEDALUS_TURBO_PROGRESS_HPP
#define DAEDALUS_TURBO_PROGRESS_HPP

#include <map>
#include <optional>
#include <dt/format.hpp>
#include <dt/mutex.hpp>

namespace daedalus_turbo {
    struct progress {
        using state_map = std::map<std::string, std::string>;

        struct info {
            size_t total = 0;
            size_t active = 0;
            size_t completed = 0;
            size_t failed = 0;
        };

        static progress &get()
        {
            static std::optional<progress> p {};
            if (!p) {
                alignas(mutex::padding) static std::mutex m {};
                std::scoped_lock lk { m };
                // checking again since another thread could have already started initializing the logger
                if (!p)
                    p.emplace();
            }
            return *p;
        }

        void update(const std::string &name, const std::string &value)
        {
            logger::trace("progress {}: {}", name, value);
            std::scoped_lock lk { _state_mutex };
            _state[name] = value;
        }

        void update(const state_map &updates)
        {
            std::scoped_lock lk { _state_mutex };
            for (const auto &[name, value]: updates) {
                logger::trace("progress {}: {}", name, value);
                _state[name] = value;
            }
        }

        void update(const state_map &updates, const state_map &retiring)
        {
            std::scoped_lock lk { _state_mutex };
            for (const auto &[name, value]: retiring)
                _state.erase(name);
            for (const auto &[name, value]: updates) {
                logger::trace("progress {}: {}", name, value);
                _state[name] = value;
            }
        }

        void retire(const std::string &name)
        {
            std::scoped_lock lk { _state_mutex };
            _state.erase(name);
        }

        void retire(const state_map &retiring)
        {
            std::scoped_lock lk { _state_mutex };
            for (const auto &[name, value]: retiring)
                _state.erase(name);
        }

        void inform(std::ostream &stream=std::cerr)
        {
            std::string str {};
            {
                std::scoped_lock lk { _state_mutex };
                for (const auto &[name, val]: _state)
                    str += fmt::format("{}: [{}] ", name, val);
            }
            // adjust for the invisible whitespace
            if (str.size() > _max_str)
                _max_str = str.size();
            stream << fmt::format("{:<{}}\r", str, _max_str);
        }

        const state_map copy() const
        {
            state_map state_copy {};
            {
                std::scoped_lock lk { _state_mutex };
                state_copy = _state;
            }
            return state_copy;
        }
    private:
        alignas(mutex::padding) mutable std::mutex _state_mutex {};
        state_map _state {};
        size_t _max_str = 0;
    };

    struct progress_guard {
        progress_guard(const std::initializer_list<std::string> &names): _names { names }, _progress { progress::get() }
        {
            for (const auto &name: _names)
                _progress.update(name, "0.000%");
        }

        ~progress_guard()
        {
            for (const auto &name: _names)
                _progress.retire(name);
        }
    private:
        const std::vector<std::string> _names;
        progress &_progress;
    };
}

#endif // !DAEDALUS_TURBO_PROGRESS_HPP