module;

export module test2;

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


struct SuccessCombo {
	i8 permutation_index;
	i8 inversion_permutation;
	std::vector<i32> generators;
};

export class TestGammaN {
private:
	i32 _n;
	std::vector<std::array<i64,4>> _generators;
    
	ThreadSafeVector<SuccessCombo> _combos;
	ThreadSafeUnionFind _uf;

	std::vector<std::future<std::vector<i32>>> futures;

	i32 _generators_per_thread = 32;
	i32 _number_of_threads = 8;

	ThreadPool& _pool;

	//std::vector<i32> _temp1;
	//std::vector<i32> _temp2;
public:

	TestGammaN(std::vector<std::array<i64,4>>&& gens, i32 n, ThreadPool& pool, i32 generators_per_thread = 32, i32 number_of_threads = 8)
		: _n(n), 
		_generators(std::move(gens)), 
		_successful(), 
		_remaining(std::ranges::to<std::vector<i32>>(std::ranges::iota_view{0uz, _generators.size()})), 
		_generators_per_thread(generators_per_thread),
		_number_of_threads(number_of_threads),
		_pool(pool)
	{
		_successful.reserve(_remaining.size());
		futures.reserve((_remaining.size() + _generators_per_thread - 1) / _generators_per_thread);
		_temp1.reserve(_remaining.size());
		_temp2.reserve(_remaining.size());
	}

	void run_initial_check()
    {
        auto first_caller = [this](i32 start_idx) {
            std::vector<i32> out;
			i32 maxidx = std::min(start_idx + _generators_per_thread, (i32)_generators.size());
			for (i32 idx = start_idx; idx < maxidx; ++idx) {
                if (check_element(cast_matrix<i128>(_generators[idx]), _n)) {
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

	void run_multiplication_checks() 
	{

		

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

		std::set_difference(
			_remaining.begin(), _remaining.end(),
			_temp1.begin(), _temp1.end(),
			std::back_inserter(_temp2)
		);

		_remaining.swap(_temp2);
		_temp2.clear();


		return _temp1.size();
	}


	template<integral I>
	bool compute_one(i32 test_index)
	{
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

	template<integral I>
	void test_multiplications() {
		auto one_mult = [this](std::vector<i32> indices) -> bool
		{
			SuccessCombo sc;
			if (compute_one<i128>(std::move(indices), sc)) {
				
			}
		};

		return for_each_combination_with_repetition(_generators.size(), 2, 

		);
	}

	template<integral I>
	bool check_combination(
		std::vector<i32> multiplier_indices,
		SuccessCombo& sc
	) {
		int num_multipliers = multiplier_indices.size();

		auto& perms = global_perms[num_multipliers];

		std::vector<std::array<I,4>> multipliers(num_multipliers);
		for (i32 i = 0; i < num_multipliers; ++i) {
			multipliers[i] = cast_matrix<I>(_generators[multiplier_indices[i]]);
		}

		std::array<I, 4> totest{}; // Group identity
		std::array<I, 4> multip;
		// y x1 x2

		i32 perms_size = perms.size();
		// Try all position permutations
		for (i32 pi = 0; pi < perms_size; ++pi) {
			// We shall have tested all combinations of inversions of the multiplying generators
			for (i32 gi = 0; gi < (1 << num_multipliers); ++gi) {
				// This creates the product
				for (i32 pos = 0; pos < num_multipliers+1; ++pos) {
					bool should_invert = (1 << pos) & gi;
					multip = multipliers[perms[pi][pos]];
					if (should_invert) {
						group_inversion_(multip, _n);
					}
					totest = group_multiplication(totest, multip, _n);
				}

				// The product is produced, check it
				if (check_element(totest, _n)) {
					sc.permutation_index = pi;
					sc.inversion_permutation = gi;
					sc.generators = std::move(multiplier_indices);
					return true;
				}

				totest = {}; // Same as group identity
			}

		}
		return false;
	}

};

