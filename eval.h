#include "chess.h"

#include <iostream>





//--------------------------------
class k2eval : public k2chess
{

public:
    k2eval();
    void RunUnitTests();

protected:

    typedef i8 pst_t;
    typedef u8 rank_t;
    typedef u8 dist_t;

    const eval_t
    centipawn = 100,
    king_value = 32000,
    infinite_score = 32760,
    material_total = 80,

    pawn_val_opn = 100,
    kinght_val_opn = 395,
    bishop_val_opn = 405,
    rook_val_opn = 600,
    queen_val_opn = 1200,
    pawn_val_end = 128,
    kinght_val_end = 369,
    bishop_val_end = 405,
    rook_val_end = 640,
    queen_val_end = 1300,

    pawn_dbl_iso_opn = -15,
    pawn_dbl_iso_end = -55,
    pawn_iso_opn = -15,
    pawn_iso_end = -25,
    pawn_dbl_opn = -5,
    pawn_dbl_end = -15,
    pawn_hole_opn = -30,
    pawn_hole_end = -15,
    pawn_gap_opn = 0,
    pawn_gap_end = -30,
    pawn_king_tropism1 = 30,
    pawn_king_tropism2 = 10,
    pawn_king_tropism3 = 15,
    pawn_pass_1 = 0,
    pawn_pass_2 = 21,
    pawn_pass_3 = 40,
    pawn_pass_4 = 85,
    pawn_pass_5 = 150,
    pawn_pass_6 = 200,
    pawn_blk_pass_1 = 0,
    pawn_blk_pass_2 = 10,
    pawn_blk_pass_3 = 30,
    pawn_blk_pass_4 = 65,
    pawn_blk_pass_5 = 80,
    pawn_blk_pass_6 = 120,
    pawn_pass_opn_divider = 3,
    pawn_pass_connected = 28,
    pawn_unstoppable_1 = 120,
    pawn_unstoppable_2 = 350,
    king_no_shelter = 47,
    king_saf_1 = 2,
    king_saf_2 = 20,
    king_saf_3 = 20,
    king_saf_4 = 22,
    king_saf_central_files = 68,
    rook_on_last_rank = 47,
    rook_semi_open_file = 22,
    rook_open_file = 54,
    rook_max_pawns_for_open_file = 4,
    bishop_pair = 30,
    mob_queen = 6,
    mob_rook = 13,
    mob_bishop = 8,
    mob_knight = 4,
    mobility_divider = 8,
    imbalance_king_in_corner = 200,
    imbalance_kbpk = 750,
    imbalance_multicolor1 = 2,
    imbalance_multicolor2 = 4,
    imbalance_multicolor3 = 5,
    imbalance_no_pawns = 3,
    side_to_move_bonus = 8;

    eval_t val_opn, val_end;
    eval_t initial_score;
    rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
    rank_t (*pawn_max)[sides], (*pawn_min)[sides];
    eval_t material_values_opn[piece_types + 1];
    eval_t material_values_end[piece_types + 1];

    const pst_t pst[piece_types][sides][board_height][board_width];
//    std::vector<float> tuning_factors;


public:


    eval_t EvalDebug();

    bool SetupPosition(const char *p)
    {
        bool ans = k2chess::SetupPosition(p);
        InitPawnStruct();
        InitEvalOfMaterialAndPst();
        return ans;
    }


protected:


    struct state_s
    {
        eval_t val_opn;
        eval_t val_end;
    };

    state_s e_state[prev_states + max_ply]; // eval state for each ply depth
    state_s *state;  // pointer to eval state

    void FastEval(const move_c m);
    eval_t Eval();
    void InitEvalOfMaterialAndPst();
    void InitPawnStruct();
    void SetPawnStruct(const coord_t col);
    bool IsPasser(const coord_t col, const bool stm) const;
    bool MakeMove(const move_c m);
    void TakebackMove(const move_c move);

    eval_t ReturnEval(const bool stm) const
    {
        i32 X, Y;
        X = material[0]/centipawn + 1 + material[1]/centipawn +
                1 - pieces[0] - pieces[1];
        Y = ((val_opn - val_end)*X + material_total*val_end)/material_total;
        return stm ? (eval_t)(Y) : (eval_t)(-Y);
    }

    bool is_light(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) == stm;
    }

    bool is_dark(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) != stm;
    }

    dist_t king_dist(const coord_t from_coord, const coord_t to_coord) const
    {
        return std::max(std::abs(get_col(from_coord) - get_col(to_coord)),
                        std::abs(get_row(from_coord) - get_row(to_coord)));
    }


private:

    const size_t opening = 0, endgame = 1;

    void EvalPawns(const bool stm);
    bool IsUnstoppablePawn(const coord_t col, const bool side_of_pawn,
                           const bool stm) const;
    void MaterialImbalances();
    void MovePawnStruct(const piece_t movedPiece, const coord_t from_coord,
                        const move_c m);
    void MobilityEval(bool stm);
    void KingSafety(const bool king_color);
    eval_t KingShelter(const coord_t k_col, coord_t k_row,
                       const bool stm);
    bool Sheltered(const coord_t k_col, coord_t k_row,
                           const bool stm);
    attack_t KingSafetyBatteries(const coord_t targ_coord, const attack_t att,
                                 const bool stm);
    void EvalRooks(const bool stm);
};
