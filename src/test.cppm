module;

#include <unordered_map>
export module test;

import std;
import radlib;
import threadpool;
import containers;

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

template<typename Func>
bool for_each_combination_with_repetition(i32 k, i32 n, Func callback) {
	if (n == 0)
		throw std::invalid_argument("n must be greater than 0");
	
	std::vector<i32> indices(n, 0); // Start with [0, 0, 0, ...]
	
	while (true) {
		if (callback(indices)) {
			return true;
		}
		
		// Find the rightmost position that can be incremented
		int pos = static_cast<int>(n) - 1;
		while (pos >= 0 && indices[pos] == k - 1) {
			pos--;
		}
		
		if (pos < 0) break; // All combinations generated
		
		// Increment this position and reset all positions to its right
		i32 new_value = indices[pos] + 1;
		for (i32 i = pos; i < n; ++i) {
			indices[i] = new_value;
		}
	}
	return false;
}


struct CheckResult {
	i64 products_tested = 1;

	i8 inversion_permutation_at_success;
	i8 ypos_at_success;
	std::vector<i32> gens_giving_success;
	std::vector<i32> gens_permutation;
};

export class TestGammaN {
private:
	i32 _n;
	std::vector<std::array<i64,4>> _generators;
	std::vector<i32> _successful;
	std::vector<i32> _remaining;
	std::vector<CheckResult> _check_results;


	UnionFind _equiv_classes;

	std::vector<std::future<std::vector<i32>>> futures;

	i32 _generators_per_thread = 32;
	i32 _number_of_threads = 8;

	ThreadPool& _pool;

	std::vector<i32> _temp1;
	std::vector<i32> _temp2;
public:

	TestGammaN(std::vector<std::array<i64,4>>&& gens, i32 n, ThreadPool& pool, i32 generators_per_thread = 32, i32 number_of_threads = 8)
		: _n(n), 
		_generators(std::move(gens)), 
		_successful(), 
		_remaining(std::ranges::to<std::vector<i32>>(std::ranges::iota_view{0uz, _generators.size()})), 
		_check_results(_generators.size()),
		_generators_per_thread(generators_per_thread),
		_number_of_threads(number_of_threads),
		_pool(pool),
		_equiv_classes(_generators.size())
	{
		_successful.reserve(_remaining.size());
		futures.reserve((_remaining.size() + _generators_per_thread - 1) / _generators_per_thread);
		_temp1.reserve(_remaining.size());
		_temp2.reserve(_remaining.size());
	}

	std::unordered_map<i32, std::vector<i32>> get_equiv_classes() {
		return _equiv_classes.get_classes();
	}

	std::unordered_map<i32, std::pair<std::vector<i32>, bool>> get_equiv_classes_with_bool() {
		return _equiv_classes.get_classes_with_bool();
	}

	const std::vector<i32>& get_successful_generators() const {
		return _successful;
	}

	void run_initial_check()
    {
        auto first_caller = [this](i32 start_idx) {
            std::vector<i32> out;
			i32 maxidx = std::min(start_idx + _generators_per_thread, (i32)_generators.size());
			for (i32 idx = start_idx; idx < maxidx; ++idx) {
                if (check_element(cast_matrix<i128>(_generators[idx]), _n).success) {
                    out.push_back(idx);
                }
            }
            return out;
        };

        // Test all remaining generators
        for (i32 remidx = 0; remidx < _remaining.size(); remidx += _generators_per_thread) {
            // We let each thread handle a batch of size generators_per_thread
            // Add a batch onto the thread pool
            futures.emplace_back(_pool.Enqueue(
                first_caller, remidx
            ));
        }

        i32 new_successful_count = move_remaining_to_successful();
        if (new_successful_count == 0) {
            throw std::runtime_error("No successful generators found in initial test");
        }
        
        futures.clear();
    }

