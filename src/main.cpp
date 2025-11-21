
import std;
import radlib;
import threadpool;
import tests;
import test_class;
import containers;

void run_gamma_test() {
    std::println("Hello, Hasty Radical!\n");


    auto run_gamma = [](int n) {
        auto gens = load_group_generators_tilde<i64>(n);
        i32 num_gens = gens.size();
        std::println("Loaded {} generators for Gamma({})", num_gens, n);


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

        tgn.run_non_mult_class_tests();
        std::println("Successful generators after non mult check: {}", tgn.get_successful_generators().size());

        MultType mult_type = MultType::MULT1;
        i32 new_successful_size = tgn.run_mult_class_tests(MultType::MULT1);
        //std::println("Successful generators after first MULT1 check: {}", tgn.get_successful_generators().size());
        while (num_gens != tgn.get_successful_generators().size()) {
            if (new_successful_size == 0 && (mult_type == MultType::MULT1)) {
                std::println("Switching to MULT2", n);
                mult_type = MultType::MULT2;
                new_successful_size = tgn.run_mult_class_tests(MultType::MULT2);
                //std::println("Successful generators after MULT2 check: {}", tgn.get_successful_generators().size());
            } else if (new_successful_size == 0 && (mult_type == MultType::MULT2)) {
                std::println("Switching to MULT2_AK", n);
                mult_type = MultType::MULT2_AK;
                new_successful_size = tgn.run_mult_class_tests(MultType::MULT2_AK);
                //std::println("Successful generators after MULT2_AK check: {}", tgn.get_successful_generators().size());
            } else if (new_successful_size == 0 && (mult_type == MultType::MULT2_AK)) {
                throw std::runtime_error("No new successful generators found in last mult type, stopping.");
            } else {
                mult_type = MultType::MULT1;
                new_successful_size = tgn.run_mult_class_tests(MultType::MULT1);
                //std::println("Successful generators after MULT1 check: {}", tgn.get_successful_generators().size());
            }
        }

        std::println("Successful generators after mult check: {}", tgn.get_successful_generators().size());
        std::println("Finished Gamma({})", n);
        std::println("-------------------------------------------------");
        std::println("");

    };

    for (int n = 90; n <= 100; ++n) {
        run_gamma(n);
    }
    //run_gamma(50);

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