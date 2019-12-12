#include "eval.h"





//-----------------------------
k2chess::eval_t k2eval::Eval(const bool stm, eval_vect cur_eval)
{
    for(auto side: {black, white})
    {
        cur_eval += EvalPawns(side, stm);
        cur_eval += EvalRooks(side);
        cur_eval += EvalKingSafety(side);
        cur_eval += EvalMobility(side);
    }
    cur_eval += EvalSideToMove(stm);
    cur_eval = EvalImbalances(stm, cur_eval);

    return GetEvalScore(stm, cur_eval);
}





//-----------------------------
k2eval::eval_vect k2eval::FastEval(const bool stm, const move_c move) const
{
    eval_vect ans = {0, 0};

    auto x = get_col(move.to_coord);
    auto y = get_row(move.to_coord);
    auto x0 = get_col(move.from_coord);
    auto y0 = get_row(move.from_coord);
    auto to_type = get_type(b[move.to_coord]);
    if(!stm)
    {
        y = max_row - y;
        y0 = max_row - y0;
    }

    if(move.flag & is_promotion)
    {
        ans -= material_values[pawn] + pst[pawn - 1][y0][x0];

        piece_t prom_pc[] =
        {
            black_queen, black_knight, black_rook, black_bishop
        };
        const auto type = get_type(prom_pc[(move.flag & is_promotion) - 1]);
        ans += material_values[type] + pst[type - 1][y0][x0];
    }

    if(move.flag & is_capture)
    {
        auto captured_piece = k2chess::state[ply].captured_piece;
        if(move.flag & is_en_passant)
            ans += material_values[pawn] + pst[pawn - 1][max_row - y0][x];
        else
        {
            const auto type = get_type(captured_piece);
            ans += material_values[type] + pst[type - 1][max_row - y][x];
        }
    }
    else if(move.flag & is_right_castle)
    {
        const auto r_col = default_king_col + cstl_move_length - 1;
        ans += pst[rook - 1][max_row][r_col] - pst[rook - 1][max_row][max_col];
    }
    else if(move.flag & is_left_castle)
    {
        auto r_col = default_king_col - cstl_move_length + 1;
        ans += pst[rook - 1][max_row][r_col] - pst[rook - 1][max_row][0];
    }
    ans += pst[to_type - 1][y][x] - pst[to_type - 1][y0][x0];

    return stm ? -ans : ans;
}





//-----------------------------
k2eval::eval_vect k2eval::InitEvalOfMaterial()
{
    eval_vect ans = {0, 0};
    for(auto col = 0; col <= max_col; ++col)
        for(auto row = 0; row <= max_row; ++row)
        {
            auto piece = b[get_coord(col, row)];
            if(piece == empty_square)
                continue;
            auto delta = material_values[get_type(piece)];
            ans += (piece & white) ? delta : -delta;
    }
    return ans;
}





//-----------------------------
k2eval::eval_vect k2eval::InitEvalOfPST()
{
    eval_vect ans = {0, 0};
    for(auto col = 0; col <= max_col; ++col)
        for(auto row = 0; row <= max_row; ++row)
        {
            auto piece = b[get_coord(col, row)];
            if(piece == empty_square)
                continue;
            auto row_ = row;
            if(piece & white)
                row_ = max_row - row;

            auto type = get_type(piece);
            auto delta = pst[type - 1][row_][col];
            ans += (piece & white) ? delta : -delta;
    }
    return ans;
}





//--------------------------------
bool k2eval::IsPasser(const coord_t col, const bool stm) const
{
    auto mx = pawn_max[col][stm];

    return (mx >= max_row - pawn_min[col][!stm] &&
            mx >= max_row - pawn_min[col - 1][!stm] &&
            mx >= max_row - pawn_min[col + 1][!stm]);
}





//--------------------------------
k2eval::eval_vect k2eval::EvalPawns(const bool side, const bool stm)
{
    eval_vect ans = {0, 0};
    bool passer, prev_passer = false;
    bool opp_only_pawns = material[!side]/centipawn == pieces[!side] - 1;

    for(auto col = 0; col <= max_col; col++)
    {
        bool doubled = false, isolany = false;

        const eval_t mx = pawn_max[col][side];
        if(mx == 0)
        {
            prev_passer = false;
            continue;
        }

        if(pawn_min[col][side] != mx)
            doubled = true;
        if(pawn_max[col - 1][side] == 0 && pawn_max[col + 1][side] == 0)
            isolany = true;
        if(doubled && isolany)
            ans -= pawn_dbl_iso;
        else if(isolany)
            ans -= pawn_iso;
        else if(doubled)
            ans -= pawn_dbl;

        passer = IsPasser(col, side);
        if(!passer)
        {
            // pawn holes occupied by enemy pieces
            if(col > 0 && col < max_col && mx < pawn_min[col - 1][side]
                    && mx < pawn_min[col + 1][side])
            {
                auto y_coord = side ? mx + 1 : max_row - mx - 1;
                auto op_piece = b[get_coord(col, y_coord)];
                bool occupied = is_dark(op_piece, side)
                                && get_type(op_piece) != pawn;
                if(occupied)
                    ans -= pawn_hole;
            }
            // gaps in pawn structure
            if(pawn_max[col - 1][side]
                    && std::abs(mx - pawn_max[col - 1][side]) > 1)
                ans -= pawn_gap;

            prev_passer = false;
            continue;
        }
        // following code executed only for passers

        // king pawn tropism
        const auto k_coord = king_coord(side);
        const auto opp_k_coord = king_coord(!side);
        const auto stop_coord = get_coord(col, side ? mx + 1 : max_row - mx-1);
        const auto k_dist = king_dist(k_coord, stop_coord);
        const auto opp_k_dist = king_dist(opp_k_coord, stop_coord);

        if(k_dist <= 1)
            ans += pawn_king_tropism1 + mx*pawn_king_tropism2;
        else if(k_dist == 2)
            ans += pawn_king_tropism3;
        if(opp_k_dist <= 1)
            ans -= pawn_king_tropism1 + mx*pawn_king_tropism2;
        else if(opp_k_dist == 2)
            ans -= pawn_king_tropism3;

        // passed pawn evaluation
        const bool blocked = b[stop_coord] != empty_square;
        const eval_t mx2 = mx*mx;
        if(blocked)
            ans += mx2*pawn_blk_pass2 + mx*pawn_blk_pass1 + pawn_blk_pass0;
        else
            ans += mx2*pawn_pass2 + mx*pawn_pass1 + pawn_pass0;

        // connected passers
        if(passer && prev_passer
                && std::abs(mx - pawn_max[col - 1][side]) <= 1)
        {
            const eval_t mmx = std::max(
                        mx, static_cast<eval_t>(pawn_max[col - 1][side]));
            if(mmx > 4)
                ans += mmx*pawn_pass_connected;
        }
        prev_passer = true;

        // unstoppable
        if(opp_only_pawns && IsUnstoppablePawn(col, side, stm))
        {
            ans.real(ans.real() + mx*pawn_unstoppable.real());
            ans.imag(ans.imag() + pawn_unstoppable.imag());
        }
    }
    return side ? ans : -ans;
}