	auto build_initial_equiv_classes() 
		-> std::unordered_map<i32, std::pair<std::vector<i32>, bool>>
	{
		i32 gen_size = _generators.size();

		std::unordered_map<i64, std::vector<i32>> same_map;
		same_map.clear();

		// Same signed x1
		{
			for (i32 i = 0; i < gen_size; ++i) {
				i64 x1 = _generators[i][0];
				same_map[x1].push_back(i);
			}

			for (const auto& [x1, indices] : same_map) {
				i32 indsize = indices.size();
				for (i32 i = 0; i < indsize; ++i) {
					for (i32 j = 0; j < indsize; ++j) {
						if (i == j) {
							continue;
						}
						auto gen1 = _generators[indices[i]];
						auto gen2 = _generators[indices[j]];

						if (divides_radical(abs(_n*x1 + 1), abs(gen1[2] - gen2[2]))) {
							_equiv_classes.unite(indices[i], indices[j]);
						} else if (divides_radical(abs(_n*x1 + 1), abs(gen1[2] - gen2[2]))) {
							_equiv_classes.unite(indices[i], indices[j]);
							std::println("Simon was wrong");
						}
					}
				}
			}
		}
		same_map.clear();

		// Same x3 or x2
		{
			for (i32 i = 0; i < gen_size; ++i) {
				i64 x2 = _generators[i][1];
				i64 x3 = _generators[i][2];
				same_map[abs(x3)].push_back(i);
				same_map[abs(x2)].push_back(i);
			}

			for (const auto& [x2_or_x3, indices] : same_map) {
				i32 indsize = indices.size();
				for (i32 i = 0; i < indsize; ++i) {
					for (i32 j = 0; j < indsize; ++j) {
						if (i == j) continue;
						auto gen1 = _generators[indices[i]];
						auto gen2 = _generators[indices[j]];

						if (divides_radical(abs(x2_or_x3), abs(gen1[0] - gen2[0]))) {
							_equiv_classes.unite(indices[i], indices[j]);
						}

					}
				}
			}
		}
		same_map.clear();

		// Same signed x4
		{
			for (i32 i = 0; i < gen_size; ++i) {
				i64 x4 = _generators[i][3];
				same_map[x4].push_back(i);
			}

			for (const auto& [x4, indices] : same_map) {
				i32 indsize = indices.size();
				for (i32 i = 0; i < indsize; ++i) {
					for (i32 j = 0; j < indsize; ++j) {
						if (i == j) continue;
						auto gen1 = _generators[indices[i]];
						auto gen2 = _generators[indices[j]];

						if (divides_radical(abs(_n*x4 + 1), abs(gen1[2] - gen2[2]))) {
							_equiv_classes.unite(indices[i], indices[j]);
						} else if (divides_radical(abs(_n*x4 + 1), abs(gen1[2] - gen2[2]))) {
							_equiv_classes.unite(indices[i], indices[j]);
							std::println("Simon was wrong");
						}
					}
				}
			}
		}

		std::vector<i32> new_successful;
		new_successful.reserve(_generators.size());

		std::unordered_map<i32, std::pair<std::vector<i32>, bool>> result;
        auto classes = get_equiv_classes();

		result.reserve(classes.size());

        i32 class_counter = 0;
        for (const auto& [rep, members] : classes) {
            class_counter++;
            bool was_successful = false;
            for (i32 midx = 0; midx < members.size(); ++midx) {
                for (i32 succ : _successful) {
                    if (members[midx] == succ) {
                        was_successful = true;
                        std::println("generator class {} had initial successful, class size is {}", rep, members.size());
                        break;
                    }
                }
                if (was_successful) {
					break;
				}
            }

			if (!was_successful) {
				std::vector<i32> test_k_vals = {1, _n, _n*_n+1, 2*_n*_n};
				for (i32 kidx = 0; kidx < test_k_vals.size()-1; ++kidx) {
					for (i32 midx = 0; midx < members.size(); ++midx) {
						auto mat = cast_matrix<i128>(_generators[members[midx]]);
						auto ret = check_element_Ak(mat, _n, test_k_vals[kidx], test_k_vals[kidx+1]);
						if (ret.success) {
							was_successful = true;
							std::println(
								"Gen class {}, successful via Ak, k={}, multype={}, member_idx={}, class size is {}", 
								rep, ret.k_value, ret.info, midx, members.size()
							);
							break;
						}
					}
					if (was_successful) {
						break;
					}
				}
			}

			if (!was_successful) {
				for (const auto& member : members) {
					auto mat = cast_matrix<i128>(_generators[member]);
					auto res = check_element_sequence(mat, _n);
					if (res.success) {
						was_successful = true;
						std::println(
							"Gen class {}, successful via Seq check, k={}, class size is {}", 
							rep, res.k, members.size()
						);
						break;
					}
				}
			}

			if (was_successful) {
				_equiv_classes.set_val(rep, true);
				for (const auto& member : members) {
					new_successful.push_back(member);
				}
			}
        }

		futures.clear();

		std::promise<std::vector<i32>> promise;
		promise.set_value(new_successful);
		futures.push_back(promise.get_future());

		move_remaining_to_successful();

		return result;
	}

