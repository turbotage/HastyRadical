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
	NONE = 0,
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
	std::unordered_set<i32> _successful;
	std::unordered_set<i32> _remaining;
	std::vector<CheckResult> _check_results;

	UnionFind _equiv_classes;

	ThreadPool _pool;
	ThreadPool _local_pool;

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
		std::deque<std::future<std::vector<i32>>> futures;
        for (i32 remidx = 0; remidx < _remaining.size(); remidx += 50) {
            futures.emplace_back(_pool.Enqueue(
                first_caller, remidx
            ));
        }
		while (futures.size() > 0) {
			auto local_successful = futures.front().get();
			futures.pop_front();
			_successful.insert(
				_successful.end(),
				local_successful.begin(),
				local_successful.end()
			);
			_remaining.erase(
				local_successful.begin(),
				local_successful.end()
			);
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
						_equiv_classes.unite(indices[i], indices[j]);
					} else if (divides_radical(abs(_n*x1 + 1), abs(gen1[2] - gen2[2]))) {
						_equiv_classes.unite(indices[i], indices[j]);
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
						_equiv_classes.unite(indices[i], indices[j]);
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
						_equiv_classes.unite(indices[i], indices[j]);
					} else if (divides_radical(abs(_n*x4 + 1), abs(gen1[2] - gen2[2]))) {
						_equiv_classes.unite(indices[i], indices[j]);
						std::println("Simon was wrong");
					}
				}
			}
		}
	}

	auto non_mult_class_tests(const std::vector<i32>& class_members)
		-> SuccessState
	{
		InitialSuccessSolution result = class_tests::is_initial_successful(
			class_members, 
			_generators_state
		);
		if (result.success) {
			return {SuccessType::SUCCESS_BY_INITIAL_TEST, result, {}, {}, {}};
		}

		AkSuccessSolution ak_result = class_tests::is_Ak_successful(
			class_members, 
			_generators_state
		);
		if (ak_result.success) {
			return {SuccessType::SUCCESS_BY_AK_TEST, {}, ak_result, {}, {}};
		}

		SeqSuccessSolution seq_result = class_tests::is_sequence_successful(
			class_members, 
			_generators_state
		);
		if (seq_result.success) {
			return {SuccessType::SUCCESS_BY_SEQUENCE_TEST, {}, {}, seq_result, {}};
		}

		return {SuccessType::NONE, {}, {}, {}, {}};
	}

	void run_non_mult_class_tests()
	{
		auto class_map = get_equiv_classes_with_bool();

		std::deque<std::future<SuccessState>> local_futures;
		for (const auto& [rep, class_info] : class_map) {
			if (class_info.second) {
				continue;
			}
			local_futures.push_back(
				_local_pool.Enqueue(
					[this](std::vector<i32> class_members) {
						return non_mult_class_tests(class_members);
					}, 
					class_info.first
				)
			);
		}

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


};