//-----------------------------
bool k2eval::IsUnstoppablePawn(const coord_t col, const bool side,
                               const bool stm) const
{
    auto pmax = pawn_max[col][side];
    if(pmax == pawn_default_row)
        pmax++;
    const auto promo_square = get_coord(col, side ? max_row : 0);
    const int dist = king_dist(king_coord(!side), promo_square);
    const auto k_coord = king_coord(side);
    if(get_col(k_coord) == col &&
            king_dist(k_coord, promo_square) <= max_row - pmax)
        pmax--;
    return dist - (side != stm) > max_row - pmax;
}





//-----------------------------
k2eval::eval_vect k2eval::EvalMobility(bool stm)
{
    eval_vect f_type[] = {{0, 0}, {0, 0}, mob_queen, mob_rook, mob_bishop,
                          mob_knight};
    eval_t f_num[] = {-15, -15, -15, -10, -5, 5, 10, 15, 18, 20,
                      21, 21, 21, 22, 22};

    eval_vect ans = {0, 0};
    auto mask = exist_mask[stm] & slider_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto coord = coords[stm][piece_id];
        const auto type = get_type(b[coord]);
        int cr = sum_directions[stm][piece_id];
        if(type == queen)
            cr /= 2;
        assert(cr < 15);
        ans += f_num[cr]*f_type[type];
    }
    const eval_t mob_devider = 8;
    return stm ? ans/mob_devider : -ans/mob_devider;
}





