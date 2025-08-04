#include "movegen.h"

namespace MOVEGEN {

static Bitboard bishopMasks[64];
static Bitboard rookMasks[64];

static Bitboard bishopAttacks[64][512];
static Bitboard rookAttacks[64][4096];

static Bitboard notAfile = ~files[A];
static Bitboard notHfile = ~files[H];
static Bitboard notABfile = notAfile & ~files[B];
static Bitboard notHGfile = notHfile & ~files[G];

static Bitboard maskPawnAttacks(int side, int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	// White
	if (!side) {
		if (((attacker << noEa) & notAfile)) attacks |= (attacker << noEa);
		if (((attacker << noWe) & notHfile)) attacks |= (attacker << noWe);
	}
	// Black
	else {
		if (((attacker << soEa) & notAfile)) attacks |= (attacker << soEa);
		if (((attacker << soWe) & notHfile)) attacks |= (attacker << soWe);
	}
	return attacks;
}

static Bitboard maskKnightAttacks(int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	if (((attacker << noEa + north) & notAfile)) attacks |= (attacker << noEa + north);
	if (((attacker << noWe + north) & notHfile)) attacks |= (attacker << noWe + north);
	if (((attacker << noWe + west) & notHGfile)) attacks |= (attacker << noWe + west);
	if (((attacker << noEa + east) & notABfile)) attacks |= (attacker << noEa + east);

	if (((attacker << soWe + south) & notHfile)) attacks |= (attacker << soWe + south);
	if (((attacker << soEa + south) & notAfile)) attacks |= (attacker << soEa + south);
	if (((attacker << soEa + east) & notABfile)) attacks |= (attacker << soEa + east);
	if (((attacker << soWe + west) & notHGfile)) attacks |= (attacker << soWe + west);

	return attacks;
}

static Bitboard maskKingAttacks(int square) {
	Bitboard attacks;
	Bitboard attacker = Bitboard::GetSquare(square);

	attacks |= (attacker << north);
	attacks |= (attacker << south);
	
	if (((attacker << west) & notHfile)) attacks |= (attacker << west);
	if (((attacker << soWe) & notHfile)) attacks |= (attacker << soWe);
	if (((attacker << soEa) & notAfile)) attacks |= (attacker << soEa);
	if (((attacker << east) & notAfile)) attacks |= (attacker << east);
	if (((attacker << noEa) & notAfile)) attacks |= (attacker << noEa);
	if (((attacker << noWe) & notHfile)) attacks |= (attacker << noWe);

	return attacks;
}

static Bitboard maskBishopAttacks(int square) {
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

static Bitboard maskRookAttacks(int square) {
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

static Bitboard bishopAttacksOnTheFly(int square, Bitboard block) {
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
		if ((currentSquare & block)) break;
	}
	// North East
	for (int r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
	}
	//South West
	for (int r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
	}
	//South East
	for (int r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
		currentSquare = (1ULL << (r * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
	}

	return attacks;
}

static Bitboard rookAttacksOnTheFly(int square, Bitboard block) {
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
		if ((currentSquare & block)) break;
	}
	// South
	for (int r = tr - 1; r >= 0; r--) {
		currentSquare = (1ULL << (r * 8 + tf));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
	}
	// East
	for (int f = tf + 1; f <= 7; f++) {
		currentSquare = (1ULL << (tr * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
	}
	// West
	for (int f = tf - 1; f >= 0; f--) {
		currentSquare = (1ULL << (tr * 8 + f));
		attacks |= currentSquare;
		if ((currentSquare & block)) break;
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

			int magicIndex = (occupancy * bishopMagicNumbers[square])
			>> (64 - bishopRelevantBits[square]);

			bishopAttacks[square][magicIndex] = bishopAttacksOnTheFly(square, occupancy);
		}

		for (int index = 0; index < 4096; index++) {
			Bitboard occupancy = Bitboard::getOccupancy(index, rookMasks[square]);

			int magicIndex = (occupancy * rookMagicNumbers[square])
			>> (64 - rookRelevantBits[square]);

			rookAttacks[square][magicIndex] = rookAttacksOnTheFly(square, occupancy);
		}
	}
}

Bitboard getBishopAttack(int square, U64 occupancy) {
	occupancy &= bishopMasks[square];
	occupancy *= bishopMagicNumbers[square];
	occupancy >>= (64 - bishopRelevantBits[square]);

	return bishopAttacks[square][occupancy];
}

Bitboard getRookAttack(int square, U64 occupancy) {
	occupancy &= rookMasks[square];
	occupancy *= rookMagicNumbers[square];
	occupancy >>= (64 - rookRelevantBits[square]);

	return rookAttacks[square][occupancy];
}

Bitboard getQueenAttack(int square, U64 occupancy) {
	U64 bishopOccupancy = occupancy;
	U64 rookOccupancy = occupancy;

	bishopOccupancy &= bishopMasks[square];
	bishopOccupancy *= bishopMagicNumbers[square];
	bishopOccupancy >>= (64 - bishopRelevantBits[square]);

	rookOccupancy &= rookMasks[square];
	rookOccupancy *= rookMagicNumbers[square];
	rookOccupancy >>= (64 - rookRelevantBits[square]);

	return rookAttacks[square][rookOccupancy] | bishopAttacks[square][bishopOccupancy];
}

Bitboard getPieceAttacks(int square, int piece, int color, U64 occupancy) {
	if (piece == Pawn) {
		return pawnAttacks[color][square];
	} else if (piece == Knight) {
		return knightAttacks[square];
	} else if (piece == King) {
		return kingAttacks[square];
	} else if (piece == Bishop) {
		return getBishopAttack(square, occupancy);
	} else if (piece == Rook) {
		return getRookAttack(square, occupancy);
	} else {
		return getQueenAttack(square, occupancy);
	}
}

Bitboard getPawnPushes(int square, bool color, Bitboard &occupancy) {
	Bitboard pushes;

	int direction = color ? south : north;
	Bitboard secondRank = color ? ranks[r_7] : ranks[r_2];

	if (!occupancy.IsSet(square + direction)) {
		pushes.SetBit(square + direction);

		if (secondRank.IsSet(square) && !occupancy.IsSet(square + 2 * direction)) {
			pushes.SetBit(square + 2 * direction);
		} 
	}

	return pushes;
}

void GenThreatMaps(Board &board) {
	for (int color = White; color <= Black; color++) {
		board.colorThreats[color] = 0ULL;

		for (int type = Pawn; type <= King; type++) {
			board.pieceThreats[type] = 0ULL;

			Bitboard pieces = board.pieces[type] & board.colors[color];

			while (pieces) {
				int square = pieces.getLS1BIndex();

				board.pieceThreats[type] |= MOVEGEN::getPieceAttacks(square, type, color, board.occupied
					& ~(board.colors[!color] & board.pieces[King]));

				pieces.PopBit(square);

			}

			board.colorThreats[color] |= board.pieceThreats[type];
		}
	}
}
}
