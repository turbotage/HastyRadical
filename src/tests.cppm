module;

export module tests;

import std;
import radlib;
import threadpool;
import containers;

import mult_test;

export struct GeneratorsState {

    GeneratorsState(
        const std::vector<std::array<i64,4>>& gens,
        const std::unordered_set<i32>& succ,
        const std::unordered_set<i32>& rem,
        i32 n_val,
        ThreadPool& thread_pool
    )
        : generators(gens),
          successful(succ),
          remaining(rem),
          n(n_val),
          tp(thread_pool)
    {}

    const std::vector<std::array<i64,4>>& generators;
    const std::unordered_set<i32>& successful;
    const std::unordered_set<i32>& remaining;
    i32 n;
    ThreadPool& tp;
};

export struct InitialSuccessSolution {
    bool success;
    i32 initial_successful_genidx;
};

export InitialSuccessSolution is_initial_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
) {
    for (i32 midx = 0; midx < class_members.size(); ++midx) {
        for (i32 succ : gen_state.successful) {
            if (class_members[midx] == succ) {
                return {true, class_members[midx]};
            }
        }
    }
    return {false, -1};
}

export struct AkSuccessSolution {
    bool success;
    i32 ak_successful_genidx;
    i32 k_value;
};

export AkSuccessSolution is_Ak_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
) {
    i32 n = gen_state.n;
    std::vector<i32> test_k_vals = {1, n, n*n+1, 2*n*n};
    for (i32 kidx = 0; kidx < test_k_vals.size()-1; ++kidx) {
        for (i32 midx = 0; midx < class_members.size(); ++midx) {
            auto mat = cast_matrix<i128>(gen_state.generators[class_members[midx]]);
            auto ret = check_element_Ak(mat, n, test_k_vals[kidx], test_k_vals[kidx+1]);
            if (ret.info != SuccessStateAk::SuccessType::NONE) {
                return {true, class_members[midx], ret.k_value};
            }
        }
    }
    return {false, -1, -1};
}

export struct SeqSuccessSolution {
    bool success = false;
    i32 seq_successful_genidx;
    i32 k_value;
};

export SeqSuccessSolution is_sequence_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
) {
    for (const auto& member : class_members) {
        auto mat = cast_matrix<i128>(gen_state.generators[member]);
        auto res = check_element_sequence(mat, gen_state.n);
        if (res.info != SuccessStateSeq::SuccessType::NONE) {
            return {true, member, res.k};
        }
    }
    return {false, -1, -1};
}

export struct MultAndAkSuccessSolution 
{
    i32 mult_successful_genidx;
    i32 multiplier1_genidx;
    i32 multiplier2_genidx;
    i32 k_value;
    MultResult mult_result;
};

export MultAndAkSuccessSolution is_mult_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
) 
{
    auto member_checker = [&class_members, &gen_state](i32 mat1_idx, i32 mat2_idx)
        -> MultAndAkSuccessSolution
    {
        std::vector<std::array<i128,4>> factors(4);
        std::vector<bool> factors_are_k = {false, false, false, true};

        factors[1] = cast_matrix<i128>(gen_state.generators[mat1_idx]);
        factors[2] = cast_matrix<i128>(gen_state.generators[mat2_idx]);

        for (i32 midx : class_members) {

            factors[0] = cast_matrix<i128>(gen_state.generators[midx]);

            for (i32 k = 0; k < gen_state.n+1; ++k) {
                factors[3][0] = k;
                factors[3][1] = -k*k;
                factors[3][2] = 1;
                factors[3][3] = -k;

                auto result = check_one_mult<i128>(
                                        factors, 
                                        factors_are_k, 
                                        gen_state.n
                                    );
                if (result.success) {
                    return MultAndAkSuccessSolution{
                        midx, mat1_idx, mat2_idx, k, result
                    };
                }
            }

        }
        return MultAndAkSuccessSolution{
            -1, -1, -1, -1, MultResult{}
        };
    };

    MultAndAkSuccessSolution result;

    i32 succ_size = gen_state.successful.size();
    std::deque<std::future<MultAndAkSuccessSolution>> futures;
    // iterate over all permutations of 2 successful generators
    for (auto it1 = gen_state.successful.begin(); it1 != gen_state.successful.end(); ++it1) {
    for (auto it2 = it1; it2 != gen_state.successful.end(); ++it2) {
        i32 mat1_idx = *it1;
        i32 mat2_idx = *it2;

        futures.push_back(gen_state.tp.Enqueue(
            member_checker, mat1_idx, mat2_idx
        ));
        if (futures.size() >= 8) {
            result = futures.front().get();
            futures.pop_front();
            if (result.mult_result.success) {
                break;
            }
        }
    }}
    while (!futures.empty()) {
        if (!result.mult_result.success) {
            result = futures.front().get();
        } else {
            futures.front().get();
        }
        futures.pop_front();
    }
    return result;
}

export std::vector<i32> gens_in_successful_mult(
    const MultAndAkSuccessSolution& mult_solution
)
{
    auto& mult_result = mult_solution.mult_result;
    auto& perm = mult_result.perm.get();
    std::array<i32,4> factors = {
        mult_solution.mult_successful_genidx,
        mult_solution.multiplier1_genidx,
        mult_solution.multiplier2_genidx,
        -1
    };
    std::vector<i32> result;
    for (i32 i = 0; i < 4; ++i) {
        i32 possible_factor = perm[factors[i]];
        if ((possible_factor != -1) && (i < mult_result.num_mult)) {
            result.push_back(possible_factor);
        }
    }
    return result;
}

export struct SuccessState {

    enum class SuccessType : u8 {
        NONE = 0,
        SUCCESS_BY_INITIAL_TEST = 1,
        SUCCESS_BY_EQUIVALENCE = 2,
        SUCCESS_BY_AK_TEST = 3,
        SUCCESS_BY_SEQUENCE_TEST = 4,
        SUCCESS_BY_MULT_TEST = 5
    };

	SuccessType success_type;
    i32 success_parent_genidx;
	std::optional<InitialSuccessSolution> initial_success_solution;
	std::optional<AkSuccessSolution> ak_success_solution;
	std::optional<SeqSuccessSolution> seq_success_solution;
	std::optional<MultAndAkSuccessSolution> mult_success_solution;
};

export SuccessState is_non_mult_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
)
{
    InitialSuccessSolution result = is_initial_successful(
        class_members, 
        gen_state
    );
    if (result.success) {
        return {
            SuccessState::SuccessType::SUCCESS_BY_INITIAL_TEST, 
            result.initial_successful_genidx,
            result, {}, {}, {}
        };
    }

    AkSuccessSolution ak_result = is_Ak_successful(
        class_members, 
        gen_state
    );
    if (ak_result.success) {
        return {
            SuccessState::SuccessType::SUCCESS_BY_AK_TEST,
            ak_result.ak_successful_genidx,
            {}, ak_result, {}, {}
        };
    }

    SeqSuccessSolution seq_result = is_sequence_successful(
        class_members, 
        gen_state
    );
    if (seq_result.success) {
        return {
            SuccessState::SuccessType::SUCCESS_BY_SEQUENCE_TEST,
            seq_result.seq_successful_genidx,
            {}, {}, seq_result, {}
        };
    }

    return {SuccessState::SuccessType::NONE, -1, {}, {}, {}, {}};
}