//-----------------------------
k2eval::eval_vect k2eval::EvalImbalances(const bool stm, eval_vect val)
{
    const auto X = material[black]/centipawn + 1 + material[white]/centipawn
            + 1 - pieces[black] - pieces[white];

    if(X == 3 && (material[black]/centipawn == 4 ||
                  material[white]/centipawn == 4))
    {
        // KNk, KBk, Kkn, Kkb
        if(pieces[black] + pieces[white] == 3)
            return {0, 0};
        // KPkn, KPkb
        else if((material[white]/centipawn == 1 &&
                 material[black]/centipawn == 4) ||
                (material[black]/centipawn == 1 &&
                 material[white]/centipawn == 4))
            return vect_div(val, imbalance_draw_divider);
    }
    // KNNk, KBNk, KBBk, etc
    else if(X == 6 && (material[black] == 0 || material[white] == 0))
    {
        if(quantity[white][pawn] || quantity[black][pawn])
            return val;
        if(quantity[white][knight] == 2
                || quantity[black][knight] == 2)
            return {0, 0};
        // many code for mating with only bishop and knight
        else if((quantity[white][knight] == 1 &&
                 quantity[white][bishop] == 1) ||
                (quantity[black][knight] == 1 &&
                 quantity[black][bishop] == 1))
        {
            const auto stm = quantity[black][knight] == 1 ? black : white;
            const auto bishop_mask = exist_mask[stm] & type_mask[stm][bishop];
            const auto bishop_id = lower_bit_num(bishop_mask);
            const auto bishop_coord = coords[stm][bishop_id];
            const bool is_light_b = (get_col(bishop_coord) +
                                     get_row(bishop_coord)) & 1;
            const auto corner1 = is_light_b ? get_coord(0, max_row) : 0;
            const auto corner2 = is_light_b ? get_coord(max_col, 0) :
                                              get_coord(max_col, max_row);
            const auto opp_k = king_coord(!stm);
            if(king_dist(corner1, opp_k) > 1 && king_dist(corner2, opp_k) > 1)
                return val;
            const auto opp_col = get_col(opp_k);
            const auto opp_row = get_row(opp_k);
            if(opp_col == 0 || opp_row == 0 ||
                    opp_col == max_col || opp_row == max_row)
                return stm ? imbalance_king_in_corner :
                                 -imbalance_king_in_corner;
            return val;
        }
    }
    // KXPkx, where X = {N, B, R, Q}
    else if((X == 6 || X == 10 || X == 22) &&
            quantity[white][pawn] + quantity[black][pawn] == 1)
    {
        const auto stm = quantity[white][pawn] == 1 ? white : black;
        const auto pawn_mask = exist_mask[stm] & type_mask[stm][pawn];
        const auto pawn_id = lower_bit_num(pawn_mask);
        const auto pawn_coord = coords[stm][pawn_id];
        const auto telestop = get_coord(get_col(pawn_coord),
                                        stm ? max_row : 0);
        auto king_row = get_row(king_coord(stm));
        if(!stm)
            king_row = max_row - king_row;
        const auto king_col = get_col(king_coord(stm));
        const auto pawn_col = get_col(pawn_coord);
        const bool king_in_front = king_row >
                pawn_max[get_col(pawn_coord)][stm] &&
                std::abs(king_col - pawn_col) <= 1;

        if(king_dist(king_coord(!stm), telestop) <= 1 && (!king_in_front
                || king_dist(king_coord(stm), pawn_coord) > 2))
            return vect_div(val, imbalance_draw_divider);
    }
    // KBPk(p) with pawn at first(last) file and bishop with 'good' color
    else if(X == 3)
    {
        if(quantity[black][pawn] > 1 || quantity[white][pawn] > 1)
            return val;
        if(!quantity[white][bishop] && !quantity[black][bishop])
            return val;
        const bool stm = quantity[white][bishop] == 0;
        const auto pawn_col_min = pawn_max[0][!stm];
        const auto pawn_col_max = pawn_max[max_col][!stm];
        if(pawn_col_max == 0 && pawn_col_min == 0)
            return val;
        const auto bishop_mask = exist_mask[!stm] & type_mask[!stm][bishop];
        if(!bishop_mask)
            return val;
        const auto coord = coords[!stm][lower_bit_num(bishop_mask)];
        const auto sq = get_coord(pawn_col_max == 0 ? 0 : max_col,
                                  stm ? 0 : max_row);
        if(is_same_color(coord, sq))
            return val;

        if(king_dist(king_coord(stm), sq) <= 1)
            return {0, 0};
    }
    else if(X == 0 && (material[white] + material[black])/centipawn == 1)
    {
        // KPk
        const bool side = material[white]/centipawn == 1;
        const auto pawn_mask = exist_mask[side] & type_mask[side][pawn];
        const auto pawn_id = lower_bit_num(pawn_mask);
        const auto coord = coords[side][pawn_id];
        const auto colp = get_col(coord);
        const bool unstop = IsUnstoppablePawn(colp, side, stm);
        const auto k_coord = king_coord(side);
        const auto opp_k_coord = king_coord(!side);
        const auto dist_k = king_dist(k_coord, coord);
        const auto dist_opp_k = king_dist(opp_k_coord, coord);

        if(!unstop && dist_k > dist_opp_k + (stm == side))
            return {0, 0};
        else if((colp == 0 || colp == max_col))
        {
            const auto sq = get_coord(colp, side ? max_row : 0);
            if(king_dist(opp_k_coord, sq) <= 1)
                return {0, 0};
        }
    }
    // KRB(N)kr or KB(N)B(N)kb(n)
    else if((X == 13 || X == 9) &&
            !quantity[white][pawn] && !quantity[black][pawn])
    {
        const bool stm = material[white] < material[black];
        if(!is_king_near_corner(stm))
            val = vect_mul(val, imbalance_no_pawns)/static_cast<eval_t>(64);
    }

    // two bishops
    if(quantity[white][bishop] == 2)
        val += bishop_pair;
    if(quantity[black][bishop] == 2)
        val -= bishop_pair;

    // pawn absence for both sides
    if(!quantity[white][pawn] && !quantity[black][pawn]
            && material[white] != 0 && material[black] != 0)
        val = vect_mul(val, imbalance_no_pawns)/static_cast<eval_t>(64);

    // multicolored bishops
    if(quantity[white][bishop] == 1 && quantity[black][bishop] == 1)
    {
        const auto wb_mask = exist_mask[white] & type_mask[white][bishop];
        const auto wb_id = lower_bit_num(wb_mask);
        const auto bb_mask = exist_mask[black] & type_mask[black][bishop];
        const auto bb_id = lower_bit_num(bb_mask);
        if(is_same_color(coords[white][wb_id], coords[black][bb_id]))
            return val;
        const bool only_pawns =
                material[white]/centipawn - pieces[white] == 4 - 2 &&
                material[black]/centipawn - pieces[black] == 4 - 2;
        if(only_pawns && quantity[white][pawn] + quantity[black][pawn] == 1)
            return vect_div(val, imbalance_draw_divider);
        auto Ki = only_pawns ? imbalance_multicolor.real() :
                               imbalance_multicolor.imag();
        val.imag(val.imag()*Ki/64);
    }
    return val;
}





//-----------------------------
void k2eval::DbgOut(const char *str, eval_vect val, eval_vect &sum) const
{
    std::cout << str << '\t';
    std::cout << val.real() << '\t' << val.imag() << '\t' <<
                 GetEvalScore(white, val) << std::endl;
    sum += val;
}





//-----------------------------
void k2eval::EvalDebug(const bool stm)
{
    std::cout << "\t\t\tMidgame\tEndgame\tTotal" << std::endl;
    eval_vect sum = {0, 0}, zeros = {0, 0};
    DbgOut("Material both sides", InitEvalOfMaterial(), sum);
    DbgOut("PST both sides\t", InitEvalOfPST(), sum);
    DbgOut("White pawns\t", EvalPawns(white, stm), sum);
    DbgOut("Black pawns\t", EvalPawns(black, stm), sum);
    DbgOut("King safety white", EvalKingSafety(white), sum);
    DbgOut("King safety black", EvalKingSafety(black), sum);
    DbgOut("Mobility white\t", EvalMobility(white), sum);
    DbgOut("Mobility black\t", EvalMobility(black), sum);
    DbgOut("Rooks white\t", EvalRooks(white), sum);
    DbgOut("Rooks black\t", EvalRooks(black), sum);
    DbgOut("Bonus for side to move", EvalSideToMove(stm), sum);
    DbgOut("Imbalances both sides", EvalImbalances(stm, sum) - sum, zeros);
    sum = EvalImbalances(stm, sum);
    eval_vect tmp = InitEvalOfMaterial() + InitEvalOfPST();
    if(Eval(wtm, tmp) != GetEvalScore(wtm, sum))
    {
        assert(true);
        std::cout << "\nInternal error, please contact with developer\n";
        return;
    }
    std::cout << std::endl << "Grand Total:\t\t" << sum.real() << '\t' <<
                 sum.imag() << '\t' << GetEvalScore(white, sum) << std::endl;
    std::cout << "(positive values mean advantage for white)\n\n";
}





