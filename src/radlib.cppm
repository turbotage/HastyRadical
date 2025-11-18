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
void gamma_isomorphism(std::array<I,4>& mat, i32 n) {
    mat[idx(0,0)] -= 1;
    mat[idx(1,1)] -= 1;

    mat[idx(0,0)] /= n;
    mat[idx(0,1)] /= n;
    mat[idx(1,0)] /= n;
    mat[idx(1,1)] /= n;
}

export template<integral I>
void gamma_isomorphism_inv(std::array<I,4>& mat, i32 n) {
    mat[idx(0,0)] *= n;
    mat[idx(0,1)] *= n;
    mat[idx(1,0)] *= n;
    mat[idx(1,1)] *= n;

    mat[idx(0,0)] += 1;
    mat[idx(1,1)] += 1;
}


export template<integral I>
void group_multiplication_(const std::array<I,4>& lhs, const std::array<I,4>& rhs, std::array<I,4>& out, i32 n) {
    // Unrolled 2x2 matrix multiplication, out = X + Y + nXY
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
std::array<I,4> group_multiplication(const std::array<I,4>& lhs, const std::array<I,4>& rhs, i32 n) {
    std::array<I,4> out;
    group_multiplication_(lhs, rhs, out, n);
    return out;
}

export template<integral I>
void group_inversion_(std::array<I,4>& inout, i32 n) {
    // not correct
    I temp = inout[0];
    inout[0] = inout[3];
    inout[3] = temp;
    inout[1] = -inout[1];
    inout[2] = -inout[2];
}

export template<integral I>
void print(const std::array<I,4>& mat) {
    std::println("[[{}, {}], [{}, {}]]", mat[0], mat[1], mat[2], mat[3]);
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
inline I abs(I x) {
    return x < 0 ? -x : x;
}

export enum class SuccessType : u8 {
    NONE = 0,
    RAD_31 = 1,
    RAD_21 = 2,
    X2_EQ_N = 3,
    X3_EQ_N = 4
};

export struct CheckResultRegular {
    bool success;
    u8 info;
};

export template<integral I>
CheckResultRegular check_element(const std::array<I,4>& mat, i32 n) {
    // not correct
    if (divides_radical(abs(mat[idx(1,0)]), abs(mat[idx(0,0)])))
        return {true, (u8)SuccessType::RAD_31};
    if (divides_radical(abs(mat[idx(0,1)]), abs(mat[idx(0,0)])))
        return {true, (u8)SuccessType::RAD_21};
    if (abs(mat[idx(0,1)]) == n)
        return {true, (u8)SuccessType::X2_EQ_N};
    if (abs(mat[idx(1,0)]) == n)
        return {true, (u8)SuccessType::X3_EQ_N};
    return {false, (u8)SuccessType::NONE};
}

export enum class SuccessTypeAk : u8 {
    NONE = 0,
    LEFT_MULTIPLY = 1,
    RIGHT_MULTIPLY = 2,
    LEFT_MULTIPLY_INVERT = 3,
    RIGHT_MULTIPLY_INVERT = 4
};

export struct CheckResultAk {
    bool success;
    u8 info;
    u32 k_value;
};

export template<integral I>
CheckResultAk check_element_Ak(const std::array<I,4>& mat, i32 n, i32 lower_k, i32 upper_k) {

    std::array<I,4> test_map;
    for (u16 k = lower_k; k < upper_k; ++k) {
        test_map[0] = k;
        test_map[1] = -k*k;
        test_map[2] = 1;
        test_map[3] = -k;

        // Left multiply
        std::array<I,4> result;
        result = group_multiplication(mat, test_map, n);
        if (check_element(result, n).success) return {true, (u8)SuccessTypeAk::LEFT_MULTIPLY, k};
        // Right multiply
        result = group_multiplication(test_map, mat, n);
        if (check_element(result, n).success) return {true, (u8)SuccessTypeAk::RIGHT_MULTIPLY, k};
        // Invert and left multiply and right multiply
        group_inversion_(test_map, n);
        result = group_multiplication(mat, test_map, n);
        if (check_element(result, n).success) return {true, (u8)SuccessTypeAk::LEFT_MULTIPLY_INVERT, k};
        result = group_multiplication(test_map, mat, n);
        if (check_element(result, n).success) return {true, (u8)SuccessTypeAk::RIGHT_MULTIPLY_INVERT, k};
    
    }

    return {false, (u8)SuccessTypeAk::NONE, 0};
}

export struct CheckResultSeq {
    bool success;
    u8 k;
};

export template<integral I>
CheckResultSeq check_element_sequence(const std::array<I,4>& mat, i32 n)
{
    I x1 = mat[0];
    I x3 = mat[2];
    I temp;
    for (i32 k = 0; k < 6; ++k) {
        temp = x1 + x3;
        x3 = n*x1  + 1;
        x1 = temp;

        if (divides_radical(abs(x3), abs(x1))) {
            return {true, static_cast<u8>(k)};
        }
    }
    return {false, 0};
}


export template<integral I>
I gcd(I a, I b) {
    while (b) {
        I r = a % b;
        a = b;
        b = r;
    }
    return a;
}

export template<integral I>
bool divides_radical(I a, I b) {
    if (a == 0)
        return true;
    while (true) {
        I g = gcd(a, b);
        if (g == 1) 
            return a == 1;
        do {
            a /= g;
        } while (a % g == 0);
        if (a == 1) 
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

            if (val > n*n*n*n*n) {
                throw std::runtime_error("Generator entry larger than n**5: ");
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