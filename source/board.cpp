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
	GenAttackMaps(*this);
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
	int count = 1;
	for (Move move : moveList) {
		std::cout << count << ". ";
		move.PrintMove();
		count++;
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

U64 Board::GetAttackMaps(bool side) {
	U64 combined = 0ULL;
	for (int type = Pawn; type <= King; type++) {
		combined |= attackMaps[side][type];
	}
	return combined;
}

bool Board::InCheck(bool side) {
	Bitboard myKingSquare = colors[side] & pieces[King];

	return GetAttackMaps(!side) & myKingSquare.GetBoard();

	return false;
}

void Board::SetPiece(int piece, int square, bool color) {
	pieces[piece].SetBit(square);
	colors[color].SetBit(square);
	occupied.SetBit(square);
}

void Board::RemovePiece(int piece, int square, bool color) {
	pieces[piece].PopBit(square);
	colors[color].PopBit(square);
	occupied.PopBit(square);
}

void Board::Promote(int square, int pieceType, int color, bool isCapture) {
	if (isCapture) {
		int targetType = GetPieceType(square);
		RemovePiece(targetType, square, !color);
	}

	SetPiece(pieceType, square, color);
}

void Board::MakeMove(Move move) {
	
}