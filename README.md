# Snake
Bad implementation of snake in C using SDL2 and SDL2_TTF made mainly for educational purposes. The playlist in which I write all of this code (discussed in italian) can be found here: !TBD!

# Dependencies
The only dependencies are SDL2 and SDL2_TTF.

In arch these can be installed with

```
sudo pacman -S sdl2
sudo pacman -S sdl2_ttf
```

In ubuntu instead we have

```
sudo apt-get install libsdl2-dev
sudo apt-get install libsdl2-ttf-dev
```

Other than that, you need to specify the path to a .ttf file in one of the parameters at the top of the code.

Also, this probably does not work on Windows (OS) because of gettimeofday() used for managing delays.

# Compilation

Once you have the dependencies install simply do

```
make
./main
```

and it should run.
