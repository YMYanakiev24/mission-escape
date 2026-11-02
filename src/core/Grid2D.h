#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

/// @file Grid2D.h
/// The dynamically allocated 2D array used for every per-cell map in the game
/// (walls, fog memory, BFS distances, visited flags, ...).

namespace core {

/// Owning, dynamically allocated 2D array of `T` with row-pointer access (`grid[y][x]`).
///
/// Memory layout: one contiguous block of `width * height` elements allocated with `new T[]`
/// plus a table of `height` row pointers (allocated with `new T*[]`) that point into that block.
/// This gives classic two-index access while needing only two allocations, which keeps the
/// class cache friendly and makes exception safety easy to reason about.
///
/// The class follows the rule of five: copies are deep, moves steal the buffers and leave the
/// source empty, and the destructor releases both arrays with `delete[]`.
template <typename T>
class Grid2D {
public:
    /// Creates an empty 0 x 0 grid that owns no memory.
    Grid2D() noexcept = default;

    /// Allocates a @p width x @p height grid with every element set to @p initialValue.
    /// @throws std::invalid_argument if a dimension is negative.
    Grid2D(int width, int height, const T& initialValue = T{}) {
        allocate(width, height);
        try {
            fill(initialValue);
        } catch (...) {
            release(); // the destructor does not run for a constructor that throws
            throw;
        }
    }

    /// Releases the element block and the row-pointer table.
    ~Grid2D() { release(); }

    /// Deep copy: allocates new memory and copies every element.
    Grid2D(const Grid2D& other) {
        allocate(other.m_width, other.m_height);
        try {
            std::copy(other.m_storage, other.m_storage + other.elementCount(), m_storage);
        } catch (...) {
            release();
            throw;
        }
    }

    /// Deep copy assignment (copy-and-swap, strong exception guarantee).
    Grid2D& operator=(const Grid2D& other) {
        if (this != &other) {
            Grid2D copy(other);
            swap(copy);
        }
        return *this;
    }

    /// Move construction: takes ownership of the buffers, leaving @p other empty.
    Grid2D(Grid2D&& other) noexcept { swap(other); }

    /// Move assignment: frees the current buffers and takes ownership of @p other's.
    Grid2D& operator=(Grid2D&& other) noexcept {
        if (this != &other) {
            release();
            swap(other);
        }
        return *this;
    }

    /// Number of columns.
    int width() const noexcept { return m_width; }

    /// Number of rows.
    int height() const noexcept { return m_height; }

    /// True when the grid holds no elements.
    bool empty() const noexcept { return m_storage == nullptr; }

    /// True when (x, y) addresses an element of this grid.
    bool inBounds(int x, int y) const noexcept {
        return x >= 0 && y >= 0 && x < m_width && y < m_height;
    }

    /// Bounds-checked element access.
    /// @throws std::out_of_range if (x, y) is outside the grid.
    T& at(int x, int y) {
        checkBounds(x, y);
        return m_rows[y][x];
    }

    /// Bounds-checked element access (read-only).
    /// @throws std::out_of_range if (x, y) is outside the grid.
    const T& at(int x, int y) const {
        checkBounds(x, y);
        return m_rows[y][x];
    }

    /// Unchecked row access enabling `grid[y][x]`; the caller guarantees `0 <= y < height()`.
    T* operator[](int y) noexcept { return m_rows[y]; }

    /// Unchecked read-only row access enabling `grid[y][x]`.
    const T* operator[](int y) const noexcept { return m_rows[y]; }

    /// Sets every element to @p value.
    void fill(const T& value) { std::fill(m_storage, m_storage + elementCount(), value); }

    /// Exchanges the contents of two grids without allocating.
    void swap(Grid2D& other) noexcept {
        std::swap(m_width, other.m_width);
        std::swap(m_height, other.m_height);
        std::swap(m_storage, other.m_storage);
        std::swap(m_rows, other.m_rows);
    }

    /// Element-wise equality (same dimensions and same values).
    friend bool operator==(const Grid2D& lhs, const Grid2D& rhs) {
        return lhs.m_width == rhs.m_width && lhs.m_height == rhs.m_height &&
               std::equal(lhs.m_storage, lhs.m_storage + lhs.elementCount(), rhs.m_storage);
    }

    friend bool operator!=(const Grid2D& lhs, const Grid2D& rhs) { return !(lhs == rhs); }

private:
    std::size_t elementCount() const noexcept {
        return static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    }

    void checkBounds(int x, int y) const {
        if (!inBounds(x, y)) {
            throw std::out_of_range("Grid2D index out of range");
        }
    }

    /// Allocates both arrays for a fresh (empty) grid. On failure nothing is leaked.
    void allocate(int width, int height) {
        if (width < 0 || height < 0) {
            throw std::invalid_argument("Grid2D dimensions must not be negative");
        }
        if (width == 0 || height == 0) {
            return;
        }
        const std::size_t count =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        T* storage = new T[count];
        T** rows = nullptr;
        try {
            rows = new T*[static_cast<std::size_t>(height)];
        } catch (...) {
            delete[] storage;
            throw;
        }
        for (int row = 0; row < height; ++row) {
            rows[row] = storage + static_cast<std::size_t>(row) * static_cast<std::size_t>(width);
        }
        m_width = width;
        m_height = height;
        m_storage = storage;
        m_rows = rows;
    }

    /// Frees both arrays and returns the grid to the empty state.
    void release() noexcept {
        delete[] m_rows;
        delete[] m_storage;
        m_rows = nullptr;
        m_storage = nullptr;
        m_width = 0;
        m_height = 0;
    }

    int m_width = 0;
    int m_height = 0;
    T* m_storage = nullptr; ///< Contiguous element block (owned).
    T** m_rows = nullptr;   ///< Row-pointer table into m_storage (owned).
};

} // namespace core
