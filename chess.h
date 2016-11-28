#include <iostream>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include "short_list.h"





//--------------------------------
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long long u64;

#define ONBRD(X)        (!((X) & 0x88))
#define XY2SQ(X, Y)     (((Y) << 4) + (X))
#define COL(SQ)         ((SQ) & 7)
#define ROW(SQ)         ((SQ) >> 4)
#define ABSI(X)         ((X) > 0 ? (X) : (-(X)))
#define GET_INDEX(X)    ((X)/2)
#define TO_BLACK(X)     ((X) & ~white)

const int lst_sz        = 32;                                                 // size of piece list for one colour
const unsigned max_ply  = 100;                                                // maximum search depth
const int prev_states   = 4;
const u8  white         = 1;
const u8  black         = 0;





//--------------------------------
enum {__ = 0,  _k = 2, _q = 4, _r = 6, _b = 8, _n = 10, _p = 12,
               _K = 3, _Q = 5, _R = 7, _B = 9, _N = 11, _P = 13};
enum {mCAPT = 0x10, mCS_K = 0x20, mCS_Q = 0x40, mCSTL = 0x60, mENPS = 0x80,
      mPR_Q = 0x01, mPR_N = 0x02, mPR_R = 0x03, mPR_B = 0x04, mPROM = 0x07};





//--------------------------------
class Move
{
public:
    u8 to;
    short_list<u8, lst_sz>::iterator_entity pc;
    u8 flg;
    u8 scr;

    bool operator == (Move m)   {return to == m.to && pc == m.pc && flg == m.flg;}
    bool operator != (Move m)   {return to != m.to || pc != m.pc || flg != m.flg;}
    bool operator < (const Move m) const {return scr < m.scr;}
};
                                                                        // 'to' - coords for piece to move to (8 bit);
                                                                        // 'pc' - number of piece in 'men' array (8 bit);
                                                                        // 'flg' - flags (mCAPT, etc) - (8 bit)
                                                                        // 'scr' - unsigned score (priority) by move generator (8 bit)
                                                                        //      0..63    - bad captures
                                                                        //      64..127  - silent moves without history (pst value)
                                                                        //      128..195 - silent moves with history value
                                                                        //      196 - equal capture
                                                                        //      197 - move from pv
                                                                        //      198 - second killer
                                                                        //      199 - first killer
                                                                        //      200..250 - good captures and/or promotions
                                                                        //      255 - opp king capture or hash hit

//--------------------------------
struct BrdState
{
    u8 capt;                                                            // taken piece, 6 bits
    short_list<u8, lst_sz>::iterator_entity captured_it;                // iterator to captured piece
    u8 fr;                                                              // from point, 7 bits
    u8 cstl;                                                            // castling rights, bits 0..3: _K, _Q, _k, _q, 4 bits

    short_list<u8, lst_sz>::iterator_entity castled_rook_it;            // iterator to castled rook, 8 bits
    u8 ep;                                                              // 0 = no_ep, else ep=COL(x) + 1, not null only if opponent pawn is near, 4 bits
    short_list<u8, lst_sz>::iterator_entity nprom;                      // number of next piece for promoted pawn, 6 bits
    u16 reversibleCr;                                                    // reversible halfmove counter
    u8 to;                                                              // to point, 7 bits (for simple repetition draw detection)
    short val_opn;                                                       // store material and PST value considered all material is on the board
    short val_end;                                                       // store material and PST value considered deep endgame (kings and pawns only)
    u8 scr;                                                             // move priority by move genererator
    short tropism[2];
};





//--------------------------------
void InitChess();
void InitAttacks();
void InitBrd();
bool BoardToMen();
bool FenToBoard(char *p);
void ShowMove(u8 fr, u8 to);
bool MakeCastle(Move m, u8 fr);
void UnMakeCastle(Move m);
bool MakeEP(Move m, u8 fr);
bool SliderAttack(u8 fr, u8 to);
bool Attack(u8 to, int xtm);
bool LegalSlider(u8 fr, u8 to, u8 pt);
bool Legal(Move m, bool ic);
void SetPawnStruct(int col);
void InitPawnStruct();
bool PieceListCompare(u8 men1, u8 men2);
void StoreCurrentBoardState(Move m, u8 fr, u8 targ);
void MakeCapture(Move m, u8 targ);
void MakePromotion(Move m, short_list<u8, lst_sz>::iterator it);
void UnmakeCapture(Move m);
void UnmakePromotion(Move m);
bool MkMoveFast(Move m);
void UnMoveFast(Move m);
