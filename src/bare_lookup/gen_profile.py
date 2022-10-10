LAST_CLOSE = 585.3
AS_UNIT = 0.01

if __name__ == "__main__":
    BOOK_SIZE = (int)(LAST_CLOSE/10/AS_UNIT*2)
    with open("profile.hpp", "w") as f:
        f.write(f"\n#define LAST_CLOSE {LAST_CLOSE}")
        f.write(f"\n#define AS_UNIT {AS_UNIT}")
        f.write(f"\n#define BOOK_SIZE {BOOK_SIZE}")
