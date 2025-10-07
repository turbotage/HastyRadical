
import std;
import radlib;

constexpr int num_threads = 16;

ThreadPool pool(num_threads);

bool check_combination(const std::array<i128,4>& y, i32 last_added_n, std::vector<u32>& totest) -> bool {
    int multipliers = totest.size();

    std::array<i128, 4> totest{}; // Group identity

    // We shall have tested all positions of y in the multiplication
    for (i32 ypos = 0; ypos < multipliers; ++ypos) {
        // We shall have tested all combinations of inversions of the multiplying generators
        for (int gi = 0; gi < multipliers; ++gi) {

            // This creates the product
            for (i = 0; i < multipliers+1; ++i) {

                if (i == ypos) {
                    totest = group_multiplication(totest, y, n);
                    continue;
                }

                bool should_invert = (1 << i) & ginv_perm;

                auto multip = generators[totest[i]];
                if (should_invert) {
                    multip = group_inversion(multip, n);
                }

                totest = group_multiplication(totest, multip, n);
            }

            // The product is produced, check it
            if (check_element(totest)) {
                return true;
            }

        }
    }

}

template<typename Func>
void for_each_combination_with_repetition(size_t k, size_t n, Func callback) {
    if (n == 0) return;
    
    std::vector<u32> indices(n, 0); // Start with [0, 0, 0, ...]
    
    while (true) {
        callback(indices);
        
        // Find the rightmost position that can be incremented
        int pos = static_cast<int>(n) - 1;
        while (pos >= 0 && indices[pos] == k - 1) {
            pos--;
        }
        
        if (pos < 0) break; // All combinations generated
        
        // Increment this position and reset all positions to its right
        u32 new_value = indices[pos] + 1;
        for (u32 i = pos; i < n; ++i) {
            indices[i] = new_value;
        }
    }
}

void compute_gamma_n(const std::vector<std::array<i64,4>>& generators, int n) {

    std::vector<i32> successful;
    std::vector<i32> remaining;
    successful.reserve(generators.size());
    remaining.reserve(generators.size());
    for (i32 i = 0; i < generators.size(); ++i) {
        remaining.push_back(i);
    }

    auto mhm1 = []()


    auto compute_gen_perms = [](u32 index, u32 mult_factor, u32 last_added_n) -> bool {
        for (u32 i = last_added_n; i < successful.size(); ++i) {

        }
    };

    while (remaining.size() > 0) {

    }

}


int main() {
    std::println("Hello, Hasty Radical!");

    int maxn = 100;
    std::vector<std::array<i64,4>> gen1 = std::async(
        std::launch::async, 
        load_group_generators<i64>, 
        1
    );
    for (int i = 1; i < maxn; ++i) {
        if (i+1 < maxn) {
            auto gen2 = std::async(
                std::launch::async, 
                load_group_generators<i64>, 
                i+1
            );
        }
        if (i % 2 == 0) {
            computen(gen2.get(), i);
        } else {
            computen(gen1.get(), i);
        }
    }

    for (const auto& gen : generators) {
        std::println("Generator:");
        for (i32 i = 0; i < 2; ++i) {
            std::println("  {} {}", gen[i*2], gen[i*2 + 1]);
        }
    }

    for (int layer = 0; true; ++layer) {
        std::println("Layer {}", layer);
        for (const auto& gen : generators) {
            std::println("Generator:");
            for (i32 i = 0; i < 2; ++i) {
                std::println("  {} {}", gen[i*2], gen[i*2 + 1]);
            }
        }
        if (layer == 2) break; // Just to limit output for this example
    }

    return 0;
}