template <MovegenMode mode>
void GenPawnMoves(Board &board) {
	Bitboard pawns = board.pieces[Pawn] & board.colors[board.sideToMove];
	
	while (pawns) {
		int square = pawns.getLS1BIndex();

		Bitboard pushes = getPawnPushes(square, board.sideToMove, board.occupied);
		Bitboard captures = pawnAttacks[board.sideToMove][square] & board.colors[!board.sideToMove];

		Bitboard lastRank = board.sideToMove ? ranks[r_1] : ranks[r_8];

		if constexpr (mode == Noisy) {
			pushes &= lastRank;

			if (board.enPassantTarget != noEPTarget) {
				if (captures.IsSet(board.enPassantTarget)) {
					board.AddMove(Move(square, board.enPassantTarget, epCapture));
				}
			}

			while (captures) {
				int targetSquare = captures.getLS1BIndex();
				
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

			while (pushes) {
				int pushSquare = pushes.getLS1BIndex();

				// Only promotions because NOISY
				board.AddMove(Move(square, pushSquare, knightPromotion));
				board.AddMove(Move(square, pushSquare, bishopPromotion));
				board.AddMove(Move(square, pushSquare, rookPromotion));
				board.AddMove(Move(square, pushSquare, queenPromotion));

				pushes.PopBit(pushSquare);
			}

			pawns.PopBit(square);
			continue;
		} else {
			// Checking for en passant
			if (board.enPassantTarget != noEPTarget) {
				if (captures.IsSet(board.enPassantTarget)) {
					board.AddMove(Move(square, board.enPassantTarget, epCapture));
				}
			}

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
		}

		pawns.PopBit(square);
	}
}
template <MovegenMode mode>
void GenKnightMoves(Board &board) {
	Bitboard knights = board.colors[board.sideToMove] & board.pieces[Knight];

	while (knights) {
		int square = knights.getLS1BIndex();

		Bitboard moves = knightAttacks[square] & ~board.colors[board.sideToMove];
		Bitboard captures = moves & board.colors[!board.sideToMove];

		if constexpr (mode == Noisy) {
			while (captures) {
				int targetSquare = captures.getLS1BIndex();
	
				board.AddMove(Move(square, targetSquare, capture));
	
				captures.PopBit(targetSquare);

				captures.PopBit(targetSquare);
			}

			knights.PopBit(square);
			continue;
		} else {
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
}
template <MovegenMode mode>
void GenRookMoves(Board &board) {
}
template <MovegenMode mode>
void GenBishopMoves(Board &board) {
}
template <MovegenMode mode>
void GenQueenMoves(Board &board) {
}
template <MovegenMode mode>
void GenKingMoves(Board &board) {
}

template <MovegenMode mode>
void GenerateMoves(Board &board) {
    board.ResetMoves();

	GenPawnMoves<mode>(board);
	GenKnightMoves<mode>(board);
	GenKingMoves<mode>(board);
	GenBishopMoves<mode>(board);
	GenRookMoves<mode>(board);
	GenQueenMoves<mode>(board);
};