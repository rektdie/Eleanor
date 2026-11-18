inline bool IsEPLegal(Board &board, int from, int kingSquare) {
	const bool us = board.sideToMove;
	const int to = board.enPassantTarget;
	const int captured = to + (us ? north : south);

	Bitboard occ = board.occupied ^ Bitboard::GetSquare(from) ^ Bitboard::GetSquare(to) ^ Bitboard::GetSquare(captured);
	Bitboard theirQueens = board.pieces[Queen] & board.colors[!us];

	return !(getBishopAttack(kingSquare, occ) & (theirQueens | (board.pieces[Bishop] & board.colors[!us])))
		&& !(getRookAttack(kingSquare, occ) & (theirQueens | (board.pieces[Rook] & board.colors[!us])));
}

template <MovegenMode mode>
void GenPawnMoves(Board &board, Bitboard checkMask, int kingSquare) {
	const bool us = board.sideToMove;
	Bitboard pawns = board.pieces[Pawn] & board.colors[us];
	Bitboard lastRank = us ? ranks[r_1] : ranks[r_8];

	while (pawns) {
		int square = pawns.PopLSB();

		Bitboard legalMask = checkMask;
		if (board.pinned.IsSet(square)) legalMask &= lineBB[kingSquare][square];

		Bitboard pushes = getPawnPushes(square, us, board.occupied) & legalMask;

		if constexpr (mode == Noisy || mode == All) {
			Bitboard attacks = pawnAttacks[us][square];
			Bitboard captures = attacks & board.colors[!us] & legalMask;
			Bitboard noisyPushes = pushes & lastRank;

			if (board.enPassantTarget != noEPTarget && attacks.IsSet(board.enPassantTarget)
					&& IsEPLegal(board, square, kingSquare)) {
				board.AddMove(Move(square, board.enPassantTarget, epCapture));
			}

			while (captures) {
				int targetSquare = captures.PopLSB();

				if (lastRank.IsSet(targetSquare)) {
					board.AddMove(Move(square, targetSquare, knightPromoCapture));
					board.AddMove(Move(square, targetSquare, bishopPromoCapture));
					board.AddMove(Move(square, targetSquare, rookPromoCapture));
					board.AddMove(Move(square, targetSquare, queenPromoCapture));
				} else {
					board.AddMove(Move(square, targetSquare, capture));
				}
			}

			while (noisyPushes) {
				int pushSquare = noisyPushes.PopLSB();

				board.AddMove(Move(square, pushSquare, knightPromotion));
				board.AddMove(Move(square, pushSquare, bishopPromotion));
				board.AddMove(Move(square, pushSquare, rookPromotion));
				board.AddMove(Move(square, pushSquare, queenPromotion));
			}
		}

		if constexpr (mode == Quiet || mode == All) {
			Bitboard quietPushes = pushes & ~lastRank;

			while (quietPushes) {
				int pushSquare = quietPushes.PopLSB();

				if (std::abs(square - pushSquare) == 16) {
					board.AddMove(Move(square, pushSquare, doublePawnPush));
				} else {
					board.AddMove(Move(square, pushSquare, quiet));
				}
			}
		}
	}
}

template <MovegenMode mode>
void GenKnightMoves(Board &board, Bitboard checkMask) {
	const bool us = board.sideToMove;
	Bitboard knights = board.colors[us] & board.pieces[Knight] & ~board.pinned;

	while (knights) {
		int square = knights.PopLSB();

		Bitboard moves = knightAttacks[square] & ~board.colors[us] & checkMask;

		if constexpr (mode == Noisy || mode == All) {
			Bitboard captures = moves & board.colors[!us];

			while (captures) {
				int targetSquare = captures.PopLSB();
				board.AddMove(Move(square, targetSquare, capture));
			}
		}

		if constexpr (mode == Quiet || mode == All) {
			Bitboard quiets = moves & ~board.occupied;

			while (quiets) {
				int targetSquare = quiets.PopLSB();
				board.AddMove(Move(square, targetSquare, quiet));
			}
		}
	}
}

