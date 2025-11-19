
import std;
import radlib;
import threadpool;
import test;
import containers;

constexpr int num_threads = 32;
constexpr int generators_per_thread = 32;

ThreadPool pool(num_threads);

void run_gamma_test() {
        std::println("Hello, Hasty Radical!");

    int maxn = 50;

    auto run_gamma = [](int n) {
        auto gens = load_group_generators<i64>(n);
        std::println("Loaded {} generators for Gamma({})", gens.size(), n);

        auto tgn = TestGammaN(std::move(gens), n, pool, generators_per_thread, num_threads);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        tgn.run_initial_check();

        tgn.build_initial_equiv_classes();

        auto classes = tgn.get_equiv_classes_with_bool();
        
        std::println("Found {} equivalence classes after initial check for Gamma({})", classes.size(), n);
        std::println("Successful generators after initial check: {}", tgn.get_successful_generators().size());

        tgn.run_non_mult_class_tests();

        std::println("Successful generators after non mult check: {}", tgn.get_successful_generators().size());

        tgn.run_mult_class_tests();

        std::println("Successful generators after mult check: {}", tgn.get_successful_generators().size());

    };

    run_gamma(100);

    /*
    std::vector<std::future<void>> futures;
    for (int n = 35; n < maxn; ++n) {
        futures.emplace_back(pool.Enqueue(run_gamma, n));
        if (futures.size() > num_threads / 4) {
            futures.front().get();
            futures.erase(futures.begin());
        }
    }
    for (auto& fut : futures)
    fut.get();
    
    std::println("Congratulations! You have computed Gamma(n) for n = 1 to {}", maxn-1);
    */
    
}



int main() {

    run_gamma_test();
    return 0;
}