//-----------------------------
void k2eval::SetPawnStruct(const coord_t col)
{
    assert(col <= max_col);
    coord_t y;
    if(wtm)
    {
        y = pawn_default_row;
        while(b[get_coord(col, max_row - y)] != black_pawn && y < max_row)
            y++;
        pawn_min[col][black] = y;

        y = max_row - pawn_default_row;
        while(b[get_coord(col, max_row - y)] != black_pawn && y > 0)
            y--;
        pawn_max[col][black] = y;
    }
    else
    {
        y = pawn_default_row;
        while(b[get_coord(col, y)] != white_pawn && y < max_row)
            y++;
        pawn_min[col][white] = y;

        y = max_row - pawn_default_row;
        while(b[get_coord(col, y)] != white_pawn && y > 0)
            y--;
        pawn_max[col][white] = y;
    }
}





//-----------------------------
void k2eval::MovePawnStruct(const piece_t moved_piece, const move_c move)
{
    if(get_type(moved_piece) == pawn || (move.flag & is_promotion))
    {
        SetPawnStruct(get_col(move.to_coord));
        if(move.flag)
            SetPawnStruct(get_col(move.from_coord));
    }
    if(get_type(k2chess::state[ply].captured_piece) == pawn
            || (move.flag & is_en_passant))  // is_en_passant not needed
    {
        wtm ^= white;
        SetPawnStruct(get_col(move.to_coord));
        wtm ^= white;
    }
#ifndef NDEBUG
    rank_t copy_max[8][2], copy_min[8][2];
    memcpy(copy_max, pawn_max, sizeof(copy_max));
    memcpy(copy_min, pawn_min, sizeof(copy_min));
    InitPawnStruct();
    assert(!memcmp(copy_max, pawn_max, sizeof(copy_max)));
    assert(!memcmp(copy_min, pawn_min, sizeof(copy_min)));
#endif
}





//-----------------------------
void k2eval::InitPawnStruct()
{
    pawn_max = &p_max[1];
    pawn_min = &p_min[1];
    for(auto col: {0, 9})
        for(auto color: {black, white})
        {
            p_max[col][color] = 0;
            p_min[col][color] = 7;
        }
    for(auto col = 0; col <= max_col; col++)
    {
        pawn_max[col][black] = 0;
        pawn_max[col][white] = 0;
        pawn_min[col][black] = max_row;
        pawn_min[col][white] = max_row;
        for(auto row = pawn_default_row;
            row <= max_row - pawn_default_row;
            row++)
            if(b[get_coord(col, row)] == white_pawn)
            {
                pawn_min[col][white] = row;
                break;
            }
        for(auto row = max_row - pawn_default_row;
            row >= pawn_default_row;
            row--)
            if(b[get_coord(col, row)] == white_pawn)
            {
                pawn_max[col][white] = row;
                break;
            }
        for(auto row = max_row - pawn_default_row;
            row >= pawn_default_row;
            row--)
            if(b[get_coord(col, row)] == black_pawn)
            {
                pawn_min[col][0] = max_row - row;
                break;
            }
        for(auto row = pawn_default_row;
            row <= max_row - pawn_default_row;
            row++)
            if(b[get_coord(col, row)] == black_pawn)
            {
                pawn_max[col][0] = max_row - row;
                break;
            }
    }
}





//-----------------------------
bool k2eval::MakeMove(const move_c move)
{
    bool is_special_move = k2chess::MakeMove(move);
    MovePawnStruct(b[move.to_coord], move);
    return is_special_move;
}





//-----------------------------
void k2eval::TakebackMove(const move_c move)
{
    k2chess::TakebackMove(move);

    ply++;
    wtm ^= white;
    MovePawnStruct(b[move.from_coord], move);
    wtm ^= white;
    ply--;
}





//-----------------------------
k2eval::eval_vect k2eval::EvalRooks(const bool stm)
{
    eval_vect ans = 0;
    eval_t rooks_on_last_cr = 0;
    auto mask = exist_mask[stm] & type_mask[stm][rook];
    while(mask)
    {
        auto rook_id = lower_bit_num(mask);
        mask ^= (1 << rook_id);
        const auto coord = coords[stm][rook_id];
        if((stm && get_row(coord) >= max_row - 1) ||
                (!stm && get_row(coord) <= 1))
            rooks_on_last_cr++;
        if(quantity[stm][pawn] >= rook_max_pawns_for_open_file &&
                pawn_max[get_col(coord)][stm] == 0)
            ans += (pawn_max[get_col(coord)][!stm] == 0 ? rook_open_file :
                                                         rook_semi_open_file);
    }
    ans += rooks_on_last_cr*rook_last_rank;
    return stm ? ans : -ans;
}





//-----------------------------
bool k2eval::Sheltered(const coord_t k_col, coord_t k_row,
                       const bool stm) const
{
    if(!col_within(k_col))
        return false;
    if((stm && k_row > 1) || (!stm && k_row < max_row - 1))
        return false;
    const auto shft = stm ? board_width : -board_width;
    const auto coord = get_coord(k_col, k_row);
    const auto p = black_pawn ^ stm;
    if(coord + 2*shft < 0 || coord + 2*shft >= board_size)
        return false;
    if(b[coord + shft] == p)
        return true;
    else
    {
        const auto pc1 = b[coord + shft];
        const auto pc2 = b[coord + 2*shft];
        const auto pc1_r = col_within(k_col + 1) ?
                    b[coord + shft + 1] : empty_square;
        const auto pc1_l = col_within(k_col - 1) ?
                    b[coord + shft - 1] : empty_square;
        if(pc2 == p && (pc1_r == p || pc1_l == p))
            return true;
        if(pc1 != empty_square && get_color(pc1) == stm &&
                pc2 != empty_square && get_color(pc2) == stm)
            return true;

    }
    return false;
}