template <MovegenMode mode>
void GenTargetMoves(Board &board, int square, Bitboard moves) {
	if constexpr (mode == Noisy || mode == All) {
		Bitboard captures = moves & board.colors[!board.sideToMove];

		while (captures) {
			int targetSquare = captures.PopLSB();
			board.AddMove(Move(square, targetSquare, capture));
		}
	}

	if constexpr (mode == Quiet || mode == All) {
		Bitboard quiets = moves & ~board.occupied;

		while (quiets) {
			int targetSquare = quiets.PopLSB();
			board.AddMove(Move(square, targetSquare, quiet));
		}
	}
}

template <MovegenMode mode>
void GenRookMoves(Board &board, Bitboard checkMask, int kingSquare) {
	const bool us = board.sideToMove;
	Bitboard rooks = board.colors[us] & board.pieces[Rook];

	while (rooks) {
		int square = rooks.PopLSB();

		Bitboard moves = getRookAttack(square, board.occupied) & ~board.colors[us] & checkMask;
		if (board.pinned.IsSet(square)) moves &= lineBB[kingSquare][square];

		GenTargetMoves<mode>(board, square, moves);
	}
}

template <MovegenMode mode>
void GenBishopMoves(Board &board, Bitboard checkMask, int kingSquare) {
	const bool us = board.sideToMove;
	Bitboard bishops = board.colors[us] & board.pieces[Bishop];

	while (bishops) {
		int square = bishops.PopLSB();

		Bitboard moves = getBishopAttack(square, board.occupied) & ~board.colors[us] & checkMask;
		if (board.pinned.IsSet(square)) moves &= lineBB[kingSquare][square];

		GenTargetMoves<mode>(board, square, moves);
	}
}

template <MovegenMode mode>
void GenQueenMoves(Board &board, Bitboard checkMask, int kingSquare) {
	const bool us = board.sideToMove;
	Bitboard queens = board.colors[us] & board.pieces[Queen];

	while (queens) {
		int square = queens.PopLSB();

		Bitboard moves = getQueenAttack(square, board.occupied) & ~board.colors[us] & checkMask;
		if (board.pinned.IsSet(square)) moves &= lineBB[kingSquare][square];

		GenTargetMoves<mode>(board, square, moves);
	}
}

template <MovegenMode mode>
void GenKingMoves(Board &board, int kingSquare) {
	const bool us = board.sideToMove;
	Bitboard moves = kingAttacks[kingSquare] & ~board.colors[us] & ~board.colorThreats[!us];

	GenTargetMoves<mode>(board, kingSquare, moves);

	if constexpr (mode == Quiet || mode == All) {
		int kingRight = us ? blackKingRight : whiteKingRight;
		int queenRight = us ? blackQueenRight : whiteQueenRight;

		if (board.castlingRights & queenRight) {
			U64 QueenSide = us ? 0x1f00000000000000 : 0x1f;
			U64 mask = us ? 0x1c00000000000000 : 0x1c;
			int targetSquare = us ? c8 : c1;
			if ((Bitboard(QueenSide) & board.occupied).PopCount() == 2
					&& !(mask & board.colorThreats[!us])) {
				board.AddMove(Move(kingSquare, targetSquare, queenCastle));
			}
		}

		if (board.castlingRights & kingRight) {
			U64 KingSide = us ? 0xf000000000000000 : 0xf0;
			U64 mask = us ? 0x7000000000000000 : 0x70;
			int targetSquare = us ? g8 : g1;
			if ((Bitboard(KingSide) & board.occupied).PopCount() == 2
					&& !(mask & board.colorThreats[!us])) {
				board.AddMove(Move(kingSquare, targetSquare, kingCastle));
			}
		}
	}
}

template <MovegenMode mode>
void GenerateMoves(Board &board, bool clear) {
	if (clear) {
		board.ResetMoves();
	}

	int kingSquare = (board.colors[board.sideToMove] & board.pieces[King]).getLS1BIndex();
	Bitboard checkMask = ~0ULL;

	if (board.checkers) {
		if (board.checkers.PopCount() > 1) {
			GenKingMoves<mode>(board, kingSquare);
			return;
		}

		checkMask = betweenBB[kingSquare][board.checkers.getLS1BIndex()] | board.checkers;
	}

	GenPawnMoves<mode>(board, checkMask, kingSquare);
	GenKnightMoves<mode>(board, checkMask);
	GenKingMoves<mode>(board, kingSquare);
	GenBishopMoves<mode>(board, checkMask, kingSquare);
	GenRookMoves<mode>(board, checkMask, kingSquare);
	GenQueenMoves<mode>(board, checkMask, kingSquare);
}
