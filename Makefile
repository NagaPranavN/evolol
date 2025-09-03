TARGET = evolol
SRC = src/main.c
CC = clang
CFLAGS = -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 -Wall -Isrc
LDFLAGS = -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 -framework SDL3 -framework Cocoa -Wl,-rpath,/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
