#pragma once
#include <cstdint>
#include <array>

using U64 = uint64_t;

constexpr char StartingFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum COLOR {
	White,
	Black
};

enum PIECE {
	Pawn,
	Knight,
	Bishop,
	Rook,
	Queen,
	King
};

enum CASTLING_RIGHTS {
    whiteKingRight = 1,
    whiteQueenRight = 2,
    blackKingRight = 4,
    blackQueenRight = 8
};

constexpr std::array<const char, 12> PIECE_LETTERS = {
	'P', 'p',
	'N', 'n',
	'B', 'b',
	'R', 'r',
	'Q', 'q',
	'K', 'k'
};

enum FILES { A, B, C, D, E, F, G, H };
enum RANKS { r_1, r_2, r_3, r_4, r_5, r_6, r_7, r_8 };

constexpr std::array<U64, 8> files {
	0x0101010101010101,
	0x0202020202020202,
	0x0404040404040404,
	0x0808080808080808,
	0x1010101010101010,
	0x2020202020202020,
	0x4040404040404040,
	0x8080808080808080
};

constexpr std::array<U64, 8> ranks {
	0x00000000000000ff,
	0x000000000000ff00,
	0x0000000000ff0000,
	0x00000000ff000000,
	0x000000ff00000000,
	0x0000ff0000000000,
	0x00ff000000000000,
	0xff00000000000000
};

enum SQUARE {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

constexpr std::array<const char*, 64> squareCoords = {
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

enum DIRECTION {
	north = 8,
	east = 1,

	south = -north,
	west = -east,

	noWe = north + west,
	noEa = north + east,
	soWe = south + west,
	soEa = south + east
};
