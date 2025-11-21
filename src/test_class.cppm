module;

#include <functional>
#include <future>
export module test_class;

import std;
import radlib;
import threadpool;
import containers;
import util;
import tests;



export class TestGammaN {
private:
	i32 _n;
	std::vector<std::array<i64,4>> _generators;
	std::unordered_set<i32> _successful;
	std::unordered_set<i32> _remaining;
	std::vector<SuccessState> _success_states;

	UnionFind _union_find;

	ThreadPool _small_pool;
	ThreadPool _large_pool;

	GeneratorsState _generators_state;

public:

	TestGammaN(std::vector<std::array<i64,4>>&& gens, i32 n)
		: 
		_n(n), 
		_generators(std::move(gens)), 
		_successful(), 
		_remaining(std::ranges::to<std::unordered_set<i32>>(std::ranges::iota_view{0uz, _generators.size()})), 
		_success_states(_generators.size()),
		_union_find(_generators.size()),
		_small_pool(20),
		_large_pool(20),
		_generators_state(
			_generators, _successful, _remaining, 
			_n,
			_generators.size(),
			_large_pool
		)
	{
		_successful.reserve(_remaining.size());
	}

	GeneratorsState& get_generators_state() {
		return _generators_state;
	}

	std::unordered_map<i32, std::pair<std::vector<i32>, bool>> get_equiv_classes_with_bool() {
		return _union_find.get_classes_with_bool();
	}

	std::vector<std::pair<std::vector<i32>, bool>> get_equiv_classes_list_with_bool() {
		return _union_find.get_classes_list_with_bool();
	}

	const std::unordered_set<i32>& get_successful_generators() const {
		return _successful;
	}

	void run_initial_check()
    {
		i32 gens_per_invoc = 50;

        auto first_caller = [this, gens_per_invoc](i32 start_idx) {
            std::vector<i32> out;
			i32 maxidx = std::min(start_idx + gens_per_invoc, (i32)_generators.size());
			for (i32 idx = start_idx; idx < maxidx; ++idx) {
                if (check_element(cast_matrix<i128>(_generators[idx]), _n) != CheckElementSuccessType::NONE) {
                    out.push_back(idx);
                }
            }
            return out;
        };

        // Test all remaining generators
		std::deque<std::future<std::vector<i32>>> futures;
        for (i32 remidx = 0; remidx < _remaining.size(); remidx += gens_per_invoc) {
            futures.emplace_back(_large_pool.Enqueue(
                first_caller, remidx
            ));
        }
		while (futures.size() > 0) {
			auto local_successful = futures.front().get();
			futures.pop_front();
			_successful.insert(
				local_successful.begin(),
				local_successful.end()
			);
			for (i32 succ_idx : local_successful) {
				_remaining.erase(succ_idx);
			}
			// Change success states to initial success
			for (i32 succ_idx : local_successful) {
				_success_states[succ_idx] = SuccessState{
					SuccessState::SuccessType::SUCCESS_BY_INITIAL_TEST, succ_idx, 
					{}, {}, {}, {}
				};
			}
		}
	}