//-----------------------------
bool k2eval::KingHasNoShelter(coord_t k_col, coord_t k_row,
                              const bool stm) const
{
    if(k_col == 0)
        k_col++;
    else if(k_col == max_col)
        k_col--;
    const i32 cstl[] = {black_can_castle_left | black_can_castle_right,
                        white_can_castle_left | white_can_castle_right};
    if(!quantity[!stm][queen] ||
            (k2chess::state[ply].castling_rights & cstl[stm]))
        return false;
    if(!Sheltered(k_col, k_row, stm))
    {
        if(k_col > 0 && k_col < max_col &&
                (stm ? k_row < max_row : k_row > 0) &&
                b[get_coord(k_col, k_row+(stm ? 1 : -1))] != empty_square &&
                Sheltered(k_col - 1, k_row, stm) &&
                Sheltered(k_col + 1, k_row, stm))
            return false;
        else
            return true;
    }
    if(!Sheltered(k_col - 1, k_row, stm) &&
            !Sheltered(k_col + 1, k_row, stm))
        return true;
    return 0;
}





//-----------------------------
k2chess::attack_t k2eval::KingSafetyBatteries(const coord_t targ_coord,
                                              const attack_t att,
                                              const bool stm) const
{
    auto msk = att & slider_mask[!stm];
    auto ans = att;
    while(msk)
    {
        const auto piece_id = lower_bit_num(msk);
        msk ^= (1 << piece_id);
        const auto coord1 = coords[!stm][piece_id];
        auto maybe = attacks[!stm][coord1] & slider_mask[!stm];
        if(!maybe)
            continue;
        while(maybe)
        {
            const auto j = lower_bit_num(maybe);
            maybe ^= (1 << j);
            const auto coord2 = coords[!stm][j];
            const auto type1 = get_type(b[coord1]);
            const auto type2 = get_type(b[coord2]);
            bool is_ok = type1 == type2 || type2 == queen;
            if(!is_ok && type1 == queen)
            {
                const auto col1 = get_col(coord1);
                const auto col2 = get_col(coord2);
                const auto col_t = get_col(targ_coord);
                const auto row1 = get_row(coord1);
                const auto row2 = get_row(coord2);
                const auto row_t = get_row(targ_coord);
                bool like_rook = (col1 == col2 && col1 == col_t) ||
                        (row1 == row2 && row1 == row_t);
                bool like_bishop = sgn(col1 - col_t) == sgn(col2 - col1) &&
                        sgn(row1 - row_t) == sgn(row2 - row_t);
                if(type2 == rook && like_rook)
                    is_ok = true;
                else if(type2 == bishop && like_bishop)
                    is_ok = true;
            }
            if(is_ok && IsOnRay(coord1, targ_coord, coord2) &&
                    IsSliderAttack(coord1, coord2))
                ans |= (1 << j);
        }
    }
    return ans;
}





//-----------------------------
size_t k2eval::CountAttacksOnKing(const bool stm, const coord_t k_col,
                                  const coord_t k_row) const
{
    size_t ans = 0;
    for(auto delta_col = -1; delta_col <= 1; ++delta_col)
        for(auto delta_row = -1; delta_row <= 1; ++delta_row)
        {
            const auto col = k_col + delta_col;
            const auto row = k_row + delta_row;
            if(!row_within(row) || !col_within(col))
                continue;
            const auto targ_coord = get_coord(col, row);
            const auto att = attacks[!stm][targ_coord];
            if(!att)
                continue;
            const auto all_att = KingSafetyBatteries(targ_coord, att, stm);
            ans += __builtin_popcount(all_att);
        }
    return ans;
}





//-----------------------------
k2eval::eval_vect k2eval::EvalKingSafety(const bool stm)
{
    eval_vect zeros = {0, 0};
    if(quantity[!stm][queen] < 1 && quantity[!stm][rook] < 2)
        return zeros;
    const auto k_col = get_col(king_coord(stm));
    const auto k_row = get_row(king_coord(stm));

    const eval_t attacks = CountAttacksOnKing(stm, k_col, k_row);
    const eval_t no_shelter = KingHasNoShelter(k_col, k_row, stm);
    const eval_t div = 10;
    const eval_vect f_saf = no_shelter ?
                vect_mul(king_saf_attack1, king_saf_attack2)/div :
                king_saf_attack2;
    eval_vect f_queen = {10, 10};
    if(!quantity[!stm][queen])
        f_queen = king_saf_no_queen;
    const eval_vect center = (std::abs(2*k_col - max_col) <= 1 &&
                              quantity[!stm][queen]) ? king_saf_central_files :
                                                       zeros;
    const eval_t att2 = attacks*attacks;
    const eval_vect ans = center + king_saf_no_shelter*no_shelter +
            vect_div(att2*f_saf, f_queen);

    return stm ? -ans : ans;
}





//-----------------------------
k2eval::k2eval() : material_values {{0, 0}, king_val, queen_val, rook_val,
            bishop_val, knight_val, pawn_val},
