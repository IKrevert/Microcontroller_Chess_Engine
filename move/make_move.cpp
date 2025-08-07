// makeMove.cpp

#include "defs.h"
#include <cstdio>
#include "make_move.h"
#include "../util/attack.h"
#include "../util/setup.h"

#define HASH_PCE(pce, sq) (board->posKey ^= (zobristPieceKeys[(pce)][(sq)]))
#define HASH_CA           (board->posKey ^= (zobristCastleKeys[(board->castlePerm)]))
#define HASH_SIDE         (board->posKey ^= (zobristSideKey))
#define HASH_EP           (board->posKey ^= (zobristPieceKeys[EMPTY][(board->enPas)]))

const int CastlePerm[120] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static void ClearPiece(const int sq, S_BOARD* board) {
    ASSERT(SqOnBoard(sq));
    ASSERT(isBoardStateValid(board));

    const int piece = board->pieces[sq];
    ASSERT(pieceValueid(piece));

    const int color = pieceColor[piece];
    ASSERT(SideValid(color));

    HASH_PCE(piece, sq);

    board->pieces[sq] = EMPTY;
    board->material[color] -= pieceValue[piece];

    if (isBigPiece[piece]) {
        board->bigPce[color]--;
        if (isMajorPiece[piece]) {
            board->majPce[color]--;
        } else {
            board->minPce[color]--;
        }
    } else {
        CLRBIT(board->pawns[color], SQ64(sq));
        CLRBIT(board->pawns[BOTH],  SQ64(sq));
    }

    int foundIndex = -1;
    for (int i = 0; i < board->pceNum[piece]; ++i) {
        if (board->pList[piece][i] == sq) {
            foundIndex = i;
            break;
        }
    }

    ASSERT(foundIndex != -1);
    ASSERT(foundIndex >= 0 && foundIndex < 10);

    board->pceNum[piece]--;
    board->pList[piece][foundIndex] = board->pList[piece][board->pceNum[piece]];
}

static void AddPiece(const int sq, S_BOARD* board, const int piece) {
    ASSERT(pieceValueid(piece));
    ASSERT(SqOnBoard(sq));

    const int color = pieceColor[piece];
    ASSERT(SideValid(color));

    HASH_PCE(piece, sq);

    board->pieces[sq] = piece;

    if (isBigPiece[piece]) {
        board->bigPce[color]++;
        if (isMajorPiece[piece]) {
            board->majPce[color]++;
        } else {
            board->minPce[color]++;
        }
    } else {
        SETBIT(board->pawns[color], SQ64(sq));
        SETBIT(board->pawns[BOTH],  SQ64(sq));
    }

    board->material[color] += pieceValue[piece];
    board->pList[piece][board->pceNum[piece]++] = sq;
}

static void MovePiece(const int from, const int to, S_BOARD* board) {
    ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));

    const int piece = board->pieces[from];
    ASSERT(pieceValueid(piece));

    const int color = pieceColor[piece];
    ASSERT(SideValid(color));

#ifdef DEBUG
    bool updatedList = false;
#endif

    HASH_PCE(piece, from);
    board->pieces[from] = EMPTY;

    HASH_PCE(piece, to);
    board->pieces[to] = piece;

    if (!isBigPiece[piece]) {
        CLRBIT(board->pawns[color], SQ64(from));
        CLRBIT(board->pawns[BOTH],  SQ64(from));
        SETBIT(board->pawns[color], SQ64(to));
        SETBIT(board->pawns[BOTH],  SQ64(to));
    }

    for (int i = 0; i < board->pceNum[piece]; ++i) {
        if (board->pList[piece][i] == from) {
            board->pList[piece][i] = to;
#ifdef DEBUG
            updatedList = true;
#endif
            break;
        }
    }

#ifdef DEBUG
    ASSERT(updatedList);
#endif
}

