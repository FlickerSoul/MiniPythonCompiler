def test(a: int) -> int:
    print(a)
    return a

def test2(a: int) -> None:
    a += 2
    print(a)
    return

test(10)
test2(20)
