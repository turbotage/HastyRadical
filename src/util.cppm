module;

export module util;

import std;


export template<typename T>
using refw = std::reference_wrapper<T>;

export template<typename T>
using crefw = std::reference_wrapper<const T>;

// Waits until any future in the container is ready, pops and returns its value.
// Returns the result directly (blocks until one is ready).
export template <typename FutureContainer>
inline auto pop_when_ready(FutureContainer& futures)
    -> decltype(std::declval<FutureContainer&>().front().get())
{
    using namespace std::chrono_literals;
    while (true) {
        for (auto it = futures.begin(); it != futures.end(); ++it) {
            if (it->wait_for(0s) == std::future_status::ready) {
                auto val = it->get();
                futures.erase(it);
                return val;
            }
        }
        std::this_thread::sleep_for(1ns);
    }
}

export template<typename T>
bool contains_negative_idx(const std::vector<T>& vec) {
    for (const auto& val : vec) {
        if (val < 0) {
            return true;
        }
    }
    return false;
}