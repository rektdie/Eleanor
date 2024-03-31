#include "board.h"
#include "movegen.h"
#include <iostream>

void Board::Init() {
	SetByFen(StartingFen);
}

void Board::Reset() {
	castlingRights = { false, false, false, false };
	enPassantTarget = 0;
	halfMoves = 0;
	fullMoves = 0;
	sideToMove = White;
	occupied = 0ULL;
	std::cout << '\n';

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();
}

void Board::SetByFen(const char* fen) {
	Reset();

	// Starting from top left
	Bitboard currentSquare = 0x100000000000000;

	int digits = 0;
	while (*fen != ' ' && *fen) {
		PIECE pieceType = Pawn;
		COLOR pieceColor = Black;

		switch (*fen) {
		case 'p':
			pieceType = Pawn;
			pieceColor = Black;
			break;
		case 'P':
			pieceType = Pawn;
			pieceColor = White;
			break;
		case 'n':
			pieceType = Knight;
			pieceColor = Black;
			break;
		case 'N':
			pieceType = Knight;
			pieceColor = White;
			break;
		case 'b':
			pieceType = Bishop;
			pieceColor = Black;
			break;
		case 'B':
			pieceType = Bishop;
			pieceColor = White;
			break;
		case 'r':
			pieceType = Rook;
			pieceColor = Black;
			break;
		case 'R':
			pieceType = Rook;
			pieceColor = White;
			break;
		case 'k':
			pieceType = King;
			pieceColor = Black;
			break;
		case 'K':
			pieceType = King;
			pieceColor = White;
			break;
		case 'q':
			pieceType = Queen;
			pieceColor = Black;
			break;
		case 'Q':
			pieceType = Queen;
			pieceColor = White;
			break;
		case '/':
			currentSquare = currentSquare >> 8 + digits;
			digits = 0;
			fen++;
			continue;
		default:
			int num = (*fen) - 48;
			fen++;
			if (*fen != '/') {
				currentSquare = currentSquare << num;
				digits += num;
			}
			continue;
		}

		pieces[pieceType] |= currentSquare;
		colors[pieceColor] |= currentSquare;

		fen++;
		if (*fen != '/') {
			digits++;
			currentSquare = currentSquare << 1;
		}
	}

	fen++; // skipping previous space
	int section = 0;

	while (*fen) {
		switch (*fen) {
		case ' ':
			fen++;
			section++;
			continue;
		case 'w':
			sideToMove = White;
			break;
		case 'b':
			sideToMove = Black;
			break;
		case 'K':
			castlingRights[White] = true;
			break;
		case 'Q':
			castlingRights[White + 1] = true;
			break;
		case 'k':
			castlingRights[Black * 2] = true;
			break;
		case 'q':
			castlingRights[Black * 2 + 1] = true;
			break;
		default:
			if (*fen != '-') {
				if (section == 2) {
					int passantFile;
					switch (*fen)
					{
					case 'a':
						passantFile = 0;
						break;
					case 'b':
						passantFile = 1;
						break;
					case 'c':
						passantFile = 2;
						break;
					case 'd':
						passantFile = 3;
						break;
					case 'e':
						passantFile = 4;
						break;
					case 'f':
						passantFile = 5;
						break;
					case 'g':
						passantFile = 6;
						break;
					case 'h':
						passantFile = 7;
						break;
					}
					fen++;
					const int passantRank = *fen - 49;
					enPassantTarget = passantFile + passantRank*8;
				}
				else if (section == 3) {
					halfMoves = (*fen) - 48;
				}
				else if (section == 4) {
					fullMoves = (*fen) - 48;
				}
			}

			break;
		}
		fen++;
	}

	occupied = (colors[White] | colors[Black]);
}

void Board::PrintBoard() {

	for (int rank = 7; rank >= 0; rank--) {

		std::cout << "+---+---+---+---+---+---+---+---+\n";
		std::cout << "| ";

		for (int file = 0; file < 8; file++) {
			int square = rank * 8 + file;
			bool pieceSet = false;

			for (int i = Pawn; i <= King; i++) {
				if ((colors[White] & pieces[i]).IsSet(square)) {
					std::cout << PIECE_LETTERS[i * 2 + White] << " | ";
					pieceSet = true;
				} else if ((colors[Black] & pieces[i]).IsSet(square)) {
					std::cout << PIECE_LETTERS[i * 2 + Black] << " | ";
					pieceSet = true;
				}
			}

			if (!pieceSet) {
				std::cout << "  | ";
			}
		}
		std::cout << ' ' << rank + 1 << '\n';
	}

	std::cout << "+---+---+---+---+---+---+---+---+\n";
	std::cout << "  a   b   c   d   e   f   g   h\n\n";
	std::cout << "      Side to move: ";
	if (!sideToMove) {
		std::cout << "White\n";
	} else {
		std::cout << "Black\n";
	}
	if (enPassantTarget != 0) {
		std::cout << "      En Passant square: " << squareCoords[enPassantTarget] << '\n';
	} else {
		std::cout << "      En Passant square: None" << '\n';
	}

	std::cout << "      Castling rights: ";
	if (castlingRights[0]) std::cout << "K"; else std::cout << "-";
	if (castlingRights[1]) std::cout << "Q"; else std::cout << "-";
	if (castlingRights[2]) std::cout << "k"; else std::cout << "-";
	if (castlingRights[3]) std::cout << "q"; else std::cout << "-";
	std::cout << '\n';
}

void Board::AddMove(Move move) {
	moveList.push_back(move);
}

void Board::ResetMoves() {
	moveList.clear();
}

