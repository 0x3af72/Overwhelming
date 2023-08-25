#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_image.h"

using namespace std;

#pragma once

SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path){
    SDL_Surface* temp_surface = IMG_Load(path);
    if (!temp_surface){
        cout << "Unable to load image: " << path << "\n";
    }
    SDL_Texture* loaded_texture = SDL_CreateTextureFromSurface(renderer, temp_surface);
    SDL_FreeSurface(temp_surface);
    return loaded_texture;
}

void centerRect(SDL_Rect& rect){
    rect.x -= rect.w / 2;
    rect.y -= rect.h / 2;
}

int angle(double posx, double posy, double targetx, double targety){
    return atan2(targetx - posx, targety - posy) * (180.0 / M_PI);
}

float radians(int angle){
    return angle * M_PI / 180;
}