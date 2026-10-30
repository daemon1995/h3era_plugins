#pragma once
#include <cstring>

namespace wog_path {
constexpr int maxSize = 252;
constexpr int width = maxSize + 2;
constexpr int count = width * width;

struct Point { short x, y, z; };
static_assert(sizeof(Point) == 6, "WoG Point ABI");
static_assert(sizeof(int) == 4, "WoG path distances require 32-bit int");
inline int cell(int x, int y) { return (x + 1) * width + y + 1; }
inline bool valid(const Point &p, int size) {
    return p.x >= 0 && p.y >= 0 && p.x < size && p.y < size && p.z >= 0 && p.z < 2;
}
struct Grid { int value[count]; };
struct Search {
    int heap[count], position[count], used;
    Grid *grid;
    void swapNodes(int a, int b) {
        int t = heap[a]; heap[a] = heap[b]; heap[b] = t;
        position[heap[a]] = a; position[heap[b]] = b;
    }
    void up(int n) {
        while (n > 0) {
            int p = (n - 1) / 2;
            if (grid->value[heap[p]] <= grid->value[heap[n]]) break;
            swapNodes(p, n); n = p;
        }
    }
    void insertOrDecrease(int c) {
        int n = position[c];
        if (n < 0) { n = used++; heap[n] = c; position[c] = n; }
        up(n);
    }
    int pop() {
        const int result = heap[0]; position[result] = -1; --used;
        if (used) {
            heap[0] = heap[used]; position[heap[0]] = 0;
            for (int n = 0;;) {
                int child = 2 * n + 1;
                if (child >= used) break;
                if (child + 1 < used && grid->value[heap[child + 1]] < grid->value[heap[child]]) ++child;
                if (grid->value[heap[n]] <= grid->value[heap[child]]) break;
                swapNodes(n, child); n = child;
            }
        }
        return result;
    }
    template<class Blocked>
    int run(Grid &g, int size, Point seed, Point origin, bool shortway, Blocked blocked) {
        std::memset(g.value, 0xFF, sizeof(g.value));
        if (size < 1 || size > maxSize || !valid(seed, size) || !valid(origin, size)) return -1;
        for (int x = 0; x < size; ++x)
            for (int y = 0; y < size; ++y)
                g.value[cell(x, y)] = blocked(x, y) ? -1 : 0;
        g.value[cell(origin.x, origin.y)] = 0;
        const int start = cell(seed.x, seed.y), end = cell(origin.x, origin.y);
        g.value[start] = 1;
        if (seed.x == origin.x && seed.y == origin.y && seed.z == origin.z) return 1;
        std::memset(position, 0xFF, sizeof(position)); used = 0; grid = &g;
        insertOrDecrease(start);
        while (used) {
            const int current = pop(), d = g.value[current];
            if (shortway && current == end) break;
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy) {
                    if (!dx && !dy) continue;
                    const int next = current + dx * width + dy;
                    if (g.value[next] < 0) continue;
                    const int distance = d + ((dx && dy) ? 3 : 2);
                    if (g.value[next] == 0 || distance < g.value[next]) {
                        g.value[next] = distance; insertOrDecrease(next);
                    }
                }
        }
        return 0;
    }
};

// Returns false if the field does not descend to the requested destination.
// Steps follows WoG: values greater than one stop after steps - 1 moves;
// one (and zero in the original implementation) means no early stop.
inline bool trace(const Grid &g, Grid *stepsGrid, int size, Point from, Point to,
                  int steps, Point &result) {
    if (stepsGrid) std::memset(stepsGrid->value, 0, sizeof(stepsGrid->value));
    if (!valid(from, size) || !valid(to, size)) return false;
    int x = from.x, y = from.y;
    if (x == to.x && y == to.y) { result = from; return true; }
    for (int step = 1; step <= size * size; ++step) {
        const int current = g.value[cell(x, y)];
        if (current <= 0) return false;
        int best = current, nx = x, ny = y;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                if (!dx && !dy) continue;
                const int d = g.value[cell(x, y) + dx * width + dy];
                if (d > 0 && d < best) { best = d; nx = x + dx; ny = y + dy; }
            }
        if (nx == x && ny == y) return false;
        x = nx; y = ny;
        if (stepsGrid) stepsGrid->value[cell(x, y)] = step;
        if ((x == to.x && y == to.y) || (steps > 1 && step + 1 == steps)) {
            result = Point{static_cast<short>(x), static_cast<short>(y), from.z};
            return true;
        }
    }
    return false;
}
}
