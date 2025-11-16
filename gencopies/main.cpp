
import std;
import radlib;
import threadpool;

constexpr int num_threads = 16;
constexpr int generators_per_thread = 32;

ThreadPool pool(num_threads);




bool compute_one(
    const std::array<i64,4>& y, 
    const std::vector<std::array<i64,4>>& generators, 
    const std::vector<i32>& successful,
    const std::vector<i32>& remaining,
    i32 last_successful_size,
    i32 mult_level,
    i32 n
) {
    return for_each_combination_with_repetition(
        successful.size(), mult_level, [&](const std::vector<i32>& indices) {
        // We have 1 multiplier that is always taken from a index >= last_added_n, we don't want to recheck already tested combinations
        // Then we have mult_level multipliers that can be taken from any successful generator
        std::vector<std::array<i128,4>> multipliers(mult_level);
        for (i32 i = last_successful_size; i < successful.size(); ++i) {
            multipliers[0] = cast_matrix<i128>(generators[successful[i]]);
            for (i32 j = 0; j < mult_level-1; ++j) {
                multipliers[j+1] = cast_matrix<i128>(generators[successful[indices[j]]]);
            }
            if (check_combination(cast_matrix<i128>(y), multipliers, n)) {
               return true;
            }
        }
        return false;
    });
}

std::vector<i32> compute_many(
    const std::vector<std::array<i64,4>>& generators, 
    const std::vector<i32>& successful,
    const std::vector<i32>& remaining,
    i32 start_idx,
    i32 last_successful_size,
    i32 mult_level,
    i32 n
) {
    std::vector<i32> local_successful;
    local_successful.reserve(generators_per_thread);
    
    // The thread should handle a batch of size generators_per_thread, or less if at the end
    i32 idxmax = std::min(start_idx + generators_per_thread, (i32)remaining.size());
    for (i32 idx = start_idx; idx < idxmax; ++idx) {
        if (compute_one(
                generators[remaining[idx]], generators, successful, 
                remaining, last_successful_size, mult_level, n)) 
        {
            local_successful.push_back(remaining[idx]);
        }
    }
    return local_successful;
}

i32 move_remaining_to_successful(
    std::vector<i32>& successful,
    std::vector<i32>& remaining,
    std::vector<std::future<std::vector<i32>>>& futures
) {
        std::vector<i32> new_successful;
        new_successful.reserve(futures.size() * generators_per_thread);
        // Collect results from all threads
        for (auto& fut : futures) {
            auto local_successful = fut.get();
            new_successful.insert(
                new_successful.end(), 
                local_successful.begin(), 
                local_successful.end()
            );
        }

        // Merge new successful into successful
        successful.insert(
            successful.end(), 
            new_successful.begin(), 
            new_successful.end()
        );
        
        // Remove new successful from remaining
        remaining.erase(
            std::remove_if(
                remaining.begin(), remaining.end(),
                [&new_successful](i32 idx) {
                    return std::find(new_successful.begin(), new_successful.end(), idx) != new_successful.end();
                }
            ),
            remaining.end()
        );

        return new_successful.size();
}

void compute_gamma_n(const std::vector<std::array<i64,4>>& generators, i32 n) {

    std::vector<i32> successful;
    successful.reserve(generators.size());
    //auto view = std::ranges::iota_view{0, generators.size()};
    std::vector<i32> remaining = std::ranges::to<std::vector<i32>>(std::ranges::iota_view{0uz, generators.size()});

    std::vector<std::future<std::vector<i32>>> futures;
    futures.reserve((remaining.size() + generators_per_thread - 1) / generators_per_thread);

    {
        auto first_caller = [&generators, n](i32 start_idx) {
            std::vector<i32> out;
            for (i32 idx = start_idx; idx < std::min(start_idx + generators_per_thread, (i32)generators.size()); ++idx) {
                if (check_element(cast_matrix<i128>(generators[idx]), n)) {
                    out.push_back(idx);
                }
            }
            return out;
        };

        // Test all remaining generators
        for (i32 i = 0; i < remaining.size(); i += generators_per_thread) {
            // We let each thread handle a batch of size generators_per_thread
            // Add a batch onto the thread pool
            futures.emplace_back(pool.Enqueue(
                first_caller, i
            ));
        }

        i32 new_successful_count = move_remaining_to_successful(successful, remaining, futures);
        if (new_successful_count == 0) {
            throw std::runtime_error("No successful generators found in initial test");
        }
        
        futures.clear();
    }

    i32 last_successful_size = 0;
    i32 mult_level = 1;

    while (remaining.size() > 0) {
        if (mult_level > 2) {
            throw std::runtime_error("Multiplication level exceeded two multiplications");
        }

        // Test all remaining generators
        for (i32 rem = 0; rem < remaining.size(); rem += generators_per_thread) {
            // We let each thread handle a batch of size generators_per_thread
            // Add a batch onto the thread pool
            futures.emplace_back(pool.Enqueue(
                compute_many,
                std::ref(generators),
                std::ref(successful),
                std::ref(remaining),
                rem,
                last_successful_size,
                mult_level,
                n
            ));
        }

        //Set last successful size before we modify successful
        last_successful_size = successful.size();

        i32 new_successful_count = move_remaining_to_successful(successful, remaining, futures);
        // If no new successful found, increase multiplication level
        if (new_successful_count == 0) {
            // No new successful found at this multiplication level, increase level
            mult_level++;
            last_successful_size = 0; // We want to try all successful at the new level
        }

        futures.clear();
    }

    std::println("Gamma({}) succeeded", n);
}


int main() {
    std::println("Hello, Hasty Radical!");

    //std::cout << gcd(10, 0) << std::endl;

    int maxn = 50;

    for (int i = 2; i < maxn; ++i) {
        auto gens = load_group_generators<i64>(i);
        std::println("Loaded {} generators for Gamma({})", gens.size(), i);
        compute_gamma_n(gens, i);
    }
    /*
    std::future<std::vector<std::array<i64,4>>> gen1 = std::async(
        std::launch::async,
        load_group_generators<i64>, 
        1
    );
    std::future<std::vector<std::array<i64,4>>> gen2;
    for (int i = 1; i < maxn; ++i) {
        if (i+1 < maxn) {
            gen2 = std::async(
                std::launch::async, 
                load_group_generators<i64>, 
                i+1
            );
        }
        if (i % 2 == 0) {
            compute_gamma_n(gen2.get(), i);
        } else {
            compute_gamma_n(gen1.get(), i);
        }
    }
    */


    std::println("Congratulations! You have computed Gamma(n) for n = 1 to {}", maxn-1);
    
    return 0;
}