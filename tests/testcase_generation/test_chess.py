import string
import itertools

files = string.ascii_uppercase[:8]
ranks = list(range(1, 9))

def AN_To_Bitboard_1():
    for i, an in enumerate([file + str(rank) for rank, file in itertools.product(ranks, files)]):
        print(f'ASSERT_EQ(1ULL << {i}, SCE_AN_To_Bitboard("{an}"));')

def Bitboard_To_AN():
    for i, an in enumerate([file + str(rank) for rank, file in itertools.product(ranks, files)]):
        print(f'ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << {i}U), SCE_SUCCESS);')
        print(f'ASSERT_EQ(memcmp(an, "{an}", 2U), 0);')

def Square_Under_Attack_3():
    squares = [file + str(rank) for rank, file in itertools.product(ranks, files)]
    attacked_by_white = [
        "D5",
        "F5",
        "A6",
        "B5",
        "C4",
        "E4",
        "A3",
        "B3",
        "C3",
        "D3",
        "E3",
        "F3",
        "G3",
        "H3",
        "A2",
        "B2",
        "D2",
        "D1",
        "E2",
        "F2",
        "F1",
        "F3",
        "H3",
        "G1",
        "H2",
        "B1",
        "E1",
        "C2",
        "G4"
    ]
    for square in filter(lambda x: x not in attacked_by_white, squares):
        print(f'ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("{square}"), WHITE));')


if __name__ == "__main__":
    pass
