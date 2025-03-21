#include "movegen.h"

namespace MOVEGEN {

static Bitboard pawnAttacks[2][64];
static Bitboard knightAttacks[64];
static Bitboard kingAttacks[64];


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

static Bitboard getBishopAttack(int square, U64 occupancy) {
	occupancy &= bishopMasks[square];
	occupancy *= bishopMagicNumbers[square];
	occupancy >>= (64 - bishopRelevantBits[square]);

	return bishopAttacks[square][occupancy];
}

static Bitboard getRookAttack(int square, U64 occupancy) {
	occupancy &= rookMasks[square];
	occupancy *= rookMagicNumbers[square];
	occupancy >>= (64 - rookRelevantBits[square]);

	return rookAttacks[square][occupancy];
}

static Bitboard getQueenAttack(int square, U64 occupancy) {
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

static Bitboard getPieceAttacks(int square, int piece, int color, U64 occupancy) {
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

static Bitboard getPawnPushes(int square, bool color, Bitboard &occupancy) {
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

// Returns bitboard of checkers
static Bitboard Checkers(Board &board, bool color) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
	Bitboard checkerBoard = 0ULL;

	for (int type = Pawn; type < King; type++) {
		Bitboard targets = getPieceAttacks(kingSquare, type, color, board.occupied);
		checkerBoard |= targets & board.pieces[type] & board.colors[!color];
	}
	return checkerBoard;
}

// Returns the direction to target from square
static int GetDirection(int square, int target) {
	int rankDiff = (target / 8) - (square / 8);
	int fileDiff = (target % 8) - (square % 8);

	// Main directions
	if (rankDiff == 0 && fileDiff < 0) {
		return west;
	} else if (rankDiff == 0 && fileDiff > 0) {
		return east;
	} else if (rankDiff < 0 && fileDiff == 0) {
		return south;
	} else if (rankDiff > 0 && fileDiff == 0) {
		return north;
	}

	// Diagonals
	if (abs(rankDiff) == abs(fileDiff)) {
		if (rankDiff < 0 && fileDiff < 0) {
			return soWe;
		} else if (rankDiff < 0 && fileDiff > 0) {
			return soEa;
		} else if (rankDiff > 0 && fileDiff < 0) {
			return noWe;
		} else if (rankDiff > 0 && fileDiff > 0) {
			return noEa;
		}
	}

	return 0;
}

// Returns a bitboard of the squares from attacker to target
static Bitboard SliderRayToSquare(int attacker, int target) {
	Bitboard ray;
	int direction = GetDirection(attacker, target);

	if (direction) {
		while (attacker != target) {
			ray.SetBit(attacker);
			attacker += direction;
		}
		ray.SetBit(target);
	}

	return ray;
}

static Bitboard GetPinningRay(Board &board, int square, int direction) {
	Bitboard ray;
	int currentSquare = (board.pieces[King] & board.colors[board.sideToMove]).getLS1BIndex();
	int previousRank = currentSquare / 8;

	currentSquare += direction;
	while (currentSquare < 64 && currentSquare >= 0) {
		int currentRank = currentSquare / 8;

		int difference = abs(currentRank - previousRank);

		if ((abs(direction) == 7 || abs(direction) == 9)
			&& difference != 1) {
				break;
		} else if (abs(direction) == 1 && difference) {
			break;
		}
		previousRank = currentRank;

		ray.SetBit(currentSquare);
		if (board.occupied.IsSet(currentSquare)) {
			if (currentSquare == square) {
				currentSquare += direction;
				continue;
			}
			int attackerType = board.GetPieceType(currentSquare);
			int attackerColor = board.GetPieceColor(currentSquare);

			if (attackerColor != board.sideToMove) {
				// Pinned by rook or queen
				if (abs(direction) == 1 || abs(direction) == 8) {
					if (attackerType == Rook || attackerType == Queen) {
						return ray;
					}
				// Pinned by bishop or queen
				} else if (direction % 2 != 0) {
					if (attackerType == Bishop || attackerType == Queen) {
						return ray;
					}
				}
			}
			break;
		}
		currentSquare += direction;
	}

	return 0ULL;
}

static bool IsPinned(Board &board, int square) {
	int kingSquare = (board.colors[board.sideToMove] & board.pieces[King]).getLS1BIndex();
	int directionToKing = GetDirection(square, kingSquare);

	if (directionToKing) {
		// Checking if there are any pieces between square and kingSquare
		if ((SliderRayToSquare(square, kingSquare)
			& board.occupied).PopCount() != 2) {
			return false;
		}
		
		return GetPinningRay(board, square, -directionToKing).PopCount();
	}

	return false;
}

static bool IsLegalEnPassant(Board &board, int square) {
	Bitboard kingSquare = board.colors[board.sideToMove] & board.pieces[King];
	int pushDirection = board.sideToMove ? south : north;
	Bitboard newOccupancy = board.occupied;
	newOccupancy.PopBit(square);
	newOccupancy.PopBit(board.enPassantTarget - pushDirection);

	// Looking for horizontal checks
	int directionToKing = GetDirection(square, kingSquare.getLS1BIndex());

	// Either east or west
	if (abs(directionToKing) == 1) {
		int currentSq = kingSquare.getLS1BIndex() - directionToKing;
		
		Bitboard endingFile = directionToKing > 0 ? files[H] : files[A];

		while (!endingFile.IsSet(currentSq)) {
			if (newOccupancy.IsSet(currentSq)) {
				int squareColor = board.GetPieceColor(currentSq);
				int squarePiece = board.GetPieceType(currentSq);

				if (squareColor != board.sideToMove) {
					if (squarePiece == Rook || squarePiece == Queen) {
						return false;
					}
				}
				break;
			}
			currentSq -= directionToKing;
		}
	}

	return true;
}

static void GenPawnMoves(Board &board, bool color) {
	Bitboard pawns = board.colors[color] & board.pieces[Pawn];

	while (pawns) {
		int square = pawns.getLS1BIndex();

		Bitboard pushes = getPawnPushes(square, color, board.occupied);
		Bitboard captures = pawnAttacks[color][square];

		Bitboard lastRank = color ? ranks[r_1] : ranks[r_8];

		if (IsPinned(board, square)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square, -directionToKing);
			pushes &= pinningRay;
			captures &= pinningRay;

			// Checking for en passant
			if (board.enPassantTarget != noEPTarget) {
				if (captures.IsSet(board.enPassantTarget)) {
					if (IsLegalEnPassant(board, square)) {
						board.AddMove(Move(square, board.enPassantTarget, epCapture));
					}
				}
			}

			// Removing own pieces from capture mask
			captures &= board.colors[!color];

			while (pushes) {
				int pushSquare = pushes.getLS1BIndex();

				if (abs(square - pushSquare) == 16) {
					board.AddMove(Move(square, pushSquare, doublePawnPush));
				} else {
					board.AddMove(Move(square, pushSquare, quiet));
				}

				pushes.PopBit(pushSquare);
			}

			while (captures) {
				int targetSquare = captures.getLS1BIndex();

				// Checking for promotion
				if (lastRank.IsSet(targetSquare)) {
					board.AddMove(Move(square, targetSquare, knightPromoCapture));
					board.AddMove(Move(square, targetSquare, bishopPromoCapture));
					board.AddMove(Move(square, targetSquare, rookPromoCapture));
					board.AddMove(Move(square, targetSquare, queenPromoCapture));
				} else {
					board.AddMove(Move(square, targetSquare, capture));
				}

				captures.PopBit(targetSquare);
			}

			pawns.PopBit(square);
			continue;
		}

		// Checking for en passant
		if (board.enPassantTarget != noEPTarget) {
			if (captures.IsSet(board.enPassantTarget)) {
				// Checking if the move gives discovered check to own king
				if (IsLegalEnPassant(board, square)) {
					board.AddMove(Move(square, board.enPassantTarget, epCapture));
				}
			}
		}

		// Removing own pieces from capture mask
		captures &= board.colors[!color];

		while (pushes) {
			int pushSquare = pushes.getLS1BIndex();

			// Checking for promotion
			if (lastRank.IsSet(pushSquare)) {
				board.AddMove(Move(square, pushSquare, knightPromotion));
				board.AddMove(Move(square, pushSquare, bishopPromotion));
				board.AddMove(Move(square, pushSquare, rookPromotion));
				board.AddMove(Move(square, pushSquare, queenPromotion));
			} else {
				if (abs(square - pushSquare) == 16) {
					board.AddMove(Move(square, pushSquare, doublePawnPush));
				} else {
					board.AddMove(Move(square, pushSquare, quiet));
				}
			}

			pushes.PopBit(pushSquare);
		}

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			// Checking for promotion
			if (lastRank.IsSet(targetSquare)) {
				board.AddMove(Move(square, targetSquare, knightPromoCapture));
				board.AddMove(Move(square, targetSquare, bishopPromoCapture));
				board.AddMove(Move(square, targetSquare, rookPromoCapture));
				board.AddMove(Move(square, targetSquare, queenPromoCapture));
			} else {
				board.AddMove(Move(square, targetSquare, capture));
			}

			captures.PopBit(targetSquare);
		}

		pawns.PopBit(square);
	}
}

static void GenKnightMoves(Board &board, bool color) {
	Bitboard knights = board.colors[color] & board.pieces[Knight];

	while (knights) {
		int square = knights.getLS1BIndex();

		if (IsPinned(board, square)) {
			knights.PopBit(square);
			continue;
		}

		Bitboard moves = knightAttacks[square] & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		while (moves) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
		}

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		knights.PopBit(square);
	}
}

