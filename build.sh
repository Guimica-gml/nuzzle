CFLAGS="-Wall -Wextra -std=c11 -pedantic"
CLIBS="-lSDL2 -lSDL2_image -lm"

gcc $CFLAGS -o nuzzle ./main.c $CLIBS
RESULT=$?

if [ $RESULT -eq 0 ]; then
    if [ "$1" = "run" ]; then
        ./nuzzle $2
    fi
fi
