#include <iostream>
#include <unordered_map>
#include <algorithm>
#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_image.h"
#include "SDL2/include/SDL2/SDL_mixer.h"

using namespace std;

#pragma once

unordered_map<string, Mix_Chunk*> cached_chunks;
unordered_map<string, Mix_Music*> cached_music;
unordered_map<string, int> alloc_channels; // different channel for each chunk

Mix_Chunk* loadChunkWav(string path){
    Mix_Chunk* returned_audio = Mix_LoadWAV(path.c_str());
    if (returned_audio == NULL){
        cout << "Error loading audio: " << Mix_GetError() << "\n";
    }
    return returned_audio;
}

void playChunkWav(string path){

    Mix_Chunk* played_chunk;
    
    // check if chunk is cached
    if (cached_chunks.find(path) != cached_chunks.end()){

        played_chunk = cached_chunks[path];

    } else { // load and cache the chunk

        played_chunk = loadChunkWav(path);
        cached_chunks[path] = played_chunk;
        alloc_channels[path] = alloc_channels.size();

    }

    Mix_PlayChannel(alloc_channels[path], played_chunk, 0);
}

void playMusicWav(string path, int loops){

    Mix_Music* played_music;

    // check if music is cached
    if (cached_music.find(path) != cached_music.end()){

        played_music = cached_music[path];

    } else { // load and cache the music

        played_music = Mix_LoadMUS(path.c_str());
        cached_music[path] = played_music;

    }

    if (Mix_PlayingMusic()){
        Mix_FadeOutMusic(1000);
    }

    Mix_PlayMusic(played_music, loops);

}