pst
{
{ //king
{{-121,-83},{62,-44},{89,-44},{25,-32}, {-7,-9},  {73,3},   {119,-34},{68,-52}},
{{113,-35}, {126,-7},{91,6},  {123,16}, {69,15},  {124,28}, {117,-7}, {81,-24}},
{{121,-17}, {82,8},  {9,26},  {-79,36}, {-51,32}, {115,43}, {127,27}, {45,-13}},
{{20,-23},  {72,6},  {-63,35},{-103,47},{-96,43}, {-73,38}, {115,7},  {8,-17}},
{{-62,-25}, {2,-3},  {-79,34},{-123,42},{-124,45},{-66,34}, {16,-1}, {-26,-23}},
{{26,-29},  {40,-5}, {-37,23},{-52,31}, {-49,34}, {-41,26}, {65,1},   {36,-24}},
{{27,-45},  {59,-18},{29,-1}, {-37,17}, {5,15},   {11,8},   {66,-14}, {63,-40}},
{{-61,-64}, {41,-46},{17,-25},{-43,-7}, {56,-35}, {-15,-35},{71,-47}, {22,-66}},
},

{ // queen
{{-86,-21},{-26,7},  {-15,-6},{-40,2}, {17,-8}, {-17,-10},{-22,-23},{12,-21}},
{{-69,-15},{-65,-1}, {-27,13},{34,11}, {-35,21},{-2,2},   {-17,-6}, {30,-42}},
{{-38,-16},{-47,-7}, {-7,-1}, {-12,22},{43,21}, {11,1},   {-15,-3}, {-10,-24}},
{{-31,-1}, {-27,8},  {-26,12},{-16,33},{12,23}, {6,12},   {-12,16}, {-17,-6}},
{{6,-13},  {-24,24}, {2,13},  {-13,36},{20,14}, {4,13},   {7,8},    {-14,-10}},
{{-7,-1},  {20,-22}, {8,7},   {4,1},   {8,1},   {1,9},    {9,1},    {8,-11}},
{{-21,-7}, {-8,-1},  {32,-12},{11,-1}, {22,-9}, {11,-15}, {-28,-19},{6,-26}},
{{11,-21}, {-15,-12},{2,-3},  {34,-8}, {2,-7},  {-34,-7}, {-47,-19},{-72,-34}},
},

{ // rook
{{45,14}, {30,11}, {6,18}, {41,14},{65,14}, {16,12}, {9,13},  {-15,17}},
{{23,13}, {11,12}, {45,8}, {45,9}, {24,5},  {34,5},  {-14,13},{2,11}},
{{13,4},  {18,7},  {13,6}, {25,7}, {-12,4}, {-12,1}, {16,-2}, {-19,-5}},
{{-12,3}, {-22,7}, {14,11},{11,1}, {3,1},   {-6,1},  {-60,-3},{-15,-4}},
{{-26,6}, {-15,10},{-3,10},{-10,9},{-9,1},  {-44,6}, {-20,-4},{-21,-5}},
{{-29,4}, {-10,4}, {-17,1},{-20,5},{-12,-3},{-29,-1},{-19,3}, {-39,1}},
{{-25,-1},{-2,-1}, {-22,1},{-5,1}, {-12,-5},{-12,-3},{-38,-3},{-63,-2}},
{{2,-2},  {3,7},   {17,4}, {7,7},  {12,2},  {1,2},   {-42,7}, {10,-24}},
},

{ // bishop
{{-99,-9}, {-50,-15},{-126,1},{-101,1},{-29,-5},{-96,-2},{-35,-12},{-32,-29}},
{{-66,-7}, {-17,-5}, {-36,7}, {-67,-4},{13,-8}, {34,-7}, {7,-13},  {-101,-12}},
{{-42,4},  {-1,-5},  {22,-5}, {1,-1},  {-5,4},  {25,-3}, {-1,1},   {-23,-3}},
{{-8,-6},  {-21,11}, {-2,8},  {43,7},  {22,6},  {30,-4}, {-15,-1}, {-10,-8}},
{{-1,-5},  {6,-2},   {2,11},  {25,13}, {26,5},  {-12,8}, {-1,-4},  {8,-13}},
{{10,-7},  {10,4},   {13,8},  {11,8},  {-2,13}, {22,5},  {6,4},    {11,-4}},
{{2,-16},  {25,-10}, {18,-7}, {-3,-2}, {3,8},   {7,-5},  {41,-5},  {-5,-28}},
{{-29,-24},{8,-15},  {-8,-13},{-2,-4}, {2,-12}, {-6,-12},{-37,-7}, {-35,-7}},
},

{ // knight
{{-126,-89},{-126,-24},{-15,7},{-58,-5},{68,-15},{-126,-16},{-121,-28},{-126,-100}},
{{-126,-5}, {-51,9},   {82,-1},{50,20}, {8,13},  {59,-6},   {16,-15},{-81,-38}},
{{-89,-3},  {33,12},   {62,36},{82,34}, {87,22}, {124,11},  {50,1},  {8,-26}},
{{-1,-3},   {21,27},   {41,47},{82,45}, {36,53}, {101,31},  {9,32},  {37,-8}},
{{-21,3},   {16,16},   {39,42},{32,52}, {47,43}, {42,42},   {47,22}, {9,-4}},
{{-22,-11}, {2,26},    {16,32},{40,38}, {55,32}, {39,26},   {28,13}, {-20,-4}},
{{-32,-25}, {-38,-1},  {9,7},  {27,14}, {8,27},  {16,6},    {-1,1},  {-1,-32}},
{{-61,-44}, {-10,-30}, {-54,-3},{-29,1},{10,-7}, {-4,-7},   {-12,-9},{-34,-59}},
},

{ // pawn
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
{{127,92}, {127,61}, {127,33}, {127,20},{127,40},{127,29}, {108,55},{124,76},},
{{85,25},  {50,22},  {38,8},   {4,-14}, {36,-19},{113,-12},{60,7},  {53,20},},
{{12,7},   {13,-4},  {-2,-8},  {19,-21},{26,-13},{14,-8},  {16,1},  {-12,3},},
{{-19,1},  {-23,-2}, {-18,-7}, {7,-16}, {9,-11}, {-8,-9},  {-20,-6},{-32,-5},},
{{-15,-16},{-30,-10},{-24,-10},{-18,-5},{-9,-3}, {-17,-2}, {-7,-13},{-14,-19},},
{{-14,-10},{-27,-9}, {-48,12}, {-35,-3},{-23,3}, {16,3},   {-7,-3}, {-18,-17},},
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
},
}

{
    InitPawnStruct();
}




