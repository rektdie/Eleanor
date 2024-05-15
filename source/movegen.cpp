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

Bitboard maskKnightAttacks(int square) {
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

Bitboard maskKingAttacks(int square) {
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

static void GenPawnMoves(Board &board, bool color) {
    Bitboard pawns = board.pieces[Pawn] & board.colors[color];

    while (pawns) {
        int square = pawns.getLS1BIndex();
        Bitboard seventhRank = color ? ranks[r_2] : ranks[r_7];

        Bitboard pushes = getPawnPushes(square, color, board.occupied);
        Bitboard captures = pawnAttacks[color][square] & board.colors[!color];

        if (seventhRank.IsSet(square)) {
            while (pushes) {
                int targetSquare = pushes.getLS1BIndex();

                board.AddMove(Move(square, targetSquare, queenPromotion));
                board.AddMove(Move(square, targetSquare, rookPromotion));
                board.AddMove(Move(square, targetSquare, bishopPromotion));
                board.AddMove(Move(square, targetSquare, knightPromotion));

                pushes.PopBit(targetSquare);
            }

            while (captures) {
                int targetSquare = captures.getLS1BIndex();

                board.AddMove(Move(square, targetSquare, queenPromoCapture));
                board.AddMove(Move(square, targetSquare, rookPromoCapture));
                board.AddMove(Move(square, targetSquare, bishopPromoCapture));
                board.AddMove(Move(square, targetSquare, knightPromoCapture));

                captures.PopBit(targetSquare);
            }
        } else {
            while (pushes) {
                int targetSquare = pushes.getLS1BIndex();

                // Checking for double push
                if (abs(targetSquare - square) == 16) {
                    board.AddMove(Move(square, targetSquare, doublePawnPush));
                } else {
                    board.AddMove(Move(square, targetSquare, quiet));
                }

                pushes.PopBit(targetSquare);
            }

            while (captures) {
                int targetSquare = captures.getLS1BIndex();

                board.AddMove(Move(square, targetSquare, capture));

                captures.PopBit(targetSquare);
            }

            // Checking for en passant
            if (pawnAttacks[color][square].IsSet(board.enPassantTarget)) {
                board.AddMove(Move(square, board.enPassantTarget, epCapture));
            }
        }

        pawns.PopBit(square);
    }
}

static void GenKnightMoves(Board &board, bool color) {
	Bitboard knights = board.colors[color] & board.pieces[Knight];

	while (knights) {
		int square = knights.getLS1BIndex();

		Bitboard moves = knightAttacks[square] & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		Bitboard quietMoves = moves & ~board.colors[!color];

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		while (quietMoves) {
			int targetSquare = quietMoves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			quietMoves.PopBit(targetSquare);
		}

		knights.PopBit(square);
    }	
}

static void GenRookMoves(Board &board, bool color) {
    Bitboard rooks = board.pieces[Rook] & board.colors[color];

    while (rooks) {
        int square = rooks.getLS1BIndex();

		Bitboard moves = getRookAttack(square, board.occupied) & ~board.colors[color];
		Bitboard captures = moves & board.colors[!color];
		Bitboard quietMoves = moves & ~board.colors[!color];

		while (captures) {
			int targetSquare = captures.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, capture));

			captures.PopBit(targetSquare);
		}

		while (quietMoves) {
			int targetSquare = quietMoves.getLS1BIndex();

			board.AddMove(Move(square, targetSquare, quiet));

			quietMoves.PopBit(targetSquare);
		}
        
        rooks.PopBit(square);
    }
}

static void GenBishopMoves(Board &board, bool color) {
    Bitboard bishops = board.pieces[Bishop] & board.colors[color];

    while (bishops) {
        int square = bishops.getLS1BIndex();

        Bitboard moves = getBishopAttack(square, board.occupied) & ~board.colors[color];
        Bitboard captures = moves & board.colors[!color];
        Bitboard quietMoves = moves & ~board.colors[!color];

        while (captures) {
            int targetSquare = captures.getLS1BIndex();

            board.AddMove(Move(square, targetSquare, capture));

            captures.PopBit(targetSquare);
        }

        while (quietMoves) {
            int targetSquare = quietMoves.getLS1BIndex();

            board.AddMove(Move(square, targetSquare, quiet));

            quietMoves.PopBit(targetSquare);
        }

        bishops.PopBit(square);
    }
}

static void GenQueenMoves(Board &board, bool color) {
    Bitboard queens = board.pieces[Queen] & board.colors[color];

    while (queens) {
        int square = queens.getLS1BIndex();

        Bitboard moves = getQueenAttack(square, board.occupied) & ~board.colors[color];
        Bitboard captures = moves & board.colors[!color];
        Bitboard quietMoves = moves & ~board.colors[!color];

        while (captures) {
            int targetSquare = captures.getLS1BIndex();

            board.AddMove(Move(square, targetSquare, capture));

            captures.PopBit(targetSquare);
        }

        while (quietMoves) {
            int targetSquare = quietMoves.getLS1BIndex();

            board.AddMove(Move(square, targetSquare, quiet));

            quietMoves.PopBit(targetSquare);
        }

        queens.PopBit(square);
    }
}

static void GenKingMoves(Board &board, bool color) {
    int kingSquare = (board.pieces[King] & board.colors[color]).getLS1BIndex();

    Bitboard moves = kingAttacks[kingSquare] & ~board.colors[color];
    Bitboard captures = moves & board.colors[!color];
    Bitboard quietMoves = moves & ~board.colors[!color];

    while (captures) {
        int targetSquare = captures.getLS1BIndex();

        board.AddMove(Move(kingSquare, targetSquare, capture));

        captures.PopBit(targetSquare);
    }

    while (quietMoves) {
        int targetSquare = quietMoves.getLS1BIndex();

        board.AddMove(Move(kingSquare, targetSquare, quiet));

        quietMoves.PopBit(targetSquare);
    }

    // Checking for king side castle
    if (board.castlingRights[color * 2]) {
        Bitboard mask = color ? 0xe000000000000000 : 0xe0;

        if ((board.occupied & mask).PopCount() == 1) {
            board.AddMove(Move(kingSquare, kingSquare + 2, kingCastle));
        }
    }

    // Checking for quuen side castle
    if (board.castlingRights[color * 2 + 1]) {
        Bitboard mask = color ? 0xf00000000000000 : 0xf;

        if ((board.occupied & mask).PopCount() == 1) {
            board.AddMove(Move(kingSquare, kingSquare - 2, queenCastle));
        }
    }
}

void GenerateMoves(Board &board) {
	bool side = board.sideToMove;
    board.ResetMoves();

	GenPawnMoves(board, side);
    GenKnightMoves(board, side);
	GenKingMoves(board, side);
	GenBishopMoves(board, side);
	GenRookMoves(board, side);
	GenQueenMoves(board, side);
}
