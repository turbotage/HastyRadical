module;

export module radlib;

import std;

// ---------- types ----------
export using u8 = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;
export using u128 = __uint128_t;    // requires GCC/Clang on 64-bit targets

export using i8 = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;
export using i128 = __int128_t;      // requires GCC/Clang on

std::vector<u32> precomputed_primes;

export template<typename T>
concept integral = std::is_same_v<T, i8> || std::is_same_v<T, i16> || std::is_same_v<T, i32> || std::is_same_v<T, i64> || std::is_same_v<T, i128>;

export template<typename T>
concept uintegral = std::is_same_v<T, u8> || std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64> || std::is_same_v<T, u128>;

consteval i32 idx(i32 i, i32 j, i32 dim = 2) {
    return i * dim + j;
}

export template<integral I>
I gcd(I a, I b);

export template<integral I>
bool divides_radical(I n, I m);


export template<integral I>
void gamma_isomorphism(std::array<I,4>& mat, I n) {
    mat[idx(0,0)] -= 1;
    mat[idx(1,1)] -= 1;

    mat[idx(0,0)] /= n;
    mat[idx(0,1)] /= n;
    mat[idx(1,0)] /= n;
    mat[idx(1,1)] /= n;
}

export template<integral I>
void gamma_isomorphism_inv(std::array<I,4>& mat, I n) {
    mat[idx(0,0)] *= n;
    mat[idx(0,1)] *= n;
    mat[idx(1,0)] *= n;
    mat[idx(1,1)] *= n;

    mat[idx(0,0)] += 1;
    mat[idx(1,1)] += 1;
}


export template<integral I>
void group_multiplication(const std::array<I,4>& lhs, const std::array<I,4>& rhs, std::array<I,4>& out, I n) {
    // Unrolled 2x2 matrix multiplication
    out[idx(0,0)] = n*(lhs[idx(0,0)] * rhs[idx(0,0)] + lhs[idx(0,1)] * rhs[idx(1,0)]);
    out[idx(0,1)] = n*(lhs[idx(0,0)] * rhs[idx(0,1)] + lhs[idx(0,1)] * rhs[idx(1,1)]);
    out[idx(1,0)] = n*(lhs[idx(1,0)] * rhs[idx(0,0)] + lhs[idx(1,1)] * rhs[idx(1,0)]);
    out[idx(1,1)] = n*(lhs[idx(1,0)] * rhs[idx(0,1)] + lhs[idx(1,1)] * rhs[idx(1,1)]);

    out[0] += lhs[0] + rhs[0];
    out[1] += lhs[1] + rhs[1];
    out[2] += lhs[2] + rhs[2];
    out[3] += lhs[3] + rhs[3];
}

export template<integral I>
std::array<I,4> group_multiplication(const std::array<I,4>& lhs, const std::array<I,4>& rhs, I n) {
    std::array<I,4> out;
    group_multiplication(lhs, rhs, out, n);
    return out;
}

export template<integral I>
void group_inversion(const std::array<I,4>& in, std::array<I,4>& out, I n) {
    // not correct
    out[0] = -in[0];
    out[1] = -in[1];
    out[2] = -in[2];
    out[3] = -in[3];
}

export template<integral I>
std::array<I,4> group_inversion(const std::array<I,4>& in, I n) {
    std::array<I,4> out;
    group_inversion(in, out, n);
    return out;
}

export template<integral I>
std::array<I,4> group_identity() {
    return {0, 0, 0, 0};
}

export template<integral I1, integral I2>
std::array<I1,4> cast_matrix(const std::array<I2,4>& in) {
    return {
        static_cast<I1>(in[0]), 
        static_cast<I1>(in[1]), 
        static_cast<I1>(in[2]), 
        static_cast<I1>(in[3])
    };
}

export template<integral I>
bool check_element(const std::array<I,4>& mat) {
    // not correct
    if (divides_radical(mat[idx(0,0)], mat[idx(1,0)]))
        return true;
    if (divides_radical(mat[idx(1,0)], mat[idx(1,1)]))
        return true;
    if (divides_radical(mat[idx(0,1)], mat[idx(1,1)]))
        return true;
    return false;
}





template<integral I>
I gcd(I a, I b) {
    while (b) {
        I r = a % b;
        a = b;
        b = r;
    }
    return a;
}

template<integral I>
bool divides_radical(I n, I m) {
    while (true) {
        I g = gcd(n, m);
        if (g == 1) 
            return n == 1;
        do {
            n /= g;
        } while (n % g == 0);
        if (n == 1) 
            return true;
    }
}




// ---------- utility functions ----------
export std::string get_project_file_path(const std::string& relative_path) {
    return std::string(PROJECT_ROOT_DIR) + "/" + relative_path;
}

export template<integral I>
std::vector<std::array<I,4>> load_group_generators(I n) {
    std::ifstream file(
        get_project_file_path("generators/gamma_" + std::to_string(n) + "_generators.txt")
    );
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open generators file");
    }

    // First pass: count lines
    int line_count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            line_count++;
        }
    }

    std::vector<std::array<I,4>> generators;
    generators.reserve(1 + line_count / 4);

    file.clear();
    file.seekg(0);

    int line_count2 = 0;
    std::array<I,4> current_gen;
    while(std::getline(file, line)) {
        if (!line.empty()) {
            I val;
            try {
                val = std::stoll(line);
            } catch (const std::invalid_argument& e) {
                throw std::runtime_error("Invalid number format in generators file: " + line);
            } catch (const std::out_of_range& e) {
                throw std::runtime_error("Number out of range in generators file: " + line);
            }
            current_gen[line_count2 % 4] = val;
            if (line_count2 % 4 == 3) {
                gamma_isomorphism(current_gen, n);
                generators.emplace_back(
                    current_gen
                );
            }
            ++line_count2;
        }
    }
    file.close();
    return generators;
}