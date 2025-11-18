module;

export module tests;

import std;
import radlib;
import threadpool;
import containers;

import mult_state;

export struct GeneratorsState {
    const std::vector<std::array<i64,4>>& generators;
    const std::vector<i32>& successful;
    const std::vector<i32>& remaining;
    i32 n;
    ThreadPool& tp;
};

namespace class_tests {

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
    std::vector<i32> test_k_vals = {1, n, n*n+1, 2*n*n};
    for (i32 kidx = 0; kidx < test_k_vals.size()-1; ++kidx) {
        for (i32 midx = 0; midx < class_members.size(); ++midx) {
            auto mat = cast_matrix<i128>(gen_state.generators[class_members[midx]]);
            auto ret = check_element_Ak(mat, n, test_k_vals[kidx], test_k_vals[kidx+1]);
            if (ret.success) {
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
        if (res.success) {
            return {true, member, res.k_value};
        }
    }
    return {false, -1, -1};
}

export struct MultSuccessSolution {
    i32 mult_successful_genidx;
    i32 multiplier_genidx;
    MultResult mult_result;
};

export MultSuccessSolution is_mult_successful(
    const std::vector<i32>& class_members,
    const GeneratorsState& gen_state
) {

    auto member_checker = [&gen_state](i32 member) 
        -> MultSuccessSolution
    {
        MultSuccessSolution result;
        
        auto mat1 = cast_matrix<i128>(gen_state.generators[member]);
        std::vector<std::array<i128,4>> factors(3);
        factors[0] = mat1;
        for (i32 succ : gen_state.successful) {
            factors[1] = cast_matrix<i128>(gen_state.generators[succ]);

            for (i32 k = 0; k < gen_state.n+1; ++k) {
                factors[2][0] = k;
                factors[2][1] = -k*k;
                factors[2][2] = 1;
                factors[2][3] = -k;

                result.mult_result = check_one_mult<i128>(factors);
                if (result.mult_result.success) {
                    result.mult_successful_genidx = member;
                    result.multiplier_genidx = succ;
                    return result;
                }
            }

        }
        return {-1, -1, {}};
    };

    MultSuccessSolution result;

    int members_size = class_members.size();
    std::deque<std::future<bool>> futures;
    for (int midx = 0; midx < members_size; ++midx) {
        futures.push_back(gen_state.tp.Enqueue(
            member_checker, class_members[midx]
        ));
        if (futures.size() >= 8) {
            result = futures.front().get();
            futures.pop_front();
            if (result.mult_result.success) {
                break;
            }
        }
    }
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

}