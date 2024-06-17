arr = [1, 2, 3, 4, 5, 6, 7, 8, 9]


def shift(arr, k):
    assert(k < len(arr))
    arr = arr.copy()
    # if k == 0: 
    #     return arr
    arr.reverse()
    return arr[:-k:][::-1] + arr[-k::][::-1]

assert(shift(arr, 0) == arr)
assert(shift(arr, 2) == [3, 4, 5, 6, 7, 8, 9, 1, 2])
assert(shift(arr, 6) == [7, 8, 9, 1, 2, 3, 4, 5, 6])
