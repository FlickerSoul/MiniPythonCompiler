def test(a: int) -> None:
    if a < 10:
        print("less than 10")
    elif a < 20:
        print("less than 20 but gt 10")
    else:
        print("greater than 20")
    
    return 

test(1)
test(12)
print(test(25))