	void run_multiplication_checks() 
	{
		i32 last_successful_size = 0;
		i32 mult_level = 1;

		while (_remaining.size() > 0) {
			if (mult_level > 2) {
				throw std::runtime_error("Multiplication level exceeded two multiplications");
			}

			// Test all remaining generators
			for (i32 rem = 0; rem < _remaining.size(); rem += _generators_per_thread) {
				// We let each thread handle a batch of size generators_per_thread
				// Add a batch onto the thread pool
				futures.emplace_back(_pool.Enqueue(
					std::bind(&TestGammaN::compute_many, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
					rem,
					last_successful_size,
					mult_level
				));
			}

			//Set last successful size before we modify successful
			last_successful_size = _successful.size();

			i32 new_successful_count = move_remaining_to_successful();
			// If no new successful found, increase multiplication level
			if (new_successful_count == 0) {
				// No new successful found at this multiplication level, increase level
				mult_level++;
				last_successful_size = 0; // We want to try all successful at the new level
			} else {
				// We found new successful, reset multiplication level
				mult_level = 1;
			}

			futures.clear();
		}

		std::println("Gamma({}) succeeded", _n);
	}

	const std::vector<CheckResult>& get_check_results() const 
	{
		return _check_results;
	}

	const void write_to_file() const {
		auto filename = get_project_file_path("statistics/gamma_" + std::to_string(_n) + "_stat.txt");
		std::ofstream file(filename);
		if (!file.is_open()) {
			throw std::runtime_error("Could not open file for writing: " + filename);
		}
		
		for (const auto& result : _check_results) {
			// Write products_tested
			file << result.products_tested << '\n';
			
			// Write inversion_permutation_at_success
			file << static_cast<int>(result.inversion_permutation_at_success) << '\n';
			
			// Write ypos_at_success
			file << static_cast<int>(result.ypos_at_success) << '\n';
			
			// Write gens_giving_success vector
			file << ':';
			for (i32 i = 0; i < result.gens_giving_success.size(); ++i) {
				if (i > 0) file << ',';
				file << result.gens_giving_success[i];
			}
			file << '\n';
			
			// Write gens_permutation vector
			file << ':';
			for (i32 i = 0; i < result.gens_permutation.size(); ++i) {
				if (i > 0) file << ',';
				file << result.gens_permutation[i];
			}
			file << '\n';
		}
	}

private:

	i32 move_remaining_to_successful() 
	{
		_temp1.clear();
		// Collect results from all threads
		for (auto& fut : futures) {
			auto local_successful = fut.get();
			_temp1.insert(
				_temp1.end(), 
				local_successful.begin(), 
				local_successful.end()
			);
		}

		// Merge new successful into successful
		_successful.insert(
			_successful.end(), 
			_temp1.begin(), 
			_temp1.end()
		);
		// Fix performance later
		std::set<i32> temp_succ(_successful.begin(), _successful.end());
		_successful = std::vector<i32>(temp_succ.begin(), temp_succ.end());

		std::set_difference(
			_remaining.begin(), _remaining.end(),
			_temp1.begin(), _temp1.end(),
			std::back_inserter(_temp2)
		);

		_remaining.swap(_temp2);
		_temp2.clear();


		return _temp1.size();
	}

	std::vector<i32> compute_many(
		i32 start_idx,
		i32 last_successful_size,
		i32 mult_level
	) 
	{
		std::vector<i32> local_successful;
		local_successful.reserve(_generators_per_thread);
		
		// The thread should handle a batch of size generators_per_thread, or less if at the end
		i32 idxmax = std::min(start_idx + _generators_per_thread, (i32)_remaining.size());
		for (i32 idx = start_idx; idx < idxmax; ++idx) {
			// If n < 50 and each entry in the generators is less than n**2, we can use i64
			if (compute_one<i128>(_remaining[idx], last_successful_size, mult_level))
				local_successful.push_back(_remaining[idx]);

		}
		return local_successful;
	}

	template<integral I>
	bool compute_one(
		i32 test_index,
		i32 last_successful_size,
		i32 mult_level
	) {
		std::vector<std::array<I,4>> multipliers(mult_level);
		std::vector<i32> multiplier_indices(mult_level);
		return for_each_combination_with_repetition(
			_successful.size(), mult_level, [&](const std::vector<i32>& indices) {
			// We have 1 multiplier that is always taken from a index >= last_added_n, we don't want to recheck already tested combinations
			// Then we have mult_level multipliers that can be taken from any successful generator
			int mindex;
			for (i32 i = last_successful_size; i < _successful.size(); ++i) {

				mindex = _successful[i];
				multiplier_indices[0] = mindex;
				multipliers[0] = cast_matrix<I>(_generators[mindex]);

				for (i32 j = 0; j < mult_level-1; ++j) {
					mindex = _successful[indices[j]];
					multiplier_indices[j+1] = mindex;
					multipliers[j+1] = cast_matrix<I>(_generators[mindex]);
				}
				if (check_combination(
						cast_matrix<I>(_generators[test_index]), 
						multipliers, 
						multiplier_indices, 
						_check_results[test_index]
					)) 
				{
					return true;
				}
			}
			return false;
		});
	}

	struct CheckMultResult {
		bool success;
	};

	template<integral I>
	CheckMultResult check_one_mult(
		const std::vector<i32>& multiplier_indices
	) 
	{
		int num_mult = multiplier_indices.size();
		auto perms = global_perms[num_mult];

		for (auto& p : perms) {
			for (i32 gi = 0; gi < (1 << num_mult); ++gi) {
				std::array<I, 4> totest{}; // Group identity
				// We create the product
				for (i32 pos = 0; pos < num_mult; ++pos) {
					bool should_invert = (1 << pos) & gi;

					auto multip = cast_matrix<I>(_generators[multiplier_indices[p[pos]]]);
					if (should_invert) {
						group_inversion_(multip, _n);
					}

					totest = group_multiplication(totest, multip, _n);

					if (pos < num_mult - 1 && ((1 << (pos+1)) & gi)) {
						// Intermediate check
						if (check_element(totest, _n).success) {
							return { true };
						}
					}
				}

				// The product is produced, check it
				if (check_element(totest, _n).success) {
					return { true };
				}
			}
		}

	}

	template<integral I>
	bool check_combination(
		const std::array<I,4>& y, 
		const std::vector<std::array<I,4>>& multipliers, 
		const std::vector<i32>& multiplier_indices, 
		CheckResult& cr
	) {
		int num_multipliers = multipliers.size();

		auto& perms = global_perms[num_multipliers-1];

		std::array<I, 4> totest{}; // Group identity
		// y x1 x2
		for (auto& p : perms) {
			// We shall have tested all positions of y in the multiplication
			for (i32 ypos = 0; ypos < num_multipliers+1; ++ypos) {
				// We shall have tested all combinations of inversions of the multiplying generators
				for (i32 gi = 0; gi < (1 << num_multipliers); ++gi) {
					// This creates the product
					for (i32 pos = 0; pos < num_multipliers+1; ++pos) {

						if (pos == ypos) {
							totest = group_multiplication(totest, y, _n);
							continue;
						}

						bool should_invert = (1 << (pos < ypos ? pos : pos-1)) & gi;

						auto multip = multipliers[p[pos < ypos ? pos : pos-1]];
						if (should_invert) {
							group_inversion_(multip, _n);
						}

						totest = group_multiplication(totest, multip, _n);
					}
					// We increment the count of products tested
					++cr.products_tested;

					// The product is produced, check it
					if (check_element(totest, _n).success) {
						cr.inversion_permutation_at_success = gi;
						cr.ypos_at_success = ypos;
						cr.gens_giving_success = multiplier_indices;
						cr.gens_permutation = p;
						return true;
					}

					totest = {}; // Same as group identity
				}

			}
		}
		return false;
	}

};