static void GenRookMoves(Board &board, bool color) {
	Bitboard rooks = board.colors[color] & board.pieces[Rook];

	while (rooks) {
		int square = rooks.getLS1BIndex();

		Bitboard moves = getRookAttack(square, board.occupied) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			rooks.PopBit(square);
			continue;
		}


		while (moves) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
		}

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		rooks.PopBit(square);
	}
}

static void GenBishopMoves(Board &board, bool color) {
	Bitboard bishops = board.colors[color] & board.pieces[Bishop];

	while (bishops) {
		int square = bishops.getLS1BIndex();

		Bitboard moves = getBishopAttack(square, board.occupied) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			bishops.PopBit(square);
			continue;
		}


		while (moves) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
		}

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		bishops.PopBit(square);
	}
}

static void GenQueenMoves(Board &board, bool color) {
	Bitboard queens = board.colors[color] & board.pieces[Queen];

	while (queens) {
		int square = queens.getLS1BIndex();

		Bitboard moves = getQueenAttack(square, board.occupied) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			queens.PopBit(square);
			continue;
		}


		while (moves) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
		}

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		queens.PopBit(square);
	}
}

static void GenKingMoves(Board &board, bool color, bool isInCheck) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();

	Bitboard moves = (kingAttacks[kingSquare] & ~board.GetAttackMaps(!color)) & ~board.colors[color];
	Bitboard captures = moves & board.colors[!color];
	moves &= ~board.colors[!color];

	while (moves) {
		int square = moves.getLS1BIndex();

		board.AddMove(Move(kingSquare, square, quiet));

		moves.PopBit(square);
	}

	while (captures) {
		int square = captures.getLS1BIndex();

		Move move(kingSquare, square, capture);
		board.AddMove(move);

		captures.PopBit(square);
	}

	// Generating castles
    int kingRight = color ? blackKingRight : whiteKingRight;
    int queenRight = color ? blackQueenRight : whiteQueenRight;

    if (board.castlingRights & queenRight)  {
        U64 QueenSide = color ? 0x1f00000000000000 : 0x1f;
        U64 mask = color ? 0x1c00000000000000 : 0x1c;
        int targetSquare = color ? c8 : c1;
        if ((Bitboard(QueenSide) & board.occupied).PopCount() == 2
                && !(mask & board.GetAttackMaps(!color))) {
            board.AddMove(Move(kingSquare, targetSquare, queenCastle));
        }
    }

    if (board.castlingRights & kingRight) {
        U64 KingSide = color ? 0xf000000000000000 : 0xf0;
        U64 mask = color ? 0x7000000000000000 : 0x70;
        int targetSquare = color ? g8 : g1;
        if ((Bitboard(KingSide) & board.occupied).PopCount() == 2
                && !(mask & board.GetAttackMaps(!color))) {
            board.AddMove(Move(kingSquare, targetSquare, kingCastle));
        }
    }
}