int makeMove(S_BOARD* board, int move) {
    ASSERT(isBoardStateValid(board));

    const int from = FROMSQ(move);
    const int to   = TOSQ(move);
    const int side = board->side;

    ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));
    ASSERT(SideValid(side));
    ASSERT(pieceValueid(board->pieces[from]));
    ASSERT(board->hisPly >= 0 && board->hisPly < MAXGAMEMOVES);
    ASSERT(board->ply   >= 0 && board->ply   < MAXDEPTH);

    board->history[board->hisPly].posKey = board->posKey;

    if (move & MFLAGEP) {
        if (side == WHITE) ClearPiece(to - 10, board);
        else               ClearPiece(to + 10, board);
    } else if (move & MFLAGCA) {
        switch (to) {
            case C1: MovePiece(A1, D1, board); break;
            case C8: MovePiece(A8, D8, board); break;
            case G1: MovePiece(H1, F1, board); break;
            case G8: MovePiece(H8, F8, board); break;
            default: ASSERT(FALSE); break;
        }
    }

    if (board->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    board->history[board->hisPly].move       = move;
    board->history[board->hisPly].fiftyMove  = board->fiftyMove;
    board->history[board->hisPly].enPas      = board->enPas;
    board->history[board->hisPly].castlePerm = board->castlePerm;

    board->castlePerm &= CastlePerm[from];
    board->castlePerm &= CastlePerm[to];
    board->enPas = NO_SQ;

    HASH_CA;

    const int captured = CAPTURED(move);
    board->fiftyMove++;

    if (captured != EMPTY) {
        ASSERT(pieceValueid(captured));
        ClearPiece(to, board);
        board->fiftyMove = 0;
    }

    board->hisPly++;
    board->ply++;

    ASSERT(board->hisPly >= 0 && board->hisPly < MAXGAMEMOVES);
    ASSERT(board->ply   >= 0 && board->ply   < MAXDEPTH);

    if (isPawn[board->pieces[from]]) {
        board->fiftyMove = 0;
        if (move & MFLAGPS) {
            if (side == WHITE) {
                board->enPas = from + 10;
                ASSERT(rankIndex120[board->enPas] == RANK_3);
            } else {
                board->enPas = from - 10;
                ASSERT(rankIndex120[board->enPas] == RANK_6);
            }
            HASH_EP;
        }
    }

    MovePiece(from, to, board);

    const int promoted = PROMOTED(move);
    if (promoted != EMPTY) {
        ASSERT(pieceValueid(promoted) && !isPawn[promoted]);
        ClearPiece(to, board);
        AddPiece(to, board, promoted);
    }

    if (isKing[board->pieces[to]]) {
        board->KingSq[board->side] = to;
    }

    board->side ^= 1;
    HASH_SIDE;

    ASSERT(isBoardStateValid(board));

    if (isSquareAttacked(board->KingSq[side], board->side, board)) {
        takeMove(board);
        return FALSE;
    }

    return TRUE;
}

void takeMove(S_BOARD* board) {
    ASSERT(isBoardStateValid(board));

    board->hisPly--;
    board->ply--;

    ASSERT(board->hisPly >= 0 && board->hisPly < MAXGAMEMOVES);
    ASSERT(board->ply   >= 0 && board->ply   < MAXDEPTH);

    const int move = board->history[board->hisPly].move;
    const int from = FROMSQ(move);
    const int to   = TOSQ(move);

    ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));

    if (board->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    board->castlePerm = board->history[board->hisPly].castlePerm;
    board->fiftyMove  = board->history[board->hisPly].fiftyMove;
    board->enPas      = board->history[board->hisPly].enPas;

    if (board->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    board->side ^= 1;
    HASH_SIDE;

    if (MFLAGEP & move) {
        if (board->side == WHITE) AddPiece(to - 10, board, bP);
        else                      AddPiece(to + 10, board, wP);
    } else if (MFLAGCA & move) {
        switch (to) {
            case C1: MovePiece(D1, A1, board); break;
            case C8: MovePiece(D8, A8, board); break;
            case G1: MovePiece(F1, H1, board); break;
            case G8: MovePiece(F8, H8, board); break;
            default: ASSERT(FALSE); break;
        }
    }

    MovePiece(to, from, board);

    if (isKing[board->pieces[from]]) {
        board->KingSq[board->side] = from;
    }

    const int captured = CAPTURED(move);
    if (captured != EMPTY) {
        ASSERT(pieceValueid(captured));
        AddPiece(to, board, captured);
    }

    if (PROMOTED(move) != EMPTY) {
        ASSERT(pieceValueid(PROMOTED(move)) && !isPawn[PROMOTED(move)]);
        ClearPiece(from, board);
        AddPiece(from, board, (pieceColor[PROMOTED(move)] == WHITE ? wP : bP));
    }

    ASSERT(isBoardStateValid(board));
}

void makeNullMove(S_BOARD* board) {
    ASSERT(isBoardStateValid(board));
    ASSERT(!isSquareAttacked(board->KingSq[board->side], board->side ^ 1, board));

    board->ply++;
    board->history[board->hisPly].posKey = board->posKey;

    if (board->enPas != NO_SQ) HASH_EP;

    board->history[board->hisPly].move       = NOMOVE;
    board->history[board->hisPly].fiftyMove  = board->fiftyMove;
    board->history[board->hisPly].enPas      = board->enPas;
    board->history[board->hisPly].castlePerm = board->castlePerm;
    board->enPas = NO_SQ;

    board->side ^= 1;
    board->hisPly++;
    HASH_SIDE;

    ASSERT(isBoardStateValid(board));
    ASSERT(board->hisPly >= 0 && board->hisPly < MAXGAMEMOVES);
    ASSERT(board->ply   >= 0 && board->ply   < MAXDEPTH);
}

void takeNullMove(S_BOARD* board) {
    ASSERT(isBoardStateValid(board));

    board->hisPly--;
    board->ply--;

    if (board->enPas != NO_SQ) HASH_EP;

    board->castlePerm = board->history[board->hisPly].castlePerm;
    board->fiftyMove  = board->history[board->hisPly].fiftyMove;
    board->enPas      = board->history[board->hisPly].enPas;

    if (board->enPas != NO_SQ) HASH_EP;
    board->side ^= 1;
    HASH_SIDE;

    ASSERT(isBoardStateValid(board));
    ASSERT(board->hisPly >= 0 && board->hisPly < MAXGAMEMOVES);
    ASSERT(board->ply   >= 0 && board->ply   < MAXDEPTH);
}