	void build_initial_equiv_classes() 
	{
		i32 gen_size = _generators.size();

		std::unordered_map<i64, std::vector<i32>> same_map;
		//same_map.clear();

		// Same signed x1
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
						_union_find.unite(indices[i], indices[j]);
					} else if (divides_radical(abs(_n*x1 + 1), abs(gen1[2] - gen2[2]))) {
						_union_find.unite(indices[i], indices[j]);
						std::println("Simon was wrong");
					}
				}
			}
		}
		same_map.clear();

		// Same x3 or x2
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
						_union_find.unite(indices[i], indices[j]);
					}

				}
			}
		}
		same_map.clear();

		// Same signed x4
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
						_union_find.unite(indices[i], indices[j]);
					} else if (divides_radical(abs(_n*x4 + 1), abs(gen1[2] - gen2[2]))) {
						_union_find.unite(indices[i], indices[j]);
						std::println("Simon was wrong");
					}
				}
			}
		}
	}

	void update_success_states_from_class_test(
		const std::vector<i32>& class_members,
		const SuccessState& success_state
	) 
	{
		switch (success_state.success_type) {
		case SuccessState::SuccessType::NONE:
		{
			// No success, nothing to do
			return;
		}
		case SuccessState::SuccessType::SUCCESS_BY_EQUIVALENCE:
		{
			throw std::runtime_error(
				"A class test should not result in an equivalence success state"
			);
		}
		break;
		case SuccessState::SuccessType::SUCCESS_BY_INITIAL_TEST:
		{
			if (!success_state.initial_success_solution.has_value()) {
				throw std::runtime_error(
					"Success state was SUCCESS_BY_INITIAL_TEST but no initial success solution was provided"
				);
			}
			i32 initial_successful_genidx = 
				success_state.initial_success_solution->initial_successful_genidx;

			for (i32 member_idx : class_members) {
				// For the initiall successtype, the successstate
				// is already set for the successful generator
				// Now we just set the others to equivalence
				// with the initial successful one as parent
				if (member_idx != initial_successful_genidx) {
					_success_states[member_idx] = SuccessState{
						SuccessState::SuccessType::SUCCESS_BY_EQUIVALENCE,
						initial_successful_genidx,
						{}, {}, {}, {}
					};
				}
			}
		}
		break;
		case SuccessState::SuccessType::SUCCESS_BY_AK_TEST:
		{
			if (!success_state.ak_success_solution.has_value()) {
				throw std::runtime_error(
					"Success state was SUCCESS_BY_AK_TEST but no AK success solution was provided"
				);
			}
			i32 ak_successful_genidx = 
				success_state.ak_success_solution->ak_successful_genidx;

			for (i32 member_idx : class_members) {
				// For the index giving the solution we set the
				// success state to the returned state
				// For the others we set it to equivalence
				// with the AK successful one as parent
				if (member_idx == ak_successful_genidx) {
					_success_states[member_idx] = success_state;
				} else {
					_success_states[member_idx] = SuccessState{
						SuccessState::SuccessType::SUCCESS_BY_EQUIVALENCE,
						ak_successful_genidx,
						{}, {}, {}, {}
					};
				}
			}
		}
		break;
		case SuccessState::SuccessType::SUCCESS_BY_SEQUENCE_TEST:
		{
			if (!success_state.seq_success_solution.has_value()) {
				throw std::runtime_error(
					"Success state was SUCCESS_BY_SEQUENCE_TEST but no sequence success solution was provided"
				);
			}
			i32 seq_successful_genidx = 
				success_state.seq_success_solution->seq_successful_genidx;

			for (i32 member_idx : class_members) {
				// For the index giving the solution we set the
				// success state to the returned state
				// For the others we set it to equivalence
				// with the sequence successful one as parent
				if (member_idx == seq_successful_genidx) {
					_success_states[member_idx] = success_state;
				} else {
					_success_states[member_idx] = SuccessState{
						SuccessState::SuccessType::SUCCESS_BY_EQUIVALENCE,
						seq_successful_genidx,
						{}, {}, {}, {}
					};
				}
			}
		}
		break;
		case SuccessState::SuccessType::SUCCESS_BY_MULT_TEST:
		{
			if (!success_state.mult_success_solution.has_value()) {
				throw std::runtime_error(
					"Success state was SUCCESS_BY_MULT_TEST but no multiplication success solution was provided"
				);
			}

			i32 mult_successful_genidx = 
				success_state.mult_success_solution->mult_successful_genidx;

			for (i32 member_idx : class_members) {
				if (member_idx >= _success_states.size()) {
					throw std::runtime_error(
						"Member index out of bounds in update_success_states_from_class_test"
					);
				}

				// For the index giving the solution we set the
				// success state to the returned state
				// For the others we set it to equivalence
				// with the multiplication successful one as parent
				if (member_idx == mult_successful_genidx) {
					_success_states[member_idx] = success_state;
				} else {
					_success_states[member_idx] = SuccessState{
						SuccessState::SuccessType::SUCCESS_BY_EQUIVALENCE,
						mult_successful_genidx,
						{}, {}, {}, {}
					};
				}
			}

			// For the multiplication success, we also need to unite the classes
			// of all the multipliers used
			auto mult_solution = *success_state.mult_success_solution;
			auto gens_in_mult = gens_in_successful_mult(mult_solution);
			for (i32 gen_idx : gens_in_mult) {
				if (gen_idx == mult_successful_genidx) {
					continue;
				}
				_union_find.unite(
					mult_successful_genidx,
					gen_idx
				);
			}
		}
		break;
		}

		// If we reached this point, we should set this class as successful
		// the success_parent_genidx should always point to a generator
		// in the now successful class (it should be the generator that
		// provided the success)
		_union_find.set_val(success_state.success_parent_genidx, true);

	}

	void run_non_mult_class_tests()
	{
		std::vector<std::pair<std::vector<i32>, bool>> class_map = _union_find.get_classes_list_with_bool();
		std::vector<i32> new_successful;
		new_successful.reserve(_remaining.size());

		using SuccessPair = std::pair<
			SuccessState,
			crefw<std::vector<i32>>
		>;

		std::deque<std::future<SuccessPair>> futures;
		for (const auto& [class_members, success] : class_map) {
			if (success) {
				continue;
			}
			futures.push_back({
				_large_pool.Enqueue(
					[this](const std::vector<i32>& class_members) 
						-> SuccessPair
					{
						return std::make_pair(is_non_mult_successful(
							class_members,
							_generators_state
						), std::cref(class_members));
					}, 
					std::cref(class_members)
				)
			});
		}
		std::vector<std::vector<i32>> successful_classes;
		while (!futures.empty()) {
			auto pair = pop_when_ready(futures);
			const auto& result = pair.first;
			const auto& class_members = pair.second.get();

			// Process result as needed
			update_success_states_from_class_test(
				class_members,
				result
			);
			if (result.success_type != SuccessState::SuccessType::NONE) {
				successful_classes.push_back(class_members);
				// std::println(
				// 	"Class successful by non-mult test. Success type: {}. Class size: {}",
				// 	static_cast<int>(result.success_type),
				// 	class_members.size()
				// );
			}
		}


		for (const auto& class_members : successful_classes) {
			_successful.insert(
				class_members.begin(),
				class_members.end()
			);
			for (i32 succ_idx : class_members) {
				_remaining.erase(succ_idx);
			}
		}

		// Unite all successful classes
		class_map = _union_find.get_classes_list_with_bool();
		i32 last_success_index = -1;
		for (const auto& [class_members, success] : class_map) {
			if (success) {
				if (last_success_index == -1) {
					last_success_index = class_members[0];
				} else {
					_union_find.unite(last_success_index, class_members[0]);
					last_success_index = class_members[0];
				}
			}
		}

	}

	inline auto one_mult_class_test(
		const std::vector<i32>& class_members,
		MultType mult_type,
		std::atomic<bool>& stop_flag
	) -> std::pair<MultAndAkSuccessSolution, crefw<std::vector<i32>>>
	{
		if (mult_type == MultType::MULT1) {
			return std::make_pair(
				is_mult1_successful(
					class_members,
					_generators_state,
					stop_flag
				), 
				std::cref(class_members)
			);
		} else if (mult_type == MultType::MULT2) {
			return std::make_pair(
				is_mult2_successful(
					class_members,
					_generators_state,
					stop_flag
				), 
				std::cref(class_members)
			);
		} else if (mult_type == MultType::MULT2_AK) {
			return std::make_pair(
				is_mult2_Ak_successful(
					class_members,
					_generators_state,
					stop_flag
				), 
				std::cref(class_members)
			);
		} else {
			throw std::runtime_error(
				"Unknown multiplication type in run_mult_class_tests"
			);
		}
	}

	i32 run_mult_class_tests(MultType mult_type)
	{
		std::vector<std::pair<std::vector<i32>, bool>> classes_list = _union_find.get_classes_list_with_bool();
		i32 current_successful = _successful.size();

		_generators_state.current_class_size = classes_list.size();

		// std::unordered_set<i32> processed_indices;
		// std::unordered_set<i32> all_successful_indices;
		// for (const auto& [class_members, class_success] : classes_list) {
		// 	for (i32 idx : class_members) {
		// 		if (processed_indices.contains(idx)) {
		// 			throw std::runtime_error(
		// 				"Generator index appears in multiple classes in run_mult_class_tests"
		// 			);
		// 		}
		// 		processed_indices.insert(idx);
		// 		if (class_success) {
		// 			all_successful_indices.insert(idx);
		// 		}
		// 	}
		// }
		// for (i32 idx = 0; idx < _generators.size(); ++idx) {
		// 	if (!processed_indices.contains(idx)) {
		// 		throw std::runtime_error(
		// 			"Generator index missing from classes in run_mult_class_tests"
		// 		);
		// 	}
		// }
		// for (i32 idx : all_successful_indices) {
		// 	if (!_successful.contains(idx)) {
		// 		throw std::runtime_error(
		// 			"Generator marked successful in class but not in successful set in run_mult_class_tests"
		// 		);
		// 	}
		// }
		// for (i32 idx : _successful) {
		// 	if (!all_successful_indices.contains(idx)) {
		// 		throw std::runtime_error(
		// 			"Generator marked successful in successful set but not in class in run_mult_class_tests"
		// 		);
		// 	}
		// }

		std::println(
			"Number of classes {}",
			classes_list.size()
		);

		std::atomic<bool> stop_flag;
		stop_flag.store(false);

		using MultVecPair = std::pair<
			MultAndAkSuccessSolution,
			std::reference_wrapper<const std::vector<i32>>
		>;

		std::deque<std::future<MultVecPair>> futures;

		std::optional<MultVecPair> result_pair;

		auto one_class_mult_checker = [this] (
							const std::vector<i32>& class_members, 
							MultType mult_type, 
							std::atomic<bool>& stop_flag) 
		{
			return one_mult_class_test(
				class_members,
				mult_type,
				stop_flag
			);
		};

		i32 small_pool_size = _small_pool.NumberOfThreads();
		for (const auto& [class_members, class_success] : classes_list) {
			// We shall only try non successful classes
			if (class_success) {
				continue;
			}
			//std::println("Submitting class with first member idx {}", class_members[0]);

			// auto ret = one_class_mult_checker(
			// 	class_members, mult_type, stop_flag
			// );

			// std::promise<decltype(ret)> prom;
			// prom.set_value(std::move(ret));
			// futures.push_back(prom.get_future());

			futures.push_back({
				_small_pool.Enqueue(
					one_class_mult_checker,
					std::cref(class_members), mult_type, std::ref(stop_flag)
				),
			});

			if (futures.size() > small_pool_size) {
				MultVecPair popped_pair = pop_when_ready(futures);
				if (popped_pair.first.mult_result.success) {
					stop_flag.store(true);
					result_pair = std::move(popped_pair);
					break;
				}
			}
		}
		std::vector<std::vector<i32>> successful_classes;

		auto process_result_pair = [&]() {
			if (result_pair.has_value()) {
				MultVecPair& result_pair_ref = *result_pair;
				if (result_pair_ref.first.mult_result.success) {
					// Process result as needed
					const auto& result = result_pair_ref.first;
					const auto& class_members = result_pair_ref.second.get();
					update_success_states_from_class_test(
						class_members,
						SuccessState{
							result.mult_result.success ? 
								SuccessState::SuccessType::SUCCESS_BY_MULT_TEST : 
								SuccessState::SuccessType::NONE,
							result.mult_successful_genidx,
							{}, {}, {}, result
						}
					);
					if (result.mult_result.success) {
						for (i32 succ_idx : class_members) {
							if (_successful.contains(succ_idx)) {
								throw std::runtime_error(
									"Generator already marked successful in run_mult_class_tests"
								);
							}
						}

						successful_classes.push_back(class_members);
						// std::println(
						// 	"Class successful by mult test. Class size: {}",
						// 	class_members.size()
						// );
					}
				}
			}
		};

		process_result_pair();
		while (!futures.empty()) {
			result_pair = pop_when_ready(futures);
			process_result_pair();
		}

		for (const auto& class_members : successful_classes) {
			_successful.insert(
				class_members.begin(),
				class_members.end()
			);
			for (i32 succ_idx : class_members) {
				_remaining.erase(succ_idx);
			}
		}

		i32 new_successful_size = _successful.size() - current_successful;

		return new_successful_size;
	}

private:


};

