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
		Bitboard targets = getPieceAttacks(kingSquare, type, color, board.occupied.GetBoard());
		checkerBoard |= targets & board.pieces[type] & board.colors[!color];
	}
	return checkerBoard;
}

// Returns the direction to target from square
static int GetDirection(int square, int target) {
	int difference = target - square;

	if (difference % north == 0) {
		return north;
	} else if (difference % south == 0) {
		return south;
	} else if (difference % noWe == 0) {
		return noWe;
	} else if (difference % noEa == 0) {
		return noEa;
	} else if (difference % soWe == 0) {
		return soWe;
	} else if (difference % soEa == 0) {
		return soEa;
	} else if (difference < 0 && difference > -8) {
		return west;
	} else if (difference > 0 && difference < 8) {
		return east;
	}

	return 0;
}

// Returns a bitboard of the squares from attacker to target
static Bitboard SliderRaysToSquare(int attacker, int target) {
	Bitboard ray;
	
	int direction = GetDirection(attacker, target);
	
	if (direction) {
		while (attacker <= target) {
			ray.SetBit(attacker);
			attacker += direction;
		}
	}

	return ray;
}

static Bitboard GetPinningRay(Board &board, int square, int direction) {
	Bitboard ray;

	if (direction) {
		while (square < 64 && square >= 0) {
			ray.SetBit(square);
			if (board.occupied.IsSet(square)) {
				int attackerColor = board.GetPieceColor(square);
				int attackerType = board.GetPieceType(square);

				if (attackerColor == !board.sideToMove && (attackerType == Rook
					|| attackerType == Bishop || attackerType == Queen)) {
					return ray;
				}
				return 0ULL;
			}
			square += direction;
		}
	}

	return 0ULL;
}

static bool IsPinned(Board &board, int square, int color) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
	int directionToKing = GetDirection(square, kingSquare);

	if (SliderRaysToSquare(square, kingSquare).PopCount() != 2) {
		return false;
	}

	int currentSquare = square - directionToKing;
	
	return GetPinningRay(board, currentSquare, -directionToKing).GetBoard();
}

static bool IsLegalEnPassant(Board &board, int square) {
	Bitboard kingSquare = board.colors[board.sideToMove] & board.pieces[King];
	int pushDirection = board.sideToMove ? south : north;
	Bitboard newOccupancy = board.occupied;
	newOccupancy.PopBit(square);
	newOccupancy.PopBit(board.enPassantTarget - pushDirection);

	// Looking for horizontal checks
	if (!(kingSquare.GetBoard() & files[A])) {
		int currentSq = kingSquare.getLS1BIndex() + east;

		while (!Bitboard(files[A]).IsSet(currentSq)) {
			if (newOccupancy.IsSet(currentSq)) {
				int attackerColor = board.GetPieceColor(currentSq);
				int attackerType = board.GetPieceType(currentSq);

				if (attackerColor != board.sideToMove && (attackerType == Rook
					|| attackerType == Bishop || attackerType == Queen)) {
					return false;
				}
				return true;
			}
			currentSq += east;
		}
	}

	if (!(kingSquare.GetBoard() & files[H])) {
		int currentSq = kingSquare.getLS1BIndex() + east;

		while (!Bitboard(files[A]).IsSet(currentSq)) {
			if (newOccupancy.IsSet(currentSq)) {
				int attackerColor = board.GetPieceColor(currentSq);
				int attackerType = board.GetPieceType(currentSq);

				if (attackerColor != board.sideToMove && (attackerType == Rook
					|| attackerType == Bishop || attackerType == Queen)) {
					return false;
				}
				return true;
			}
			currentSq += east;
		}
	}

	return true;
}

