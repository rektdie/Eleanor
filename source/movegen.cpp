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
	Bitboard pawns = board.colors[color] & board.pieces[Pawn];

	while (pawns.GetBoard()) {
		int square = pawns.getLS1BIndex();
		int direction = color ? south : north;
		int lastRank = color ? r_1 : r_8;

		Bitboard attacks = pawnAttacks[color][square];

		// Checking for possible en passant
		if (board.enPassantTarget) {
			if ((attacks & Bitboard::GetSquare(board.enPassantTarget)).GetBoard()
				&& board.sideToMove == color) {
					
				board.AddMove(Move(square, board.enPassantTarget, epCapture));
			}
		}

		attacks &= board.colors[!color];

		Bitboard firstAhead = Bitboard::GetSquare(square + direction) & ~board.occupied;
		Bitboard secondAhead = Bitboard::GetSquare(square + 2 * direction) & ~board.occupied;
		
		if ((attacks & ranks[lastRank]).GetBoard()) {
			while (attacks.GetBoard()) {
				int targetSquare = attacks.getLS1BIndex();
				
				for (int type = knightPromoCapture; type <= queenPromoCapture; type++) {
					board.AddMove(Move(square, targetSquare, type));
				}

				attacks.PopBit(targetSquare);
			}
		} else {
			while (attacks.GetBoard()) {
				int targetSquare = attacks.getLS1BIndex();
				
				board.AddMove(Move(square, targetSquare, capture));

				attacks.PopBit(targetSquare);
			}
		}

		if (firstAhead.GetBoard()) {
			int secondRank = color ? r_7 : r_2;
			if ((firstAhead & ranks[lastRank]).GetBoard()) {
				for (int type = knightPromotion; type <= queenPromotion; type++) {
					board.AddMove(Move(square, firstAhead.getLS1BIndex(), type));
				}
			} else {
				board.AddMove(Move(square, firstAhead.getLS1BIndex(), quiet));
				if (secondAhead.GetBoard() && Bitboard(ranks[secondRank]).IsSet(square)) {
					board.AddMove(Move(square, secondAhead.getLS1BIndex(), doublePawnPush));
				}
			}
		}

		pawns.PopBit(square);
	}
}

static void GenKnightMoves(Board &board, bool color) {
	Bitboard knights = board.colors[color] & board.pieces[Knight];

	while (knights.GetBoard()) {
		int square = knights.getLS1BIndex();

		Bitboard attacks = knightAttacks[square] & ~board.colors[color];
		Bitboard captures = attacks & board.colors[!color];
		attacks &= ~captures;

		while (attacks.GetBoard()) {
			int targetSquare = attacks.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, quiet));

			attacks.PopBit(targetSquare);
		}

		while (captures.GetBoard()) {
			int targetSquare = captures.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		knights.PopBit(square);
	}
}

static void GenRookMoves(Board &board, bool color) {
	Bitboard rooks = board.colors[color] & board.pieces[Rook];

	while (rooks.GetBoard()) {
		int square = rooks.getLS1BIndex();

		Bitboard attacks = getRookAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = attacks & board.colors[!color];
		attacks &= ~captures;

		while (attacks.GetBoard()) {
			int targetSquare = attacks.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, quiet));

			attacks.PopBit(targetSquare);
		}

		while (captures.GetBoard()) {
			int targetSquare = captures.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		rooks.PopBit(square);
	}
}

static void GenBishopMoves(Board &board, bool color) {
	Bitboard bishops = board.colors[color] & board.pieces[Bishop];

	while (bishops.GetBoard()) {
		int square = bishops.getLS1BIndex();

		Bitboard attacks = getBishopAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = attacks & board.colors[!color];
		attacks &= ~captures;

		while (attacks.GetBoard()) {
			int targetSquare = attacks.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, quiet));

			attacks.PopBit(targetSquare);
		}

		while (captures.GetBoard()) {
			int targetSquare = captures.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		bishops.PopBit(square);
	}
}

