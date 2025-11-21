module;

export module mult_test;

import std;
import radlib;

constexpr std::vector<std::vector<i32>> all_permutations(i32 n) {
	std::vector<i32> base(n);
	std::iota(base.begin(), base.end(), 0); // Fill with [0, 1, ..., n-1]

	std::vector<std::vector<i32>> result;

	do {
		result.push_back(base);
	} while (std::next_permutation(base.begin(), base.end()));
	
	return result;
}

export std::vector<std::vector<std::vector<i32>>> global_perms = {
    all_permutations(0),
	all_permutations(1), 
    all_permutations(2), 
	all_permutations(3), 
    all_permutations(4)
};

export struct MultResult {
    bool success = false;
    std::reference_wrapper<const std::vector<i32>> perm = std::cref(global_perms[0][0]);
    i32 inversion_bitmap = -1;
    i32 num_mult = -1;
};

export template<integral I>
MultResult check_one_mult(
    const std::vector<std::array<I,4>>& factors,
    const std::vector<bool>& factors_are_k,
    i32 n
)
{
    int num_mult = factors.size();
    const auto& perms = global_perms[num_mult];

    if (num_mult >= global_perms.size()) {
        throw std::runtime_error("check_one_mult: number of multipliers exceeds precomputed permutations");
    }
    if (factors.size() != factors_are_k.size()) {
        throw std::runtime_error("check_one_mult: factors and factors_are_k size mismatch");
    }

    for (const auto& p : perms) {
    for (i32 gi = 0; gi < (1 << num_mult); ++gi) {
        // We don't need to try inversion of k-factors
        // So if there are any k-factors that should be inverted
        // We skip this iteration
        bool skip_iteration_due_to_k = false;
        for (i32 kidx = 0; kidx < num_mult; ++kidx) {
            if (factors_are_k[p[kidx]] && ((1 << kidx) & gi)) {
                skip_iteration_due_to_k = true;
                break;
            }
        }
        if (skip_iteration_due_to_k) {
            continue;
        }

        bool first_factor_in_product = false;
        std::array<I, 4> totest{}; // Group identity
        // We create the product
        for (i32 pos = 0; pos < num_mult; ++pos) {
            bool should_invert = (1 << pos) & gi;

            i32 factor_after_perm = p[pos];
            if (factor_after_perm == 0) {
                first_factor_in_product = true;
            }

            auto multip = cast_matrix<I>(factors[factor_after_perm]);
            if (should_invert) {
                group_inversion_(multip, n);
            }

            totest = group_multiplication(totest, multip, n);

            // Intermediate check, we test product smaller than the full product (pos < (num_mult - 1)). 
            // This exact product subproduct will emerge both when the next factor
            // is to be inverted and when it isn't, thus we only test this subproduct
            // when the next factor is to be inverted ((1 << (pos+1)) & gi) to not test twice.
            // Since this is the non equiv mult test. We also only do this if the first factor is in the product
            // Also there has to be at least on multiplication (pos > 0)
            if ((pos < (num_mult - 1)) && ((1 << (pos+1)) & gi) && pos > 0 && first_factor_in_product) {
                // Intermediate check
                if (check_element(totest, n) != CheckElementSuccessType::NONE) {
                    return { true, std::cref(p), gi, pos };
                }
            }
        }

        // The product is produced, check it
        if (check_element(totest, n) != CheckElementSuccessType::NONE) {
            return { true, std::cref(p), gi, num_mult };
        }
    }}

    return { false, std::cref(global_perms[0][0]), 0, 0 };
}

export template<integral I>
MultResult check_one_mult_equiv(
    const std::vector<std::array<I,4>>& factors,
    const std::vector<bool>& factors_are_k,
    i32 n
)
{
    int num_mult = factors.size();
    const auto& perms = global_perms[num_mult];

    if (num_mult >= global_perms.size()) {
        throw std::runtime_error("check_one_mult: number of multipliers exceeds precomputed permutations");
    }
    if (factors.size() != factors_are_k.size()) {
        throw std::runtime_error("check_one_mult: factors and factors_are_k size mismatch");
    }

    for (const auto& p : perms) {
    for (i32 gi = 0; gi < (1 << num_mult); ++gi) {
        // We don't need to try inversion of k-factors
        // So if there are any k-factors that should be inverted
        // We skip this iteration
        bool skip_iteration_due_to_k = false;
        for (i32 kidx = 0; kidx < num_mult; ++kidx) {
            if (factors_are_k[p[kidx]] && ((1 << kidx) & gi)) {
                skip_iteration_due_to_k = true;
                break;
            }
        }
        if (skip_iteration_due_to_k) {
            continue;
        }

        std::array<I, 4> totest{}; // Group identity
        // We create the product
        for (i32 pos = 0; pos < num_mult; ++pos) {
            bool should_invert = (1 << pos) & gi;

            auto multip = cast_matrix<I>(factors[p[pos]]);
            if (should_invert) {
                group_inversion_(multip, n);
            }

            totest = group_multiplication(totest, multip, n);

            if (pos < num_mult - 1 && ((1 << (pos+1)) & gi) && pos > 0) {
                // Intermediate check
                if (check_element(totest, n) != CheckElementSuccessType::NONE) {
                    return { true, std::cref(p), gi, pos };
                }
            }
        }

        // The product is produced, check it
        if (check_element(totest, n) != CheckElementSuccessType::NONE) {
            return { true, std::cref(p), gi, num_mult };
        }
    }}

    return { false, std::cref(global_perms[0][0]), 0, 0 };
}