def f(x : int) -> int:
    y : int = x*x
    x : bool = x*x > 42
    while x:
        y -= 1
        if y % 2 == 1:
            x : int = y // 2
            y -= x
        else:
            y = y // 2
        x = y > 10
    return y

f(20)

