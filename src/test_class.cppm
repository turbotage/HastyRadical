module;

export module test;

import std;
import radlib;
import threadpool;
import containers;

import tests;


struct CheckResult {
	i64 products_tested = 1;

	i8 inversion_permutation_at_success;
	i8 ypos_at_success;
	std::vector<i32> gens_giving_success;
	std::vector<i32> gens_permutation;
};

enum class SuccessType : u8 {
	SUCCESS_BY_EQUIVALENCE = 1,
	SUCCESS_BY_INITIAL_TEST = 2,
	SUCCESS_BY_AK_TEST = 3,
	SUCCESS_BY_SEQUENCE_TEST = 4,
	SUCCESS_BY_MULT_TEST = 5
};

struct SuccessState {
	SuccessType success_type;
	std::optional<InitialSuccessSolution> initial_success_solution;
	std::optional<AkSuccessSolution> ak_success_solution;
	std::optional<SeqSuccessSolution> seq_success_solution;
	std::optional<MultSuccessSolution> mult_success_solution;
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

	ThreadPool _pool;
	ThreadPool _local_pool;

	std::vector<i32> _temp1;
	std::vector<i32> _temp2

	GeneratorsState _generators_state;

public:

	TestGammaN(std::vector<std::array<i64,4>>&& gens, i32 n)
		: _n(n), 
		_generators(std::move(gens)), 
		_successful(), 
		_remaining(std::ranges::to<std::vector<i32>>(std::ranges::iota_view{0uz, _generators.size()})), 
		_check_results(_generators.size()),
		_generators_per_thread(generators_per_thread),
		_number_of_threads(number_of_threads),
		_pool(4),
		_local_pool(8),
		_equiv_classes(_generators.size()),
		_generators_state(
			{_generators, _successful, _remaining, _n, _local_pool}
		)
	{
		_successful.reserve(_remaining.size());
		futures.reserve((_remaining.size() + _generators_per_thread - 1) / _generators_per_thread);
		_temp1.reserve(_remaining.size());
		_temp2.reserve(_remaining.size());
	}

	GeneratorsState& get_generators_state() {
		return _equiv_classes;
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

			if (!was_successful) {
				int members_size = members.size();
				for (int midx = 0; midx < members_size; ++midx) {
					int nextidx = (midx + 1) % members_size;
					auto mat1 = cast_matrix<i128>(_generators[members[midx]]);
					auto mat2 = cast_matrix<i128>(_generators[members[nextidx]]);
					auto prod = group_multiplication(mat1, mat2, _n);
					if (check_element(prod, _n).success) {
						was_successful = true;
						std::println(
							"Gen class {}, successful via pairwise mult, members {}, {}, class size is {}", 
							rep, members[midx], members[nextidx], members.size()
						);
						break;
					}
					auto ret = check_element_Ak(prod, _n, 1, _n);
					if (ret.success) {
						was_successful = true;
						std::println(
							"Gen class {}, successful via Ak on pairwise mult, k={}, multype={}, members {}, {}, class size is {}", 
							rep, ret.k_value, ret.info, members[midx], members[nextidx], members.size()
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

	auto non_mult_class_tests(std::vector<i32>& class_members)
	{
		InitialSuccessSolution result = test_initial_success_in_class(
			class_members, 
			_generators_state
		);

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



};


