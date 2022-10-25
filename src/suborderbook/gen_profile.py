LAST_CLOSE = 585.33
AS_UNIT = 0.01

RANGE = 20
CHAIN_LEVELS = 200

if __name__ == "__main__":
    SLOTSIZE = (LAST_CLOSE/20/AS_UNIT//RANGE)+1
    with open("profile.hpp", "w") as f:
        f.write(f"\n#define RANGE {RANGE}")
        f.write(f"\n#define CHAIN_LEVELS {CHAIN_LEVELS}")
        f.write(f"\n#define AS_UNIT {AS_UNIT}")
        f.write(f"\n#define AS_SLOTSIZE {SLOTSIZE}")
