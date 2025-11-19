
import std;
import radlib;
import threadpool;
import test;
import containers;

void run_gamma_test() {
        std::println("Hello, Hasty Radical!");

    int maxn = 50;

    auto run_gamma = [](int n) {
        auto gens = load_group_generators_tilde<i64>(n);
        std::println("Loaded {} generators for Gamma({})", gens.size(), n);

        auto tgn = TestGammaN(std::move(gens), n);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        tgn.run_initial_check();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - begin;
        std::println("Initial check took {} seconds", elapsed_seconds.count());

        begin = std::chrono::steady_clock::now();

        tgn.build_initial_equiv_classes();

        end = std::chrono::steady_clock::now();
        elapsed_seconds = end - begin;
        std::println("Building initial equivalence classes took {} seconds", elapsed_seconds.count());

        auto classes = tgn.get_equiv_classes_with_bool();
        
        std::println("Found {} equivalence classes after initial check for Gamma({})", classes.size(), n);
        std::println("Successful generators after initial check: {}", tgn.get_successful_generators().size());

        //tgn.run_non_mult_class_tests();

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