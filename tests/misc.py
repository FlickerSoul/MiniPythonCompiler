def test0(a: int) -> None:
    print(a)
    return

def test1(a: int) -> str:
    return str(a)

def test2(a: int, b: int) -> int:
    return a * b

def test4(a: int, b: int) -> int:
    a -= b
    return a * b

def test5(a: str, b: int) -> str:
    a = a * b
    return a

print(test0(0))
print(test1(1))
print(test2(2, 3))
print(test4(4, 5))
print(test5("6", 7))

