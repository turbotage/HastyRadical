module;

#include <unordered_map>
export module containers;

import std;

export class UnionFind {
private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    std::unordered_map<int, bool> class_bool_;

    int _last_num_classes = -1;
    int _last_largest_class_size = -1;
public:
    UnionFind(int n) : parent_(n), rank_(n, 0) {
        std::iota(parent_.begin(), parent_.end(), 0); // Each element is its own parent initially
        _last_num_classes = n;
        _last_largest_class_size = 1;
    }

    // Find with path compression
    /*
    int find(int x) {
        if (parent_[x] != x) {
            parent_[x] = find(parent_[x]); // Path compression
        }
        return parent_[x];
    }
    */

    int find(int x) {
        // Find the root of the component
        int root = x;
        while (parent_[root] != root) {
            root = parent_[root];
        }
        // Path compression
        int current = x;
        while (parent_[current] != root) {
            int next = parent_[current];
            parent_[current] = root; // Compress path
            current = next;
        }
        return root;
    }

    // Union by rank
    void unite(int x, int y) {
        int root_x = find(x);
        int root_y = find(y);
        
        if (root_x != root_y) {
            bool merged_bool = class_bool_[root_x] || class_bool_[root_y];
            
            if (rank_[root_x] < rank_[root_y]) {
                parent_[root_x] = root_y;
                class_bool_[root_y] = merged_bool;
                class_bool_.erase(root_x);
            } else if (rank_[root_x] > rank_[root_y]) {
                parent_[root_y] = root_x;
                class_bool_[root_x] = merged_bool;
                class_bool_.erase(root_y);
            } else {
                parent_[root_y] = root_x;
                rank_[root_x]++;
                class_bool_[root_x] = merged_bool;
                class_bool_.erase(root_y);
            }
        }
    }

    // Check if two elements are in same equivalence class
    bool same_class(int x, int y) {
        return find(x) == find(y);
    }

    void set_val(int x, bool val) {
        int root_x = find(x);
        class_bool_[root_x] = val;
    }

    bool get_val(int x) {
        int root_x = find(x);
        return class_bool_[root_x];
    }

    std::unordered_map<int, std::pair<std::vector<int>, bool>> get_classes_with_bool() {
        std::unordered_map<int, std::pair<std::vector<int>, bool>> classes;
        classes.reserve(_last_num_classes);
        for (int i = 0; i < parent_.size(); ++i) {
            int root = find(i);
            auto &entry = classes[root];
            entry.first.reserve(_last_largest_class_size);
            entry.first.push_back(i);
            entry.second = class_bool_[root];
        }
        _last_num_classes = classes.size();
        return classes;
    }

    std::vector<std::pair<std::vector<int>, bool>> get_classes_list_with_bool() {
        std::vector<std::pair<std::vector<int>, bool>> result;
        auto class_map = get_classes_with_bool();
        result.reserve(class_map.size());
        for (auto& [root, class_info] : class_map) {
            result.push_back(std::move(class_info));
        }
        return result;
    }

};

export class ThreadSafeUnionFind {
private:
	mutable std::shared_mutex mutex_;
	UnionFind uf_;
public:
	ThreadSafeUnionFind(int n) : uf_(n) {}

    void unite(int x, int y) {
        std::unique_lock lock(mutex_);
        uf_.unite(x, y);
    }

    bool same_class(int x, int y) {
        std::shared_lock lock(mutex_);
        return uf_.same_class(x, y);
    }

    int find(int x) {
        std::shared_lock lock(mutex_);
        return uf_.find(x);
    }

};

template<typename T>
class ThreadSafeSet {
private:
    mutable std::shared_mutex mutex_;
    std::set<T> set_;

public:
    void insert(const T& item) {
        std::unique_lock lock(mutex_);
        set_.insert(item);
    }

    void insert(T&& item) {
        std::unique_lock lock(mutex_);
        set_.insert(std::move(item));
    }

    bool erase(const T& item) {
        std::unique_lock lock(mutex_);
        return set_.erase(item) > 0;
    }

    bool contains(const T& item) const {
        std::shared_lock lock(mutex_);
        return set_.contains(item); // C++20, or use set_.find(item) != set_.end()
    }

    std::size_t size() const {
        std::shared_lock lock(mutex_);
        return set_.size();
    }

    bool empty() const {
        std::shared_lock lock(mutex_);
        return set_.empty();
    }

    // For iteration - returns a copy to avoid iterator invalidation
    std::set<T> snapshot() const {
        std::shared_lock lock(mutex_);
        return set_;
    }

    // Template for more complex operations
    template<typename Func>
    auto with_read_lock(Func&& func) const -> decltype(func(set_)) {
        std::shared_lock lock(mutex_);
        return func(set_);
    }

    template<typename Func>
    auto with_write_lock(Func&& func) -> decltype(func(set_)) {
        std::unique_lock lock(mutex_);
        return func(set_);
    }
};


template<typename T>
class ThreadSafeVector {
private:
    mutable std::shared_mutex mutex_;
    std::vector<T> vector_;

public:
    // Construction
    ThreadSafeVector() = default;
    ThreadSafeVector(std::size_t size) : vector_(size) {}
    ThreadSafeVector(std::size_t size, const T& value) : vector_(size, value) {}

    // Add elements
    void push_back(const T& item) {
        std::unique_lock lock(mutex_);
        vector_.push_back(item);
    }

    void push_back(T&& item) {
        std::unique_lock lock(mutex_);
        vector_.push_back(std::move(item));
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        std::unique_lock lock(mutex_);
        vector_.emplace_back(std::forward<Args>(args)...);
    }

    // Access elements (returns copy for safety)
    T at(std::size_t index) const {
        std::shared_lock lock(mutex_);
        return vector_.at(index);
    }

    T operator[](std::size_t index) const {
        std::shared_lock lock(mutex_);
        return vector_[index];
    }

    // Size operations
    std::size_t size() const {
        std::shared_lock lock(mutex_);
        return vector_.size();
    }

    bool empty() const {
        std::shared_lock lock(mutex_);
        return vector_.empty();
    }

    void clear() {
        std::unique_lock lock(mutex_);
        vector_.clear();
    }

    void reserve(std::size_t capacity) {
        std::unique_lock lock(mutex_);
        vector_.reserve(capacity);
    }

    // Get snapshot for iteration
    std::vector<T> snapshot() const {
        std::shared_lock lock(mutex_);
        return vector_;
    }

    // Functional interface for complex operations
    template<typename Func>
    auto with_read_lock(Func&& func) const -> decltype(func(vector_)) {
        std::shared_lock lock(mutex_);
        return func(vector_);
    }

    template<typename Func>
    auto with_write_lock(Func&& func) -> decltype(func(vector_)) {
        std::unique_lock lock(mutex_);
        return func(vector_);
    }
};

