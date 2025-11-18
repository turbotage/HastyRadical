module;

export module mult_test;

import tests;

constexpr std::vector<std::vector<i32>> all_permutations(i32 n) {
	std::vector<i32> base(n);
	std::iota(base.begin(), base.end(), 0); // Fill with [0, 1, ..., n-1]

	std::vector<std::vector<i32>> result;

	do {
		result.push_back(base);
	} while (std::next_permutation(base.begin(), base.end()));
	
	return result;
}

std::vector<std::vector<std::vector<i32>>> global_perms = {
	all_permutations(1), all_permutations(2), 
	all_permutations(3), all_permutations(4)
};

export struct MultResult {
    bool success;
    std::vector<i32> perm;
    i32 inversion_bitmap;
    i32 num_mult;
};

export template<integral I>
MultResult check_one_mult(
    const std::vector<std::array<I,4>>& factors,
) 
{
    int num_mult = factors.size();
    auto perms = global_perms[num_mult];

    for (auto& p : perms) {
        for (i32 gi = 0; gi < (1 << num_mult); ++gi) {
            std::array<I, 4> totest{}; // Group identity
            // We create the product
            for (i32 pos = 0; pos < num_mult; ++pos) {
                bool should_invert = (1 << pos) & gi;

                auto multip = cast_matrix<I>(factors[p[pos]]);
                if (should_invert) {
                    group_inversion_(multip, _n);
                }

                totest = group_multiplication(totest, multip, _n);

                if (pos < num_mult - 1 && ((1 << (pos+1)) & gi) && pos > 0) {
                    // Intermediate check
                    if (check_element(totest, _n).success) {
                        return { true, p, gi, num_mult };
                    }
                }
            }

            // The product is produced, check it
            if (check_element(totest, _n).success) {
                return { true, p, gi, num_mult };
            }
        }
    }

    return { false, {}, 0, 0 };
}