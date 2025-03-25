template <MovegenMode mode>
void GenPawnMoves(Board &board) {
	Bitboard pawns = board.pieces[Pawn] & board.colors[board.sideToMove];
	
	while (pawns) {
		int square = pawns.getLS1BIndex();

		Bitboard pushes = getPawnPushes(square, board.sideToMove, board.occupied);
		Bitboard attacks = pawnAttacks[board.sideToMove][square];
		Bitboard captures = attacks & board.colors[!board.sideToMove];

		Bitboard lastRank = board.sideToMove ? ranks[r_1] : ranks[r_8];

		if constexpr (mode == Noisy) {
			pushes &= lastRank;

			if (board.enPassantTarget != noEPTarget) {
				if (attacks.IsSet(board.enPassantTarget)) {
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
				if (attacks.IsSet(board.enPassantTarget)) {
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
					if (std::abs(square - pushSquare) == 16) {
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
		moves &= ~captures;

		if constexpr (mode == Noisy) {
			while (captures) {
				int targetSquare = captures.getLS1BIndex();
	
				board.AddMove(Move(square, targetSquare, capture));

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
	
		}
		knights.PopBit(square);
	}
}
template <MovegenMode mode>
void GenRookMoves(Board &board) {
	Bitboard rooks = board.colors[board.sideToMove] & board.pieces[Rook];

	while (rooks) {
		int square = rooks.getLS1BIndex();

		Bitboard moves = getRookAttack(square, board.occupied) & ~board.colors[board.sideToMove];
		Bitboard captures = moves & board.colors[!board.sideToMove];
		moves &= ~captures;

		if constexpr (mode == Noisy) {
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
template <MovegenMode mode>
void GenBishopMoves(Board &board) {
	Bitboard bishops = board.colors[board.sideToMove] & board.pieces[Bishop];

	while (bishops) {
		int square = bishops.getLS1BIndex();

		Bitboard moves = getBishopAttack(square, board.occupied) & ~board.colors[board.sideToMove];
		Bitboard captures = moves & board.colors[!board.sideToMove];
		moves &= ~captures;

		if constexpr (mode == Noisy) {
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
template <MovegenMode mode>
void GenQueenMoves(Board &board) {
	Bitboard queens = board.colors[board.sideToMove] & board.pieces[Queen];

	while (queens) {
		int square = queens.getLS1BIndex();

		Bitboard moves = getQueenAttack(square, board.occupied) & ~board.colors[board.sideToMove];
		Bitboard captures = moves & board.colors[!board.sideToMove];
		moves &= ~captures;

		if constexpr (mode == Noisy) {
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
template <MovegenMode mode>
void GenKingMoves(Board &board) {
	int kingSquare = (board.colors[board.sideToMove] & board.pieces[King]).getLS1BIndex();

	Bitboard moves = (kingAttacks[kingSquare] & ~board.colors[board.sideToMove]);
	Bitboard captures = moves & board.colors[!board.sideToMove];
	moves &= ~board.colors[!board.sideToMove];

	if constexpr (mode == Noisy) {
		while (captures) {
			int square = captures.getLS1BIndex();
	
			Move move(kingSquare, square, capture);
			board.AddMove(move);
	
			captures.PopBit(square);
		}
		return;
	}

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
    int kingRight = board.sideToMove ? blackKingRight : whiteKingRight;
    int queenRight = board.sideToMove ? blackQueenRight : whiteQueenRight;

    if (board.castlingRights & queenRight)  {
        U64 QueenSide = board.sideToMove ? 0x1f00000000000000 : 0x1f;
        U64 mask = board.sideToMove ? 0x1c00000000000000 : 0x1c;
        int targetSquare = board.sideToMove ? c8 : c1;
        if ((Bitboard(QueenSide) & board.occupied).PopCount() == 2
                && !(mask & board.GetThreatMaps(!board.sideToMove))) {
            board.AddMove(Move(kingSquare, targetSquare, queenCastle));
        }
    }

    if (board.castlingRights & kingRight) {
        U64 KingSide = board.sideToMove ? 0xf000000000000000 : 0xf0;
        U64 mask = board.sideToMove ? 0x7000000000000000 : 0x70;
        int targetSquare = board.sideToMove ? g8 : g1;
        if ((Bitboard(KingSide) & board.occupied).PopCount() == 2
                && !(mask & board.GetThreatMaps(!board.sideToMove))) {
            board.AddMove(Move(kingSquare, targetSquare, kingCastle));
        }
    }
}

template <MovegenMode mode>
void GenerateMoves(Board &board) {
    board.ResetMoves();
    board.threatMaps = std::array<std::array<U64, 64>, 2>();
    GenThreatMaps(board);

	GenPawnMoves<mode>(board);
	GenKnightMoves<mode>(board);
	GenKingMoves<mode>(board);
	GenBishopMoves<mode>(board);
	GenRookMoves<mode>(board);
	GenQueenMoves<mode>(board);
};