static void GenPawnMoves(Board &board, bool color) {
	Bitboard pawns = board.colors[color] & board.pieces[Pawn];

	while (pawns.GetBoard()) {
		int square = pawns.getLS1BIndex();

		Bitboard pushes = getPawnPushes(square, color, board.occupied);
		Bitboard captures = pawnAttacks[color][square];

		if (IsPinned(board, square, color)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square - directionToKing, -directionToKing);

			pushes &= pinningRay;
			captures &= pinningRay;

			// Checking for en passant
			if (captures.IsSet(board.enPassantTarget)) {
				if (IsLegalEnPassant(board, square)) {
					board.AddMove(Move(square, board.enPassantTarget, epCapture));
				}
			}

			// Removing own pieces from capture mask
			captures &= board.colors[!color];

			while (pushes.GetBoard()) {
				int pushSquare = pushes.getLS1BIndex();

				board.AddMove(Move(square, pushSquare, quiet));

				pushes.PopBit(pushSquare);
			}

			while (captures.GetBoard()) {
				int targetSquare = pushes.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			pawns.PopBit(square);
			continue;
		}

		// Checking for en passant
		if (captures.IsSet(board.enPassantTarget)) {
			// Checking if the move gives discovered check to own king
			if (IsLegalEnPassant(board, square)) {
				board.AddMove(Move(square, board.enPassantTarget, epCapture));
			}
		}

		// Removing own pieces from capture mask
		captures &= board.colors[!color];

		while (pushes.GetBoard()) {
			int pushSquare = pushes.getLS1BIndex();

			board.AddMove(Move(square, pushSquare, quiet));

			pushes.PopBit(pushSquare);
		}

		while (captures.GetBoard()) {
			int targetSquare = pushes.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		pawns.PopBit(square);
	}
}

static void GenKnightMoves(Board &board, bool color) {
	Bitboard knights = board.colors[color] & board.pieces[Knight];

	while (knights.GetBoard()) {
		int square = knights.getLS1BIndex();

		if (IsPinned(board, square, color)) {
			knights.PopBit(square);
			continue;
		}

		Bitboard moves = knightAttacks[square] & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		while (moves.GetBoard()) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
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

		Bitboard moves = getRookAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square, color)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square - directionToKing, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves.GetBoard()) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures.GetBoard()) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			rooks.PopBit(square);
			continue;
		}


		while (moves.GetBoard()) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
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

		Bitboard moves = getBishopAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square, color)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square - directionToKing, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves.GetBoard()) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures.GetBoard()) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			bishops.PopBit(square);
			continue;
		}


		while (moves.GetBoard()) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
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

		Bitboard moves = getQueenAttack(square, board.occupied.GetBoard()) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		moves &= ~captures;

		if (IsPinned(board, square, color)) {
			int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();
			int directionToKing = GetDirection(square, kingSquare);

			Bitboard pinningRay = GetPinningRay(board, square - directionToKing, -directionToKing);

			moves &= pinningRay;
			captures &= pinningRay;

			while (moves.GetBoard()) {
				int targetSquare = moves.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, quiet));

				moves.PopBit(targetSquare);
			}

			while (captures.GetBoard()) {
				int targetSquare = captures.getLS1BIndex();

				board.AddMove(Move(square, targetSquare, capture));

				captures.PopBit(targetSquare);
			}

			queens.PopBit(square);
			continue;
		}


		while (moves.GetBoard()) {
			int targetSquare = moves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			moves.PopBit(targetSquare);
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

// Generates moves to get out of check
static void GenCheckEvasions(Board &board, bool color) {
	int kingSquare = (board.colors[color] & board.pieces[King]).getLS1BIndex();

	// Adding moves that step out of check
	GenKingMoves(board, color);

	Bitboard captureMask = Checkers(board, color); // Checker locations
	Bitboard pushMask; // Where blockers can move to
	if (captureMask.PopCount() == 1) {
		int checkerSquare = captureMask.getLS1BIndex();
		int checkerType = board.GetPieceType(checkerSquare);
		
		if (checkerType == Rook || checkerType == Bishop || checkerType == Queen) {
			pushMask = SliderRaysToSquare(checkerSquare, kingSquare)
				& ~(Bitboard::GetSquare(kingSquare) | Bitboard::GetSquare(checkerSquare));
		}

		// looping through own pieces to add captures and blocks
		for (int type = Pawn; type < King; type++) {
			Bitboard currentPiece = board.colors[color] & board.pieces[type];

			while (currentPiece.GetBoard()) {
				int square = currentPiece.getLS1BIndex();

				if (IsPinned(board, square, color)) {
					currentPiece.PopBit(square);
					continue;
				}

				Bitboard captures;
				Bitboard pushes;

				if (type == Pawn) {
					pushes |= getPawnPushes(square, color, board.occupied) & pushMask;

					// Checking for en passant capture
					if ((pawnAttacks[color][square]
						& Bitboard::GetSquare(board.enPassantTarget)).GetBoard()) {
						board.AddMove(Move(square, board.enPassantTarget, epCapture));
					}
				}

				pushes |= getPieceAttacks(square, type, color, board.occupied.GetBoard()) & pushMask;
				captures |= getPieceAttacks(square, type, color, board.occupied.GetBoard()) & captureMask;

				while (captures.GetBoard()) {
					int captureSquare = captures.getLS1BIndex();

					board.AddMove(Move(square, captureSquare, capture));

					captures.PopBit(captureSquare);
				}

				while (pushes.GetBoard()) {
					int pushSquare = pushes.getLS1BIndex();

					board.AddMove(Move(square, pushSquare, quiet));

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

void GenerateMoves(Board &board, bool side) {
	board.moveList.clear();

	if (board.InCheck(side)) {
		GenCheckEvasions(board, side);
		return;
	}

	GenPawnMoves(board, side);

	GenKnightMoves(board, side);

	GenRookMoves(board, side);

	GenBishopMoves(board, side);

	GenQueenMoves(board, side);

	GenKingMoves(board, side);
}