#pragma once

#include <iostream>
#include <iterator>
#include <type_traits>

#define IMPLEMENT_ITERABLE(T, Type, obj, ptr, ref, elem_count, first, last)                                             \
    template<bool IsConst = true>                                                                                       \
    class iterator: public std::iterator< std::random_access_iterator_tag, T>                                           \
    {                                                                                                                   \
        using base_type = std::iterator< std::random_access_iterator_tag, T>;                                           \
    public:                                                                                                             \
        using reference = std::conditional_t<IsConst, const T&, T&>;                                                    \
        using pointer = std::conditional_t<IsConst, const T*, T*>;                                                      \
        using typename base_type::difference_type;                                                                      \
        using typename base_type::value_type;                                                                           \
        explicit iterator(Type obj) : obj(obj) {}                                                                       \
        iterator(const iterator&)  = default;                                                                           \
        iterator(iterator&&) = default;                                                                                 \
        inline iterator& operator=(const iterator&) = default;                                                          \
        inline iterator& operator=(iterator&&) = default;                                                               \
                                                                                                                        \
        inline bool operator==(iterator other) const {return obj == other.obj;}                                         \
        inline bool operator!=(iterator other) const {return !(*this == other);}                                        \
        inline reference operator*() { return ref; }                                                                    \
        inline reference operator->() { return ref; }                                                                   \
                                                                                                                        \
        iterator() : obj(nullptr) {}                                                                                    \
        inline iterator& operator++() { ptr += elem_count; return *this;}                                               \
        inline iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }                          \
                                                                                                                        \
        inline iterator& operator--() { ptr -= elem_count; return *this;}                                               \
        inline iterator operator--(int) { iterator retval = *this; --(*this); return retval; }                          \
                                                                                                                        \
        inline iterator& operator+=(difference_type n) { ptr += n * elem_count; return *this; }                         \
        inline iterator& operator-=(difference_type n) { ptr -= n * elem_count; return *this; }                         \
        inline friend iterator operator+(const iterator& it, difference_type n) { iterator cpy(*it); return cpy += n; } \
        inline friend iterator operator+(difference_type n, const iterator& it) { iterator cpy(*it); return cpy += n; } \
        inline iterator operator-(difference_type n)            { iterator cpy(*this); return cpy -= n; }               \
        inline difference_type operator-(const iterator& other) { return (ptr - other.ptr) / elem_count; }              \
        inline reference operator[](difference_type n) { return *(*this + n); }                                         \
        inline bool operator< (const iterator& other) { return other - *this > 0; }                                     \
        inline bool operator<=(const iterator& other) { return *this < other || *this == other; }                       \
        inline bool operator> (const iterator& other) { return !(*this <= other); }                                     \
        inline bool operator>=(const iterator& other) { return *this > other || *this == other; }                       \
    private:                                                                                                            \
        Type obj;                                                                                                       \
    };                                                                                                                  \
    using it    = iterator<false>;                                                                                      \
    using c_it  = iterator<true>;                                                                                       \
    using r_it  = std::reverse_iterator<it>;                                                                            \
    using rc_it = std::reverse_iterator<c_it>;                                                                          \
    inline it begin()                   { return it(first); }                                                           \
    inline it end()                     { return it(last); }                                                            \
    inline const it begin() const       { return it(first); }                                                           \
    inline const it end() const         { return it(last); }                                                            \
    inline c_it cbegin() const          { return c_it(first); }                                                         \
    inline c_it cend() const            { return c_it(last); }                                                          \
    inline const r_it rbegin() const    { return r_it(iterator<false>(last)); }                                         \
    inline const r_it rend() const      { return r_it(iterator<false>(first)); }                                        \
    inline rc_it rcbegin() const        { return rc_it(c_it(last)); }                                                   \
    inline rc_it rcend() const          { return rc_it(c_it(first)); }



// Generic contiguous 2d storage
template<typename T>
class MatrixXX
{
public:
    
    // Wraper class for a "row" of the matrix (used as return value of operator[])
    class Row
    {
    public:
        friend class MatrixXX<T>;

        IMPLEMENT_ITERABLE(T, T*, m_ptr, m_ptr, *m_ptr, 1, m_data, m_data + m_n_cols);

    public:
        Row()
        : m_data(nullptr)
        , m_n_cols(0)
        {}

        Row(const Row& other) = default;            // just copy the values
        Row& operator=(const Row& other)            // a copy assignement is only valid if the current row is valid (belongs to a matrix) and the two rows have the same size
        {
            if(!m_data || m_n_cols != other.m_n_cols)
                throw new std::runtime_error("Invalide assignement operation between Rows.");
            memcpy(m_data, other.m_data, m_n_cols * sizeof(T));
            m_n_cols = other.m_n_cols;
            return *this;
        }
        Row(Row&& other) = default;                    // just copy the values
        Row& operator=(Row&& other) = default;        // just copy the values