#ifndef NDEBUG
//-----------------------------
void k2eval::RunUnitTests()
{
    k2chess::RunUnitTests();

    assert(king_dist(get_coord("e5"), get_coord("f6")) == 1);
    assert(king_dist(get_coord("e5"), get_coord("d3")) == 2);
    assert(king_dist(get_coord("h1"), get_coord("h8")) == 7);
    assert(king_dist(get_coord("h1"), get_coord("a8")) == 7);

    SetupPosition("5k2/8/2P5/8/8/8/8/K7 w - -");
    assert(IsUnstoppablePawn(2, white, white));
    assert(!IsUnstoppablePawn(2, white, black));
    SetupPosition("k7/7p/8/8/8/8/8/1K6 b - -");
    assert(IsUnstoppablePawn(7, black, black));
    assert(!IsUnstoppablePawn(7, black, white));
    SetupPosition("8/7p/8/8/7k/8/8/K7 w - -");
    assert(IsUnstoppablePawn(7, black, black));
    assert(!IsUnstoppablePawn(7, black, white));

    SetupPosition("3k4/p7/B1Pp4/7p/K3P3/7P/2n5/8 w - -");
    assert(IsPasser(0, black));
    assert(IsPasser(2, white));
    assert(!IsPasser(3, black));
    assert(!IsPasser(4, white));
    assert(!IsPasser(7, black));
    assert(!IsPasser(7, white));
    assert(!IsPasser(1, black));

    eval_vect val, zeros = {0, 0};
    SetupPosition("4k3/1p6/8/8/1P6/8/1P6/4K3 w - -");
    val = EvalPawns(white, wtm);
    assert(val == -pawn_dbl_iso);
    val = EvalPawns(black, wtm);
    assert(val == pawn_iso);
    SetupPosition("4k3/1pp5/1p6/8/1P6/1P6/1P6/4K3 w - -");
    val = EvalPawns(white, wtm);
    assert(val == -pawn_dbl_iso);
    val = EvalPawns(black, wtm);
    assert(val == pawn_dbl);
    SetupPosition("4k3/2p1p1p1/2Np1pPp/7P/1p6/Pr1PnP2/1P2P3/4K3 b - -");
    val = EvalPawns(white, wtm);
    assert(val == static_cast<eval_t>(-2)*pawn_hole - pawn_gap);
    val = EvalPawns(black, wtm);
    assert(val == pawn_hole + pawn_gap);
    SetupPosition("8/8/3K1P2/3p2P1/1Pkn4/8/8/8 w - -");
    val = EvalPawns(white, wtm);
    assert(val == (eval_t)(3*3 + 4*4 + 5*5)*pawn_pass2 +
           (eval_t)(3 + 4 + 5)*pawn_pass1 + (eval_t)3*pawn_pass0 - pawn_iso +
           (eval_t)2*pawn_king_tropism3 - pawn_king_tropism1 -
           (eval_t)3*pawn_king_tropism2 + (eval_t)5*pawn_pass_connected);
    val = EvalPawns(black, wtm);
    assert(val == (eval_t)(-3*3)*pawn_blk_pass2 - (eval_t)3*pawn_blk_pass1 -
           pawn_blk_pass0 + pawn_iso - pawn_king_tropism1 -
           (eval_t)3*pawn_king_tropism2 + pawn_king_tropism3);

    // endings
    val = SetupPosition("5k2/8/8/N7/8/8/8/4K3 w - -");  // KNk
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("5k2/8/8/n7/8/8/8/4K3 w - -");  // Kkn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("5k2/8/8/B7/8/8/8/4K3 w - -");  // KBk
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("5k2/8/8/b7/8/8/8/4K3 w - -");  // Kkb
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("5k2/p7/8/b7/8/8/P7/4K3 w - -");  // KPkbp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < bishop_val.imag());
    val = SetupPosition("5k2/8/8/n7/8/8/P7/4K3 w - -");  // KPkn
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("5k2/p7/8/B7/8/8/8/4K3 w - -");  // KBkp
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("5k2/p6p/8/B7/8/8/7P/4K3 w - -");  // KBPkpp
    val = EvalImbalances(wtm, val);
    assert(val.imag() > 2*pawn_val.imag());
    val = SetupPosition("5k2/8/8/NN6/8/8/8/4K3 w - -");  // KNNk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/nn6/8/8/8/4K3 w - -");  // Kknn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/p7/8/nn6/8/8/P7/4K3 w - -");  // KPknnp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -2*knight_val.imag() + pawn_val.imag());
    val = SetupPosition("5k2/8/8/nb6/8/8/8/4K3 w - -");  // Kknb
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -knight_val.imag() - bishop_val.imag() +
           pawn_val.imag());
    val = SetupPosition("5k2/8/8/1b6/8/7p/8/7K w - -");  // Kkbp not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -bishop_val.imag()- pawn_val.imag()/2);
    val = SetupPosition("7k/8/7P/1B6/8/8/8/6K1 w - -");  // KBPk drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/3b4/p7/8/K7/8 w - -");  // Kkbp drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("7k/8/P7/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > bishop_val.imag() + pawn_val.imag()/2);
    val = SetupPosition("2k5/8/P7/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > bishop_val.imag() + pawn_val.imag()/2);
    val = SetupPosition("1K6/8/P7/1B6/8/8/8/6k1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > bishop_val.imag() + pawn_val.imag()/2);
    val = SetupPosition("1k6/8/1P6/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > bishop_val.imag() + pawn_val.imag()/2);
    val = SetupPosition("5k2/8/8/3b4/p4P2/8/K7/8 w - -");  // KPkbp drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/3b4/p4P2/8/8/2K5 w - -");  // KPkbp not drawn
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -bishop_val.imag());

    val = SetupPosition("1k6/8/8/P7/8/4N3/8/6K1 w - -");  // KNPk
    val = EvalImbalances(wtm, val);
    assert(val.imag() > knight_val.imag() + pawn_val.imag()/2);
    val = SetupPosition("8/8/8/5K2/2k5/8/2P5/8 b - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/5K2/2k5/8/2P5/8 w - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val.imag() > pawn_val.imag()/2);
    val = SetupPosition("k7/2K5/8/P7/8/8/8/8 w - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/p7/8/1k6/8/1K6 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/p7/8/1k6/8/2K5 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/2);
    val = SetupPosition("8/8/8/pk6/8/K7/8/8 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/2);
    val = SetupPosition("8/8/8/1k5p/8/8/8/K7 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/2);

    SetupPosition("7k/5B2/5K2/8/3N4/8/8/8 w - -");  // KBNk
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == 0);
    SetupPosition("7k/4B3/5K2/8/3N4/8/8/8 w - -");  // KBNk
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == imbalance_king_in_corner.imag());
    SetupPosition("k7/5n2/8/8/8/b7/8/7K w - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == 0);
    SetupPosition("k7/5n2/8/8/8/b7/8/K7 w - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == -imbalance_king_in_corner.imag());
    SetupPosition("8/8/8/8/8/2k2n2/4b3/1K6 b - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == 0);
    SetupPosition("8/1K6/8/8/8/2k2n2/4b3/8 b - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == 0);

    // imbalances with opponent king near telestop
    val = SetupPosition("6k1/4R3/8/8/5KP1/8/3r4/8 b - -");  // KRPkr
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("6k1/4Q3/8/8/5KP1/8/3q4/8 b - -");  // KQPkq
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("8/4B2k/8/8/5KP1/8/3n4/8 b - -");  // KBPkn
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag()/4);
    val = SetupPosition("8/2n1B3/8/8/6k1/1p6/8/1K6 b - -");  // KBknp
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("8/2n1B3/8/8/6K1/1p6/8/1k6 b - -");  // KBknp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/4);
    val = SetupPosition("1r6/8/6k1/8/8/1p6/1K6/1R6 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("6k1/4R3/8/5K2/6P1/8/3r4/8 b - -");  // KRPkr
    val = EvalImbalances(wtm, val);
    assert(val.imag() > pawn_val.imag()/4);
    val = SetupPosition("2r5/4R3/8/8/3p4/4k3/8/3K4 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/4);
    val = SetupPosition("2r5/4R3/8/8/3p4/8/8/3K2k1 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag()/4);
    val = SetupPosition("8/3k4/6K1/4R3/8/6p1/7r/8 w - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag()/4);

    val = SetupPosition("8/4R3/3n1K2/8/1k6/8/8/6r1 b - -");  // KRkrn
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag());
    val = SetupPosition("4R3/8/1K6/8/8/2B2k2/8/3r4 b - -");  // KRBkr
    val = EvalImbalances(wtm, val);
    assert(val.imag() > -pawn_val.imag());
    val = SetupPosition("8/4R2K/3n4/8/1k6/8/8/6r1 b - -");  // KRkrn
    val = EvalImbalances(wtm, val);
    assert(val.imag() < -pawn_val.imag());

    // bishop pairs
    SetupPosition("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == bishop_pair);
    SetupPosition("1nb1kb2/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == -bishop_pair);
    SetupPosition("1nb1k1b1/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == -bishop_pair);
    SetupPosition("1nb1kbb1/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == zeros);  // NB

    val = SetupPosition("4kr2/8/8/8/8/8/8/1BBNK3 w - -"); // pawn absense
    val = EvalImbalances(wtm, val);
    assert(val.imag() < rook_val.imag()/2);

    // multicolored bishops
    val = SetupPosition("rn1qkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq -");
    auto tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag() && val.imag() < tmp.imag());
    val = SetupPosition("rnbqk1nr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.imag() < pawn_val.imag() && val.imag() < tmp.imag());
    val = SetupPosition("rnbqk1nr/p1pppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == tmp.imag());
    val = SetupPosition("rn1qkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.imag() == tmp.imag());
    val = SetupPosition("4kb2/pppp4/8/8/8/8/PPPPPPPP/4KB2 w - -");
    val = EvalImbalances(wtm, val);
    assert(val.imag() < 5*pawn_val.imag()/2);

    //king safety
    SetupPosition("4k3/4p3/8/8/8/4P3/5P2/4K3 w - -");
    assert(Sheltered(4, 0, white));
    assert(Sheltered(4, 7, black));
    SetupPosition("4k3/4r3/4p3/8/8/4B3/4R3/4K3 w - -");
    assert(Sheltered(4, 0, white));
    assert(Sheltered(4, 7, black));
    SetupPosition("4k3/8/4b3/8/8/4P3/3N4/4K3 w - -");
    assert(!Sheltered(4, 0, white));
    assert(!Sheltered(4, 7, black));
    SetupPosition("4k3/4q3/8/8/8/8/8/4K3 w - -");
    assert(!Sheltered(4, 0, white));
    assert(!Sheltered(4, 7, black));

    SetupPosition("q5k1/pp3p1p/6p1/8/8/8/PP3PPP/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp4pp/5p2/8/8/7P/PP3PP1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp3pbp/6n1/8/8/6P1/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp3nbr/5nnq/8/8/5P1N/PP4PB/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp4bn/6nr/8/8/5P2/PP4P1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("4k2r/pp6/8/8/8/8/PP6/Q5K1 w k -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(4, 7, black) == false);
    SetupPosition("q3kr2/pp1p1p2/8/8/5P1P/8/PP4P1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);
    SetupPosition("q5k1/pp3p1p/7p/8/8/5P2/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);
    SetupPosition("q5k1/pp4p1/6p1/8/8/6N1/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);

    SetupPosition("6k1/pp1N1ppp/6r1/8/4n2q/B3p3/PP4P1/Q4RK1 w - -");
    assert(CountAttacksOnKing(white, 6, 0) == 3 + 1 + 1 + 0 + 1);
    assert(CountAttacksOnKing(black, 6, 7) == 1 + 0 + 0 + 2 + 0);
    SetupPosition("2q2rk1/pp3pp1/2r4p/2r5/8/2P4Q/PPP2PPR/1K5R w - -");
    assert(CountAttacksOnKing(white, 1, 0) == 0);
    assert(CountAttacksOnKing(black, 6, 7) == 0);
    SetupPosition("2q2r2/pp3pk1/2r3p1/2r5/8/7Q/PPP2PPR/1K5R w - -");
    assert(CountAttacksOnKing(white, 1, 0) == 0 + 0 + 2 + 0 + 0);
    assert(CountAttacksOnKing(black, 6, 6) == 0+0+2 + 0+0+2 + 0+0+2);
}
#endif // NDEBUG
