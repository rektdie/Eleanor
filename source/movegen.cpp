#include "movegen.h"

Bitboard pawnAttacks[2][64];
Bitboard knightAttacks[64];
Bitboard kingAttacks[64];


Bitboard bishopMasks[64];
Bitboard rookMasks[64];

Bitboard bishopAttacks[64][512];
Bitboard rookAttacks[64][4096];

static Bitboard notAfile = ~files[A];
static Bitboard notHfile = ~files[H];
static Bitboard notABfile = notAfile & ~files[B];
static Bitboard notHGfile = notHfile & ~files[G];

Bitboard maskPawnAttacks(int side, int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	// White
	if (!side) {
		if (((attacker << noEa) & notAfile).GetBoard()) attacks |= (attacker << noEa);
		if (((attacker << noWe) & notHfile).GetBoard()) attacks |= (attacker << noWe);
	}
	// Black
	else {
		if (((attacker << soEa) & notAfile).GetBoard()) attacks |= (attacker << soEa);
		if (((attacker << soWe) & notHfile).GetBoard()) attacks |= (attacker << soWe);
	}
	return attacks;
}

Bitboard maskKnightAttacks(int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	if (((attacker << noEa + north) & notAfile).GetBoard()) attacks |= (attacker << noEa + north);
	if (((attacker << noWe + north) & notHfile).GetBoard()) attacks |= (attacker << noWe + north);
	if (((attacker << noWe + west) & notHGfile).GetBoard()) attacks |= (attacker << noWe + west);
	if (((attacker << noEa + east) & notABfile).GetBoard()) attacks |= (attacker << noEa + east);

	if (((attacker << soWe + south) & notHfile).GetBoard()) attacks |= (attacker << soWe + south);
	if (((attacker << soEa + south) & notAfile).GetBoard()) attacks |= (attacker << soEa + south);
	if (((attacker << soEa + east) & notABfile).GetBoard()) attacks |= (attacker << soEa + east);
	if (((attacker << soWe + west) & notHGfile).GetBoard()) attacks |= (attacker << soWe + west);

	return attacks;
}

Bitboard maskKingAttacks(int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	attacks |= (attacker << north);
	attacks |= (attacker << south);
	
	if (((attacker << west) & notHfile).GetBoard()) attacks |= (attacker << west);
	if (((attacker << soWe) & notHfile).GetBoard()) attacks |= (attacker << soWe);
	if (((attacker << soEa) & notAfile).GetBoard()) attacks |= (attacker << soEa);
	if (((attacker << east) & notAfile).GetBoard()) attacks |= (attacker << east);
	if (((attacker << noEa) & notAfile).GetBoard()) attacks |= (attacker << noEa);
	if (((attacker << noWe) & notHfile).GetBoard()) attacks |= (attacker << noWe);

	return attacks;
}

Bitboard maskBishopAttacks(int square) {
	Bitboard attacks;

	// target rank and files
	int tr = square / 8;
	int tf = square % 8;
	
	// mask relevant occupancy bits
	
	// North West
	for (int r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) {
		attacks |= (1ULL << (r * 8 + f));
	}
	// North East
	for (int r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) {
		attacks |= (1ULL << (r * 8 + f));
	}
	// South West
	for (int r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) {
		attacks |= (1ULL << (r * 8 + f));
	}
	// South East
	for (int r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) {
		attacks |= (1ULL << (r * 8 + f));
	}

	return attacks;
}

Bitboard maskRookAttacks(int square) {
	Bitboard attacks;

	// target rank and files
	int tr = square / 8;
	int tf = square % 8;
	
	// mask relevant occupancy bits

	// North
	for (int r = tr + 1; r <= 6; r++) {
		attacks |= (1ULL << (r * 8 + tf));
	}
	// South
	for (int r = tr - 1; r >= 1; r--) {
		attacks |= (1ULL << (r * 8 + tf));
	}
	// East
	for (int f = tf + 1; f <= 6; f++) {
		attacks |= (1ULL << (tr * 8 + f));
	}
	// West
	for (int f = tf - 1; f >= 1; f--) {
		attacks |= (1ULL << (tr * 8 + f));
	}

	return attacks;
}

