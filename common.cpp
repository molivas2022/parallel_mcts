/*
Used representation: pointy-top
Exanmple of 3x3 grid:
     / \
    |0,0|
   / \ / \
  |1,0|0,1|
 / \ / \ / \
|2,0|1,1|0,2|
 \ / \ / \ /
  |2,1|1,2|
   \ / \ /
    |2,2|
     \ /
*/

#pragma once

#include <cstdint>
#include <array>

/* Constants */

using u = std::uint8_t; // max board size: N = 13

enum class Player : u { None, First, Second };

static constexpr u N = 13;
static constexpr u PADDED_N = N + 2;
static constexpr u BOARD_SIZE = PADDED_N * PADDED_N;

/* Helpers */

static constexpr std::array<u, 6> NEIGHBOR_OFFSETS = {
    static_cast<u>(-1),             // up left      (0, -1)
    static_cast<u>(-PADDED_N),      // up right     (-1, 0)
    static_cast<u>(PADDED_N - 1),   // left         (1, -1)
    static_cast<u>(-PADDED_N + 1),  // right        (-1, 1)
    static_cast<u>(PADDED_N),       // down left    (1, 0)
    static_cast<u>(1)               // down right   (0, 1)
};