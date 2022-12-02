def test(a: int) -> int:
    i: int = 0
    while i < a:
        b: int = 10
        i = i + 1
    return b

test(5)