void Board::ListMoves() {
	for (Move move : moveList) {
		move.PrintMove();
	}
}

int Board::GetPieceType(int square) {
	for (int piece = Pawn; piece <= King; piece++) {
		if (pieces[piece].IsSet(square)) return piece;
	}

	return -1;
}

int Board::GetPieceColor(int square) {
	return (colors[Black].IsSet(square));
}

bool Board::InCheck(bool side) {
	Bitboard myKingSquare = colors[side] & pieces[King];

	for (int square = a1; square <= h8; square++) {
		for (int piece = Pawn; piece <= King; piece++) {
			if ((colors[!side] & pieces[piece]).IsSet(square)) {
				if (piece == Pawn) {
					if ((pawnAttacks[!side][square] & myKingSquare).GetBoard()) return true;
				} else if (piece == Knight) {
					if ((knightAttacks[square] & myKingSquare).GetBoard()) return true;
				} else if (piece == Bishop) {
					if ((getBishopAttack(square, occupied.GetBoard()) & myKingSquare).GetBoard()) return true;
				} else if (piece == Rook) {
					if ((getRookAttack(square, occupied.GetBoard()) & myKingSquare).GetBoard()) return true;
				} else if (piece == Queen) {
					if ((getQueenAttack(square, occupied.GetBoard()) & myKingSquare).GetBoard()) return true;
				}
			}
		}
	}

	return false;
}

void Board::Promote(int square, int pieceType, int color, bool isCapture) {
	if (isCapture) {
		int targetType = GetPieceType(square);
		std::cout << targetType << '\n';

		pieces[targetType].PopBit(square);
		colors[!color].PopBit(square);
	}

	occupied.SetBit(square);
	pieces[pieceType].SetBit(square);
	colors[color].SetBit(square);
}

void Board::DoMove(Move move) {
	enPassantTarget = a1; // resetting (a1 is impossible)
	uint16_t moveType = move.moveFlags;
	int attackerType = GetPieceType(move.moveFrom);
	bool attackerColor = GetPieceColor(move.moveFrom);

	int targetType = GetPieceType(move.moveTo);

	int direction = attackerColor ? south : north;

	pieces[attackerType].PopBit(move.moveFrom);
	colors[attackerColor].PopBit(move.moveFrom);
	occupied.PopBit(move.moveFrom);

	if (moveType < knightPromotion) {
		pieces[attackerType].SetBit(move.moveTo);
		colors[attackerColor].SetBit(move.moveTo);
	}

	if (moveType == quiet) {
		occupied.SetBit(move.moveTo);
	} else if (moveType == doublePawnPush) {
		occupied.SetBit(move.moveTo);
		enPassantTarget = move.moveFrom + direction;
	} else if (moveType == capture || moveType == epCapture) {
		pieces[targetType].PopBit(move.moveTo);
		colors[!attackerColor].PopBit(move.moveTo);
	} else if (moveType == queenCastle) {
		int rookSquare = attackerColor ? a8 : a1;

		// Removing rook from old position
		pieces[Rook].PopBit(rookSquare);
		colors[attackerColor].PopBit(rookSquare);
		occupied.PopBit(rookSquare);

		// Setting rook on new position
		pieces[Rook].SetBit(rookSquare + 3);
		colors[attackerColor].SetBit(rookSquare + 3);
		occupied.SetBit(rookSquare);
	} else if (moveType == kingCastle) {
		int rookSquare = attackerColor ? h8 : h1;
		std::cout << squareCoords[rookSquare - 2] << '\n';
		
		// Removing rook from old position
		pieces[Rook].PopBit(rookSquare);
		colors[attackerColor].PopBit(rookSquare);
		occupied.PopBit(rookSquare);

		// Setting rook on new position
		pieces[Rook].SetBit(rookSquare - 2);
		colors[attackerColor].SetBit(rookSquare - 2);
		occupied.SetBit(rookSquare);
	} else if (moveType == knightPromotion) {
		Promote(move.moveTo, Knight, attackerColor, false);
	} else if (moveType == bishopPromotion) {
		Promote(move.moveTo, Bishop, attackerColor, false);
	} else if (moveType == rookPromotion) {
		Promote(move.moveTo, Rook, attackerColor, false);
	} else if (moveType == queenPromotion) {
		Promote(move.moveTo, Queen, attackerColor, false);
	} else if (moveType == knightPromoCapture) {
		Promote(move.moveTo, Knight, attackerColor, true);
	} else if (moveType == bishopPromoCapture) {
		Promote(move.moveTo, Bishop, attackerColor, true);
	} else if (moveType == rookPromoCapture) {
		Promote(move.moveTo, Rook, attackerColor, true);
	} else if (moveType == queenPromoCapture) {
		Promote(move.moveTo, Queen, attackerColor, true);
	}

	// Removing the right to castle on king movement
	if (attackerType == King) {
		castlingRights[attackerColor * 2] = false;
		castlingRights[attackerColor * 2 + 1] = false;
	}

	// Removing the right to castle on rook movement
	if (attackerType == Rook) {
		if ((Bitboard::GetSquare(move.moveFrom) & files[A] & (ranks[r_1] | ranks[r_8])).GetBoard()) {
			castlingRights[attackerColor * 2 + 1] = false;
		} else if ((Bitboard::GetSquare(move.moveFrom) & files[H] & (ranks[r_1] | ranks[r_8])).GetBoard()) {
			castlingRights[attackerColor * 2] = false;
		}
	}

	halfMoves++;

	if (halfMoves & 2 == 0) fullMoves++;
	lastMove = move;
	sideToMove = !attackerColor;
}