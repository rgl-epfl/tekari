#include <iostream>

// #include <tekari/common.h>
#include <iostream>
#include <string>
#include <tekari/matrix_xx.h>

#define ASSERT(cond, fmt, ...) \
    if (!(cond)) { \
        fprintf(stderr, "[Error:%s:%d] ", __func__, __LINE__); \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }

template<typename T>
void check_dims(size_t rows, size_t cols, const MatrixXX<T> &m)
{
    ASSERT(m.n_cols() == cols, "got %zu should have found %zu\n", m.n_cols(), cols);
    ASSERT(m.n_rows() == rows, "got %zu should have found %zu\n", m.n_rows(), rows);
    ASSERT(m.size() == rows * cols, "got %zu should have found %zu\n", m.size(), rows * cols);
}

template<typename T>
void test_constructors(size_t rows, size_t cols, const T& v)
{
    MatrixXX<T> m(rows, cols);
    check_dims(rows, cols, m);
    MatrixXX<T> m2(rows, cols, v);
    check_dims(rows, cols, m2);
    for(size_t i = 0; i < m2.n_rows(); ++i)
        for(size_t j = 0; j < m2.n_cols(); ++j)
            ASSERT(m2[i][j] == v, "%s\n", "wrong value");
}

template<typename T>
void test_resize(size_t rows, size_t cols)
{
    MatrixXX<T> m;
    m.resize(rows, cols);
    check_dims(rows, cols, m);
}

template<typename T>
void test_assign(size_t rows, size_t cols, const T& v)
{
    MatrixXX<T> m;
    m.assign(rows, cols, v);
    check_dims(rows, cols, m);
    for(size_t i = 0; i < m.n_rows(); ++i)
        for(size_t j = 0; j < m.n_cols(); ++j)
            ASSERT(m[i][j] == v, "%s\n", "wrong value");
}

template<typename T>
void test_clear()
{
    MatrixXX<T> m(6, 10);
    check_dims(6, 10, m);
    m.clear();
    check_dims(0, 0, m);
}

void test_iterator()
{
    MatrixXX<int> m(6, 10);

    int x = 0;
    int y = 0;
    for(auto& row: m) {
        ++y;
        for(auto& item: row) {
            ++x;
            item = x * y;
        }
        x = 0;
    }
    std::cout << m << std::endl;
}

int main(int argc, char const* argv[])
{
    test_constructors(54, 23, 2.4);
    test_clear<float>();
    test_resize<uint16_t>(213, 13);
    test_assign(14, 2, 43);
    test_iterator();
    return 0;
}