// Generates moves to get out of check
static void GenCheckEvasions(Board &board, bool color) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();

	// Adding moves that step out of check
	GenKingMoves(board, color, true);

	Bitboard captureMask = Checkers(board, color); // Checker locations
	Bitboard pushMask; // Where blockers can move to
	if (captureMask.PopCount() == 1) {
		int checkerSquare = captureMask.getLS1BIndex();
		int checkerType = board.GetPieceType(checkerSquare);
		
		if (checkerType == Rook || checkerType == Bishop || checkerType == Queen) {
			pushMask = SliderRayToSquare(checkerSquare, kingSquare);
			pushMask.PopBit(kingSquare);
			pushMask.PopBit(checkerSquare);
		}
		
		// looping through own pieces to add captures and blocks
		for (int type = Pawn; type < King; type++) {
			Bitboard currentPiece = board.colors[color] & board.pieces[type];

			while (currentPiece) {
				int square = currentPiece.getLS1BIndex();

				if (IsPinned(board, square)) {
					currentPiece.PopBit(square);
					continue;
				}

				Bitboard captures;
				Bitboard pushes;

				if (type == Pawn) {
					pushes |= getPawnPushes(square, color, board.occupied) & pushMask;
					// Checking for en passant capture
					if (board.enPassantTarget != noEPTarget) {
						Bitboard attackedSquare = pawnAttacks[color][square]
							& Bitboard::GetSquare(board.enPassantTarget);
						if (attackedSquare) {
							// Checking if it's legal
							if (IsLegalEnPassant(board, square)) {
								int direction = color ? south : north;
								int targetSquare = captureMask.getLS1BIndex() + direction;
								captureMask.SetBit(targetSquare);
								if ((attackedSquare & pushMask)
									|| (attackedSquare & captureMask)) {
									board.AddMove(Move(square, board.enPassantTarget, epCapture));
								}
								captureMask.PopBit(targetSquare);
							}
						}
					}
				} else {
					pushes |= getPieceAttacks(square, type, color, board.occupied) & pushMask;
				}

				captures |= getPieceAttacks(square, type, color, board.occupied) & captureMask;

				Bitboard lastRank = color ? ranks[r_1] : ranks[r_8];

				while (captures) {
					int captureSquare = captures.getLS1BIndex();

					// Checking for promotion
					if (type == Pawn) {
						if (lastRank.IsSet(captureSquare)) {
							board.AddMove(Move(square, captureSquare, knightPromoCapture));
							board.AddMove(Move(square, captureSquare, bishopPromoCapture));
							board.AddMove(Move(square, captureSquare, rookPromoCapture));
							board.AddMove(Move(square, captureSquare, queenPromoCapture));
						} else {
							board.AddMove(Move(square, captureSquare, capture));
						}
					} else {
						board.AddMove(Move(square, captureSquare, capture));
					}

					captures.PopBit(captureSquare);
				}

				while (pushes) {
					int pushSquare = pushes.getLS1BIndex();

					if (type == Pawn) {
						// Checking for promotion
						if (lastRank.IsSet(pushSquare)) {
							board.AddMove(Move(square, pushSquare, knightPromotion));
							board.AddMove(Move(square, pushSquare, bishopPromotion));
							board.AddMove(Move(square, pushSquare, rookPromotion));
							board.AddMove(Move(square, pushSquare, queenPromotion));
						} else {
							if (abs(square - pushSquare) == 16) {
								board.AddMove(Move(square, pushSquare, doublePawnPush));
							} else {
								board.AddMove(Move(square, pushSquare, quiet));
							}
						}
					} else {
						board.AddMove(Move(square, pushSquare, quiet));
					}

					pushes.PopBit(pushSquare);
				}

				currentPiece.PopBit(square);
			}
		}
	} else { // We can only step out of double check
		return;
	}
}