static void GenQueenMoves(Board &board, bool color) {
	Bitboard queens = board.colors[color] & board.pieces[Queen];

	while (queens.GetBoard()) {
		int square = queens.getLS1BIndex();

		Bitboard attacks = getQueenAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = attacks & board.colors[!color];
		attacks &= ~captures;

		while (attacks.GetBoard()) {
			int targetSquare = attacks.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, quiet));

			attacks.PopBit(targetSquare);
		}

		while (captures.GetBoard()) {
			int targetSquare = captures.getLS1BIndex();
			
			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		queens.PopBit(square);
	}
}

static void GenKingMoves(Board &board, bool color) {
	Bitboard king = board.colors[color] & board.pieces[King];

	int square = king.getLS1BIndex();

	Bitboard attacks = kingAttacks[square] & ~board.colors[color];
	Bitboard captures = attacks & board.colors[!color];
	attacks &= ~captures;

	while (attacks.GetBoard()) {
		int targetSquare = attacks.getLS1BIndex();
		
		board.AddMove(Move(square, targetSquare, quiet));

		attacks.PopBit(targetSquare);
	}

	while (captures.GetBoard()) {
		int targetSquare = captures.getLS1BIndex();
		
		board.AddMove(Move(square, targetSquare, capture));

		captures.PopBit(targetSquare);
	}

	if (board.castlingRights[color * 2 + 1]) {
		U64 QueenSide = color ? 0x1f00000000000000 : 0x1f;
		U64 mask = color ? 0x1c00000000000000 : 0x1c;
		int targetSquare = color ? c8 : c1;
		if ((Bitboard(QueenSide) & board.occupied).popCount() == 2
			&& !(mask & board.GetAttackMaps(!color))) {
			board.AddMove(Move(square, targetSquare, queenCastle));
		}
	}

	if (board.castlingRights[color * 2]) {
		U64 KingSide = color ? 0xf000000000000000 : 0xf0;
		U64 mask = color ? 0x7000000000000000 : 0x70;
		int targetSquare = color ? g8 : g1;
		if ((Bitboard(KingSide) & board.occupied).popCount() == 2
			&& !(mask & board.GetAttackMaps(!color))) {
			board.AddMove(Move(square, targetSquare, kingCastle));
		}
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
					board.attackMaps[color][type] |= getRookAttack(square, board.occupied.GetBoard()).GetBoard();
				} else if (type == Bishop) {
					board.attackMaps[color][type] |= getBishopAttack(square, board.occupied.GetBoard()).GetBoard();
				} else if (type == Queen) {
					board.attackMaps[color][type] |= getQueenAttack(square, board.occupied.GetBoard()).GetBoard();
				}
				pieces.PopBit(square);
			}
		}
	}
}

static bool isLegalMove(Board &board, Move move) {
	if (board.InCheck(board.sideToMove)) {
		if (move.moveFlags == kingCastle || move.moveFlags == queenCastle) {
			return false;
		}
		board.MakeMove(move);
		if (board.InCheck(!board.sideToMove)) {
			board.UnmakeMove();
			return false;
		}
		board.UnmakeMove();
	} else {
		board.MakeMove(move);
		if (board.InCheck(!board.sideToMove)) {
			board.UnmakeMove();
			return false;
		}
		board.UnmakeMove();
	}
	return true;
}

void GenerateMoves(Board &board, bool side) {
	board.moveList.clear();

	GenPawnMoves(board, side);

	GenKnightMoves(board, side);

	GenRookMoves(board, side);

	GenBishopMoves(board, side);

	GenQueenMoves(board, side);

	GenKingMoves(board, side);

	int listSize = board.moveList.size();
	// Filtering illegal moves
	for (int i = listSize - 1; i >= 0; --i) {
		if (!isLegalMove(board, board.moveList[i])) {
			board.moveList.erase(board.moveList.begin() + i);
		}
    }
}