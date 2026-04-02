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



if __name__ == "__main__":
    pass