        // access a particular value
        inline T operator[](size_t i) const    { return m_data[i]; }
        inline T& operator[](size_t i)         { return m_data[i]; }

        inline T *data()               { return m_data; }
        inline const T *data() const   { return m_data; }

        inline size_t n_cols() const      { return m_n_cols; }

        inline void fill(const T& v) { std::fill(m_data, m_data + m_n_cols, v); }

        friend std::ostream& operator<<(std::ostream& os, const Row& row)
        {
            os << "[";
            for (size_t j = 0; j < row.n_cols(); ++j)
                os << row[j] << (j == row.n_cols()-1 ? "]" : ", ");
            return os;
        }

        inline bool operator==(const Row& other) const { return m_data == other.m_data && m_n_cols == other.m_n_cols; }
        inline bool operator!=(const Row& other) const { return m_data != other.m_data && m_n_cols != other.m_n_cols; }

    private:
        explicit Row(T* data, size_t n_cols)
        : m_data(data)
        , m_n_cols(n_cols)
        {}

        T *m_data;
        size_t m_n_cols;
    };

    IMPLEMENT_ITERABLE(Row, Row, m_row, m_row.m_data, m_row, m_row.m_n_cols, row(0), row(m_n_rows));

public:
    MatrixXX()
    : m_data(nullptr)
    , m_n_cols(0)
    , m_n_rows(0)
    {}
    explicit MatrixXX(size_t n_rows, size_t n_cols)
    : MatrixXX()
    { resize(n_rows, n_cols); }
    explicit MatrixXX(size_t n_rows, size_t n_cols, const T& v)
    : MatrixXX()
    { assign(n_rows, n_cols, v); }

    MatrixXX(const MatrixXX&) = delete;
    MatrixXX& operator=(const MatrixXX&) = delete;

    MatrixXX(MatrixXX&& other)
    : m_data(other.m_data)
    , m_n_cols(other.m_n_cols)
    , m_n_rows(other.m_n_rows)
    { other.clear(); }
    MatrixXX& operator=(MatrixXX&& other)
    {
        m_data      = other.m_data;
        m_n_cols    = other.m_n_cols;
        m_n_rows    = other.m_n_rows;
        other.clear();
    }

    ~MatrixXX() { free(m_data); }

    void resize(size_t n_rows, size_t n_cols)
    {
        if (n_rows == m_n_rows && n_cols == m_n_cols)
            return;

        if (n_rows * sizeof(T) > std::numeric_limits<size_t>::max() / n_cols)
            throw new std::runtime_error("Cannot allocate this many floats!");

        m_n_cols = n_cols;
        m_n_rows = n_rows;

        T *new_data = static_cast<T*>(std::realloc(m_data, n_rows * n_cols * sizeof(T)));
        if (!new_data && n_rows*n_cols != 0)
            throw new std::runtime_error("Couldn't allocate new data for MatrixXX!");

        m_data = new_data;
    }
    void clear()
    {
        free(m_data);
        m_data = nullptr;
        m_n_cols = m_n_rows = 0;
    }
    void assign(size_t n_rows, size_t n_cols, const T& value)
    {
        resize(n_rows, n_cols);
        fill(value);
    }
    void fill(const T& value) { std::fill(m_data, m_data + (m_n_rows * m_n_cols), value); }

    // access a particular row
    inline Row row(size_t i)                    { return Row(m_data + i * m_n_cols, m_n_cols); }
    inline const Row row(size_t i) const        { return Row(m_data + i * m_n_cols, m_n_cols); }
    inline Row operator[](size_t i)             { return row(i); }
    inline const Row operator[](size_t i) const { return row(i); }
    inline T& operator()(size_t r, size_t c)        { return m_data[r*m_n_cols + c]; }
    inline T operator()(size_t r, size_t c) const   { return m_data[r*m_n_cols + c]; }

    // first row
    inline Row front()              { return row(0); }
    inline const Row front() const  { return row(0); }
    // last row
    inline Row back()               { return row(m_n_rows - 1); }
    inline const Row back() const   { return row(m_n_rows - 1); }

    inline T* data()                { return m_data; }
    inline const T* data() const    { return m_data; }

    inline size_t n_cols() const    { return m_n_cols; }
    inline size_t n_rows() const    { return m_n_rows; }
    inline size_t size() const      { return m_n_rows * m_n_cols; }

    friend std::ostream& operator<<(std::ostream& os, const MatrixXX& m)
    {
        os << "[\n";
        for (size_t i = 0; i < m.n_rows(); ++i)
        {
            os << "  " << m[i];
            os << (i == m.n_rows()-1 ? "\n]" : ",\n");
        }
        return os;
    }

private:
    T* m_data;
    size_t m_n_cols;
    size_t m_n_rows;
};