import string
import itertools

files = string.ascii_uppercase[:8]
ranks = list(range(1, 9))

def AN_To_Bitboard_1():
    for i, an in enumerate([file + str(rank) for rank, file in itertools.product(ranks, files)]):
        print(f'ASSERT_EQ(1ULL << {i}, SCE_AN_To_Bitboard("{an}"));')


if __name__ == "__main__":
    pass