Bitboard bishopAttacksOnTheFly(int square, Bitboard block) {
	Bitboard attacks;

	// target rank and files
	int tr = square / 8;
	int tf = square % 8;

	Bitboard currentSquare;
	
	// generate bishop attacks
	
	// North West
	for (int r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	// North East
	for (int r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	//South West
	for (int r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	//South East
	for (int r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}

	return attacks;
}

Bitboard rookAttacksOnTheFly(int square, Bitboard block) {
	Bitboard attacks;

	// target rank and files
	int tr = square / 8;
	int tf = square % 8;

	Bitboard currentSquare;
	
	// generate rook attacks
	
	// North
	for (int r = tr + 1; r <= 7; r++) {
		currentSquare = (1ULL << (r * 8 + tf));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	// South
	for (int r = tr - 1; r >= 0; r--) {
		currentSquare = (1ULL << (r * 8 + tf));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	// East
	for (int f = tf + 1; f <= 7; f++) {
		currentSquare = (1ULL << (tr * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}
	// West
	for (int f = tf - 1; f >= 0; f--) {
		currentSquare = (1ULL << (tr * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block).GetBoard()) break;
	}

	return attacks;
}

void initLeaperAttacks() {
	for (int square = 0; square < 64; square++) {
		pawnAttacks[White][square] = maskPawnAttacks(White, square);
		pawnAttacks[Black][square] = maskPawnAttacks(Black, square);

		knightAttacks[square] = maskKnightAttacks(square);
		kingAttacks[square] = maskKingAttacks(square);
	}
}

void initSliderAttacks() {
	for (int square = 0; square < 64; square++) {
		bishopMasks[square] = maskBishopAttacks(square);
		rookMasks[square] = maskRookAttacks(square);

		for (int index = 0; index < 512; index++) {
			Bitboard occupancy = Bitboard::getOccupancy(index, bishopMasks[square]);

			int magicIndex = (occupancy.GetBoard() * bishopMagicNumbers[square])
			>> (64 - bishopRelevantBits[square]);

			bishopAttacks[square][magicIndex] = bishopAttacksOnTheFly(square, occupancy);
		}

		for (int index = 0; index < 4096; index++) {
			Bitboard occupancy = Bitboard::getOccupancy(index, rookMasks[square]);

			int magicIndex = (occupancy.GetBoard() * rookMagicNumbers[square])
			>> (64 - rookRelevantBits[square]);

			rookAttacks[square][magicIndex] = rookAttacksOnTheFly(square, occupancy);
		}
	}
}

Bitboard getBishopAttack(int square, U64 occupancy) {
	occupancy &= bishopMasks[square].GetBoard();
	occupancy *= bishopMagicNumbers[square];
	occupancy >>= (64 - bishopRelevantBits[square]);

	return bishopAttacks[square][occupancy];
}

Bitboard getRookAttack(int square, U64 occupancy) {
	occupancy &= rookMasks[square].GetBoard();
	occupancy *= rookMagicNumbers[square];
	occupancy >>= (64 - rookRelevantBits[square]);

	return rookAttacks[square][occupancy];
}

Bitboard getQueenAttack(int square, U64 occupancy) {
	U64 bishopOccupancy = occupancy;
	U64 rookOccupancy = occupancy;

	bishopOccupancy &= bishopMasks[square].GetBoard();
	bishopOccupancy *= bishopMagicNumbers[square];
	bishopOccupancy >>= (64 - bishopRelevantBits[square]);

	rookOccupancy &= rookMasks[square].GetBoard();
	rookOccupancy *= rookMagicNumbers[square];
	rookOccupancy >>= (64 - rookRelevantBits[square]);

	return rookAttacks[square][rookOccupancy] | bishopAttacks[square][bishopOccupancy];
}

static void GenPawnMoves(Board &board, bool color) {
	
}

static void GenKnightMoves(Board &board, bool color) {
	
}

static void GenRookMoves(Board &board, bool color) {
	
}

static void GenBishopMoves(Board &board, bool color) {
	
}

static void GenQueenMoves(Board &board, bool color) {
	
}

static void GenKingMoves(Board &board, bool color) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();

	Bitboard moves = (kingAttacks[kingSquare] & ~board.GetAttackMaps(!color)) & ~board.colors[color];
	Bitboard captures = moves & board.colors[!color];
	moves &= ~board.colors[!color];

	while (moves.GetBoard()) {
		int square = moves.getLS1BIndex();

		board.AddMove(Move(kingSquare, square, quiet));

		moves.PopBit(square);
	}

	while (captures.GetBoard()) {
		int square = captures.getLS1BIndex();

		Move move(kingSquare, square, capture);
		board.AddMove(move);

		captures.PopBit(square);
	}
}

void GenAttackMaps(Board &board) {
	for (int color = White; color <= Black; color++) {
		for (int type = Pawn; type <= King; type++) {
			board.attackMaps[color][type] = 0ULL;
			Bitboard pieces = board.pieces[type] & board.colors[color];

			while (pieces.GetBoard()) {
				int square = pieces.getLS1BIndex();
				if (type == Pawn) {
					board.attackMaps[color][type] |= pawnAttacks[color][square].GetBoard();
				} else if (type == Knight) {
					board.attackMaps[color][type] |= knightAttacks[square].GetBoard();
				} else if (type == King) {
					board.attackMaps[color][type] |= kingAttacks[square].GetBoard();
				} else if (type == Rook) {
					board.attackMaps[color][type] |= getRookAttack(square, (board.occupied 
						& ~(board.colors[!color] & board.pieces[King])).GetBoard()).GetBoard();
				} else if (type == Bishop) {
					board.attackMaps[color][type] |= getBishopAttack(square, (board.occupied 
						& ~(board.colors[!color] & board.pieces[King])).GetBoard()).GetBoard();
				} else if (type == Queen) {
					board.attackMaps[color][type] |= getQueenAttack(square, (board.occupied 
						& ~(board.colors[!color] & board.pieces[King])).GetBoard()).GetBoard();
				}
				pieces.PopBit(square);
			}
		}
	}
}

// static bool isLegalMove(Board &board, Move move) {
	
// }

void GenerateMoves(Board &board, bool side) {
	board.moveList.clear();

	GenPawnMoves(board, side);

	GenKnightMoves(board, side);

	GenRookMoves(board, side);

	GenBishopMoves(board, side);

	GenQueenMoves(board, side);

	GenKingMoves(board, side);
}