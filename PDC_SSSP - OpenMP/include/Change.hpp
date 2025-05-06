#pragma once
#include "GraphCSR.hpp"

// Simple event describing a single edge update
enum class Op { Insert, Delete };

struct Change {
    Op      op;
    Vertex  u;
    Vertex  v;
    Weight  w = 1.0f;   // default weight for inserted edge
};