void GenAttackMaps(Board &board) {
	for (int color = White; color <= Black; color++) {
		for (int type = Pawn; type <= King; type++) {
			board.attackMaps[color][type] = 0ULL;
			Bitboard pieces = board.pieces[type] & board.colors[color];

			while (pieces) {
				int square = pieces.getLS1BIndex();
				if (type == Pawn) {
					board.attackMaps[color][type] |= pawnAttacks[color][square];
				} else if (type == Knight) {
					board.attackMaps[color][type] |= knightAttacks[square];
				} else if (type == King) {
					board.attackMaps[color][type] |= kingAttacks[square];
				} else if (type == Rook) {
					board.attackMaps[color][type] |= getRookAttack(square, board.occupied 
						& ~(board.colors[!color] & board.pieces[King]));
				} else if (type == Bishop) {
					board.attackMaps[color][type] |= getBishopAttack(square, board.occupied 
						& ~(board.colors[!color] & board.pieces[King]));
				} else if (type == Queen) {
					board.attackMaps[color][type] |= getQueenAttack(square, board.occupied 
						& ~(board.colors[!color] & board.pieces[King]));
				}
				pieces.PopBit(square);
			}
		}
	}
}

void GenerateMoves(Board &board) {
    board.ResetMoves();

	if (board.InCheck()) {
		GenCheckEvasions(board, board.sideToMove);
		return;
	}

	GenPawnMoves(board, board.sideToMove);
	GenKnightMoves(board, board.sideToMove);
	GenKingMoves(board, board.sideToMove, false);
	GenBishopMoves(board, board.sideToMove);
	GenRookMoves(board, board.sideToMove);
	GenQueenMoves(board, board.sideToMove);
}
}