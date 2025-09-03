#include <SDL.h>
#include <SDL_ttf.h>
#include <fftw3.h>
#include <iostream>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    fftw_complex in[1], out[1];
    fftw_plan p = fftw_plan_dft_1d(1, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    TTF_Quit();
    SDL_Quit();
    std::cout << "Dependencies linked successfully." << std::endl;
    return 0;
}
