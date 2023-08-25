#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_image.h"
#include "SDL2/include/SDL2/SDL_mixer.h"

#include "functions.hpp"
#include "game-classes.hpp"
#include "audio.hpp"

using namespace std;

int main(int argc, char* argv[]){

    // hide console window
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    
    // setup SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // set screen size and texture display sizes based screen size
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    float screen_width = 600;
    float screen_height = 700;
    display_mode.h -= 60; // take into account taskbars
    if (screen_height > display_mode.h){
        screen_width = float(screen_width) / float(screen_height) * float(display_mode.h);
        screen_height = display_mode.h;
    }
    camera.scaleBy(screen_width, screen_height, 600, 700);

    // window, renderer
    SDL_Window *window = SDL_CreateWindow("Overwhelming", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height,  SDL_RENDERER_ACCELERATED | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_SetWindowIcon(window, IMG_Load("assets/icon/icon.png"));

    // audio device init
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
    Mix_AllocateChannels(16);

    // font renderer
    FontRenderer font_renderer(renderer, "assets/font/");

    // random
    random_device r;
    default_random_engine rand_generator(r());

    // fps capping
    const int FPS = 120;
    const float FRAME_DELAY = 1000 / FPS;
    Uint32 framestart;
    int frametime;

    // game variables
    bool running = true;
    long long game_ticks;

    const int PLAYING = 1;
    const int MENU = 2;
    const int DEATH_SCREEN = 3;
    int game_state = MENU;

    // missiles
    vector<Missile> missiles;
    SDL_Texture *missile_texture = loadTexture(renderer, "assets/missile/missile.png");

    // particles
    vector<Particle> particles;
    vector<SDL_Texture*> particle_textures = {
        loadTexture(renderer, "assets/particle/red_circle.png"),
        loadTexture(renderer, "assets/particle/orange_circle.png"),
        loadTexture(renderer, "assets/particle/white_circle.png"),
        loadTexture(renderer, "assets/particle/green_circle.png"), // used in enemy explosions
    };
    for (SDL_Texture *texture: particle_textures){
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    uniform_int_distribution<int> thruster_particle_angle(115, 235);
    uniform_int_distribution<int> thruster_particle_texture(0, 2); // skip green texture
    uniform_int_distribution<int> thruster_particle_size(10, 20);
    uniform_int_distribution<int> thruster_particle_mult(100, 200);

    uniform_int_distribution<int> death_particle_angle(160, 200);
    uniform_int_distribution<int> death_particle_size(10, 20);
    uniform_int_distribution<int> death_particle_mult(500, 700);

    // enemies
    vector<Enemy> enemies;
    vector<EnemyAttack> enemy_attacks_vec;
    unordered_map<string, unordered_map<string, float>> enemy_data = {
        {"soldier", {
            {"speed", 1},
            {"width", 80},
            {"height", 94},
            {"frame_delay_ticks", 10},
            {"health", 500},
            {"shot_cooldown", 40},
            {"shot_num", 1},
            {"attack_damage", 20},
            {"attack_width", 20},
            {"attack_height", 20},
            {"attack_xvel_mult", 1},
            {"attack_yvel_mult", 3},
            {"attack_rotation_vel", 0},
            {"attack_next_frame_ticks", 1},
        }},
        {"compass", {
            {"speed", 0.5f},
            {"width", 100},
            {"height", 100},
            {"frame_delay_ticks", 10},
            {"health", 1000},
            {"shot_cooldown", 90},
            {"shot_num", 8},
            {"attack_damage", 40},
            {"attack_width", 20},
            {"attack_height", 20},
            {"attack_xvel_mult", 2},
            {"attack_yvel_mult", 2},
            {"attack_rotation_vel", 45},
            {"attack_next_frame_ticks", 1},
        }},
        {"shotgun", {
            {"speed", 0.5f},
            {"width", 150},
            {"height", 92},
            {"frame_delay_ticks", 10},
            {"health", 1500},
            {"shot_cooldown", 70},
            {"shot_num", 5},
            {"attack_damage", 30},
            {"attack_width", 20},
            {"attack_height", 20},
            {"attack_xvel_mult", 2},
            {"attack_yvel_mult", 2},
            {"attack_rotation_vel", 10},
            {"attack_next_frame_ticks", 1},
        }},
        {"sprayer", {
            {"speed", 0.7f},
            {"width", 150},
            {"height", 131},
            {"frame_delay_ticks", 10},
            {"health", 4000},
            {"shot_cooldown", 2},
            {"shot_num", 1},
            {"attack_damage", 30},
            {"attack_width", 20},
            {"attack_height", 20},
            {"attack_xvel_mult", 2},
            {"attack_yvel_mult", 2},
            {"attack_rotation_vel", 15},
            {"attack_next_frame_ticks", 1},
        }},
    };

    unordered_map<string, vector<SDL_Texture*>> enemy_textures = {
        {"soldier", {
            loadTexture(renderer, "assets/soldier/frames/frame1.png"),
            loadTexture(renderer, "assets/soldier/frames/frame2.png"),
        }},
        {"compass", {
            loadTexture(renderer, "assets/compass/frames/frame1.png"),
        }},
        {"shotgun", {
            loadTexture(renderer, "assets/shotgun/frames/frame1.png"),
        }},
        {"sprayer", {
            loadTexture(renderer, "assets/sprayer/frames/frame1.png"),
        }},
    };

    unordered_map<string, vector<SDL_Texture*>> enemy_attacks = {
        {"soldier", {
            loadTexture(renderer, "assets/soldier/attacks/frame1.png"),
        }},
        {"compass", {
            loadTexture(renderer, "assets/compass/attacks/frame1.png"),
        }},
        {"shotgun", {
            loadTexture(renderer, "assets/shotgun/attacks/frame1.png"),
        }},
        {"sprayer", {
            loadTexture(renderer, "assets/sprayer/attacks/frame1.png"),
        }},
    };

    unordered_map<string, bool> has_particles = {
        {"soldier", true},
        {"compass", true},
        {"shotgun", true},
        {"sprayer", false},
    };

    // set blend mode for enemy textures
    for (auto [type, texture]: enemy_textures){
        for (SDL_Texture* enemy_texture: texture){
            SDL_SetTextureBlendMode(enemy_texture, SDL_BLENDMODE_BLEND);
        }
    }

    // enemy spawning
    int max_enemies = 3;
    vector<string> choose_enemies = {
        "soldier", "compass", "shotgun",
    };
    vector<string> choose_bosses = {
        "sprayer",
    };
    int next_spawn_ticks = 0;
    int spawned_already = 0;
    int next_wave_ticks = 0;
    uniform_int_distribution<int> rand_spawn_ticks(120, 180);
    uniform_int_distribution<int> rand_enemy_index(0, choose_enemies.size() - 1);
    uniform_int_distribution<int> rand_boss_index(0, choose_bosses.size() - 1);

    // waves
    int wave = 1;

    // explosions
    vector<Explosion> explosions;
    vector<SDL_Texture*> explosion_texture = {
        loadTexture(renderer, "assets/hit/hit1.png"),
        loadTexture(renderer, "assets/hit/hit2.png"),
        loadTexture(renderer, "assets/hit/hit3.png"),
        loadTexture(renderer, "assets/hit/hit4.png"),
        loadTexture(renderer, "assets/hit/hit5.png"),
    };

    // wave text
    float wave_text_x = -300;
    bool wave_start = true;

    // fake glow
    SDL_Texture* glow_vignette = loadTexture(renderer, "assets/glow/glow.png");
    SDL_SetTextureBlendMode(glow_vignette, SDL_BLENDMODE_BLEND);
    SDL_Rect glow_rect = {0, 0, 20, 20};
    vector<pair<SDL_Rect, int>> glow_locs; // render all glows at once (faster?)

    // scrolling background
    SDL_Texture* background_texture = loadTexture(renderer, "assets/background/background.jpg");
    SDL_Rect background_1 = {-10, 0, 620, 700};
    SDL_Rect background_2 = {-10, -700, 620, 700};

    // healthbar
    HealthBar health_bar(renderer, "assets/healthbar/healthbar.jpg", 500, 50);

    // player
    Player player(renderer);

    // menu attacks
    vector<EnemyAttack> menu_attacks;
    int new_attack_angle = 0;

    // menu selections
    unordered_map<string, unordered_map<string, int>> menu_selections = {
        {"play", {
            {"x", 300},
            {"y", 350},
            {"size", 50},
        }},
        {"options", {
            {"x", 300},
            {"y", 420},
            {"size", 50},
        }},
        {"exit", {
            {"x", 300},
            {"y", 490},
            {"size", 50},
        }},
    };

    unordered_map<string, string> up_selections = {
        {"exit", "options"},
        {"options", "play"},
        {"play", "play"},
    };

    unordered_map<string, string> down_selections = {
        {"play", "options"},
        {"options", "exit"},
        {"exit", "exit"},
    };

    string menu_selected = "play";

    // arrow next to current menu selection
    SDL_Texture* selection_arrow_texture = loadTexture(renderer, "assets/arrow/arrow.png");
    SDL_Rect selection_arrow_rect = {0, 0, 20, 24};

    // menu dim
    SDL_Texture* dim_texture = loadTexture(renderer, "assets/dim/dim.png");
    SDL_Rect dim_rect = {-100, -100, 1000, 1000};
    SDL_SetTextureBlendMode(dim_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(dim_texture, 125);

    // player death explosion
    Explosion player_death_explosion(player.x, player.y, explosion_texture, player.rect.w, player.rect.h, 10);
    bool player_exploded = false;

    // death black background
    SDL_Texture *death_transition_background = loadTexture(renderer, "assets/death/transition_background.png");
    SDL_Rect death_transition_rect = {0, 0, 600, 700};
    int death_transition_alpha = 0;
    SDL_SetTextureBlendMode(death_transition_background, SDL_BLENDMODE_BLEND);

    // main loop
    while (running){

        game_ticks += 1;

        // menu stuff
        if (game_state == MENU){

            // handle events
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0){;
                if (event.type == SDL_QUIT){
                    running = false;
                } else if (event.type == SDL_KEYDOWN){
                    SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_RETURN){
                        if (menu_selected == "play"){
                            // game state
                            game_state = PLAYING;

                            // reset menu stuff
                            menu_attacks.clear();

                            // start game music
                            playMusicWav("audio/background-music.wav", -1);

                        } else if (menu_selected == "options"){

                        } else if (menu_selected == "exit"){
                            running = false;
                        }

                    } else if (key == SDLK_DOWN){
                        menu_selected = down_selections[menu_selected];
                    } else if (key == SDLK_UP){
                        menu_selected = up_selections[menu_selected];
                    }
                }
            }

            // mouse stuff
            int mousex, mousey;
            SDL_GetMouseState(&mousex, &mousey);

            new_attack_angle += 4;
            menu_attacks.push_back(EnemyAttack(-10, -10, 20, 20, enemy_attacks["soldier"], 0, sin(radians(new_attack_angle)), cos(radians(new_attack_angle)), 1));

            // renderer
            // clear render buffer
            SDL_RenderClear(renderer);

            // glow clear
            glow_locs.clear();

            // scrolling backgrounds
            background_1.y += 4;
            background_2.y += 4;

            // scrolling system
            if (background_1.y >= 700){
                background_1.y = -700;
            } else if (background_2.y >= 700){
                background_2.y = -700;
            }

            // cheap solution to avoiding gaps between two backgrounds by only moving x of the backgrounds.
            SDL_Rect new_background_1 = {background_1.x, background_1.y - camera.y, background_1.w, background_1.h};
            SDL_Rect new_background_2 = {background_2.x, background_2.y - camera.y, background_2.w, background_2.h};

            camera.renderCopy(renderer, background_texture, NULL, &new_background_1);
            camera.renderCopy(renderer, background_texture, NULL, &new_background_2);

            // attack update
            vector<EnemyAttack> new_menu_attacks;
            for (EnemyAttack& attack: menu_attacks){
                
                // fake glow around attack
                glow_rect = {int(attack.x), int(attack.y), int(attack.glow_radius), int(attack.glow_radius)};
                centerRect(glow_rect);
                pair<SDL_Rect, int> new_loc = {glow_rect, 255};
                glow_locs.push_back(new_loc);

                bool still_alive = true;
                int res = attack.update(player);
                if (res == attack.OUT_OF_SCREEN){
                    still_alive = false;
                }

                // add if still alive
                if (still_alive){
                    new_menu_attacks.push_back(attack);
                }
            }
            menu_attacks = new_menu_attacks;

            // show glows
            for (auto [rect, alpha]: glow_locs){
                SDL_SetTextureAlphaMod(glow_vignette, alpha);
                camera.renderCopy(renderer, glow_vignette, NULL, &rect);
            }

            // clear glow locs
            glow_locs.clear();

            // show enemy attacks
            for (EnemyAttack& enemy_attack: menu_attacks){
                camera.renderCopy(renderer, enemy_attack.textures[enemy_attack.curr_frame], NULL, &enemy_attack.rect);
            }

            // menu dim
            SDL_SetTextureAlphaMod(dim_texture, 125);
            camera.renderCopy(renderer, dim_texture, NULL, &dim_rect);

            // show game title
            font_renderer.renderTextCentered(renderer, "overwhelming", 300, 250, 60, 230, 230, 230);

            // selections
            for (auto& [selection, data]: menu_selections){

                // size animation
                if (menu_selected == selection){
                    // increase size
                    if (data["size"] != 60){
                        data["size"] += 1;
                    }
                } else {
                    if (data["size"] != 50){
                        data["size"] -= 1;
                    }
                }

                // show
                int text_width = font_renderer.renderTextCentered(renderer, selection, data["x"], data["y"], data["size"], 230, 230, 230);

                // arrows around if selected
                if (menu_selected == selection){

                    // left arrow
                    selection_arrow_rect.x = data["x"] - selection_arrow_rect.w - text_width / 2 - 10;
                    selection_arrow_rect.y = data["y"] - selection_arrow_rect.h + 9;
                    camera.renderCopy(renderer, selection_arrow_texture, NULL, &selection_arrow_rect);

                    // right arrow
                    selection_arrow_rect.x = data["x"] + text_width / 2 - 3;
                    camera.renderCopyEx(renderer, selection_arrow_texture, NULL, &selection_arrow_rect, 0, NULL, SDL_FLIP_HORIZONTAL);
                }
            }

        }

        else if (game_state == DEATH_SCREEN){

            // handle events
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0){;
                if (event.type == SDL_QUIT){
                    running = false;
                } else if (event.type == SDL_KEYDOWN){
                    SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_RETURN){
                        // back to menu
                        game_state = MENU;
                    }
                }
            }

            // mouse stuff
            int mousex, mousey;
            SDL_GetMouseState(&mousex, &mousey);

            // renderer
            // clear render buffer
            SDL_RenderClear(renderer);

            // death background
            SDL_SetTextureAlphaMod(death_transition_background, 255);
            camera.renderCopy(renderer, death_transition_background, NULL, &death_transition_rect);

            // death message
            font_renderer.renderTextCentered(renderer, "you died", 300, 200, 70, 255, 255, 255);
            font_renderer.renderTextCentered(renderer, "press enter to continue", 300, 500, 30, 255, 255, 255);

        }

        // game stuff
        else if (game_state == PLAYING){

            // handle events
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0){;
                if (event.type == SDL_QUIT){
                    running = false;
                } else if (event.type == SDL_KEYDOWN){
                    SDL_Keycode key = event.key.keysym.sym;
                    if (key == SDLK_g){
                        string enemy_type = "compass";
                        enemies.push_back(Enemy(
                            enemy_type,
                            enemy_textures[enemy_type],
                            300, 350,
                            enemy_data[enemy_type]["attack_rotation_vel"],
                            enemy_data[enemy_type]["attack_xvel_mult"],
                            enemy_data[enemy_type]["attack_yvel_mult"],
                            enemy_data[enemy_type]["width"],
                            enemy_data[enemy_type]["height"],
                            enemy_data[enemy_type]["speed"],
                            enemy_data[enemy_type]["frame_delay_ticks"],
                            enemy_data[enemy_type]["health"],
                            enemy_data[enemy_type]["shot_cooldown"],
                            enemy_data[enemy_type]["shot_num"]
                        ));
                    } else if (key == SDLK_h){
                        playChunkWav("audio/next-wave.wav");
                    }
                }
            }

            // mouse stuff
            int mousex, mousey;
            SDL_GetMouseState(&mousex, &mousey);

            // movement, shooting only available if player is still alive
            if (player.health > 0){

                // keyboard pressed keys
                const Uint8* keystates = SDL_GetKeyboardState(NULL);
                if (keystates[SDL_SCANCODE_A]){
                    if (player.display_rect.x > 0){
                        player.x -= player.speed;
                    }
                }
                if (keystates[SDL_SCANCODE_D]){
                    if (player.display_rect.x + player.display_rect.w < 600){
                        player.x += player.speed;
                    }
                }
                if (keystates[SDL_SCANCODE_W]){
                    if (player.display_rect.y > 0){
                        player.y -= player.speed;
                    }
                }
                if (keystates[SDL_SCANCODE_S]){
                    if (player.display_rect.y + player.display_rect.h < 700){
                        player.y += player.speed;
                    }
                }

                // player shooting
                if (game_ticks % 10 == 0){
                    playChunkWav("audio/player-shot.wav");
                    missiles.push_back(Missile(missile_texture, player.display_rect.x, player.display_rect.y, 6));
                    missiles.push_back(Missile(missile_texture, player.display_rect.x + player.display_rect.w, player.display_rect.y, 6));
                }

            }

            // spawn enemies

            if (next_wave_ticks > 0){
                next_wave_ticks -= 1;
            }

            if (next_spawn_ticks){

                next_spawn_ticks -= 1;

            } else {

                next_spawn_ticks = rand_spawn_ticks(rand_generator);
                
                // spawn if less than max
                int real_max = (1 + (max_enemies - 1) * (wave % 5 != 0)); // 1 max for every 5 waves (boss waves)
                if (int(enemies.size()) < real_max && next_wave_ticks == 0 && spawned_already != real_max){

                    // random enemy
                    string enemy_type = (real_max == 1) ? choose_bosses[rand_boss_index(rand_generator)] : choose_enemies[rand_enemy_index(rand_generator)];
                    
                    // random spawn position (cannot go offscreen)
                    uniform_int_distribution<int> rand_x_spawn(0 + enemy_data[enemy_type]["width"] / 2, 600 - enemy_data[enemy_type]["width"] / 2);
                    uniform_int_distribution<int> rand_y_spawn(0 + enemy_data[enemy_type]["height"] / 2, 500 - enemy_data[enemy_type]["height"] / 2); // don't go too close to the bottom

                    // add enemy
                    enemies.push_back(Enemy(
                        enemy_type,
                        enemy_textures[enemy_type],
                        rand_x_spawn(rand_generator), rand_y_spawn(rand_generator),
                        enemy_data[enemy_type]["attack_rotation_vel"],
                        enemy_data[enemy_type]["attack_xvel_mult"],
                        enemy_data[enemy_type]["attack_yvel_mult"],
                        enemy_data[enemy_type]["width"],
                        enemy_data[enemy_type]["height"],
                        enemy_data[enemy_type]["speed"],
                        enemy_data[enemy_type]["frame_delay_ticks"],
                        enemy_data[enemy_type]["health"],
                        enemy_data[enemy_type]["shot_cooldown"],
                        enemy_data[enemy_type]["shot_num"]
                    ));

                    spawned_already += 1;

                }

                // check if next wave
                if (spawned_already == real_max && int(enemies.size()) == 0){
                    next_wave_ticks = 240;
                    max_enemies += 1;
                    spawned_already = 0;
                    wave += 1;
                    wave_start = true;

                    // play wave transition sound
                    playChunkWav("audio/next-wave.wav");
                }
            }

            // rocket thruster particles
            for (int i = 0; i != 3 * (player.health > 0); i++){
                int new_angle = thruster_particle_angle(rand_generator);
                int new_size = thruster_particle_size(rand_generator);
                float new_vel_mult = thruster_particle_mult(rand_generator) * .01f;
                particles.push_back(Particle(particle_textures[thruster_particle_texture(rand_generator)], player.x, 10 + player.rect.y + player.rect.h / 2, -sin(radians(new_angle)) * new_vel_mult, -cos(radians(new_angle)) * new_vel_mult, 255, new_size, new_size));
            }

            // render
            // clear render buffer
            SDL_RenderClear(renderer);

            // scrolling backgrounds
            background_1.y += 4;
            background_2.y += 4;

            // scrolling system
            if (background_1.y >= 700){
                background_1.y = -700;
            } else if (background_2.y >= 700){
                background_2.y = -700;
            }

            // cheap solution to avoiding gaps between two backgrounds by only moving x of the backgrounds.
            SDL_Rect new_background_1 = {background_1.x, background_1.y - camera.y, background_1.w, background_1.h};
            SDL_Rect new_background_2 = {background_2.x, background_2.y - camera.y, background_2.w, background_2.h};

            camera.renderCopy(renderer, background_texture, NULL, &new_background_1);
            camera.renderCopy(renderer, background_texture, NULL, &new_background_2);

            // missiles
            vector<Missile> new_missiles;
            for (Missile &missile: missiles){
                bool still_alive = true;
                
                // check for collision
                for (Enemy &enemy: enemies){
                    if (SDL_HasIntersection(&missile.rect, &enemy.rect)){
                        still_alive = false;

                        // explosion
                        explosions.push_back(Explosion(
                            missile.x,
                            missile.y,
                            explosion_texture
                        ));

                        // remove health
                        enemy.health -= player.damage;

                        // shake camera
                        camera.shake(5, 2);

                        break;
                    }
                }
                
                // update
                if (missile.update(renderer) && still_alive){
                    still_alive = true;
                } else {
                    still_alive = false;
                }

                // add if it's alive
                if (still_alive){
                    new_missiles.push_back(missile);
                }

            }
            missiles = new_missiles;

            // particles
            vector<Particle> new_particles;
            for (Particle &particle: particles){
                if (particle.update()){
                    new_particles.push_back(particle);
                    
                    // glow
                    glow_rect = {int(particle.x), int(particle.y), int(particle.width * 2), int(particle.width * 2)};
                    centerRect(glow_rect);
                    pair<SDL_Rect, int> new_loc = {glow_rect, particle.alpha};
                    glow_locs.push_back(new_loc);
                }
            }
            particles = new_particles;

            // enemies
            vector<Enemy> new_enemies;
            for (Enemy &enemy: enemies){
                bool still_alive = false;

                if (!enemy.shot_cooldown_curr){
                    enemy.shoot(enemy_attacks[enemy.type], enemy_attacks_vec, enemy_data[enemy.type]["attack_width"], enemy_data[enemy.type]["attack_height"], enemy_data[enemy.type]["attack_damage"], enemy_data[enemy.type]["attack_next_frame_ticks"]);
                    enemy.shot_cooldown_curr = enemy.shot_cooldown;
                }
                
                if (enemy.update(rand_generator)){
                    still_alive = true;
                }

                // add if still alive
                if (still_alive){
                    new_enemies.push_back(enemy);
                } else {
                    // add bigger explosion
                    explosions.push_back(Explosion(enemy.x, enemy.y, explosion_texture, enemy.width, enemy.height, 10));

                    // more shake if enemy is boss
                    if (find(choose_enemies.begin(), choose_enemies.end(), enemy.type) != choose_enemies.end()){
                        camera.shake(80, 5, true);
                    } else {
                        camera.shake(270, 8, true);
                    }

                }

            // some particles
            if (game_ticks % 2 == 0 && has_particles[enemy.type]){
                int new_angle = death_particle_angle(rand_generator);
                int new_size = death_particle_size(rand_generator);
                float new_vel_mult = death_particle_mult(rand_generator) * .01f;
                particles.push_back(Particle(particle_textures[3], enemy.x, enemy.y, sin(radians(new_angle)) * new_vel_mult, cos(radians(new_angle)) * new_vel_mult, 255, new_size, new_size, 0.1f, 2));
            }

            }
            enemies = new_enemies;

            // enemy attacks
            vector<EnemyAttack> new_attacks;
            for (EnemyAttack &attack: enemy_attacks_vec){

                // fake glow around attack
                glow_rect = {int(attack.x), int(attack.y), int(attack.glow_radius), int(attack.glow_radius)};
                centerRect(glow_rect);
                pair<SDL_Rect, int> new_loc = {glow_rect, 255};
                glow_locs.push_back(new_loc);

                bool still_alive = false;
                int res = attack.update(player);
                if (!(res == attack.OUT_OF_SCREEN || res == attack.HIT_PLAYER)){
                    still_alive = true;
                } else {
                    if (res == attack.HIT_PLAYER){
                        player.health -= attack.damage;
                    }
                }

                // add if still alive
                if (still_alive){
                    new_attacks.push_back(attack);
                } else {

                    // explode if player still alive
                    if (player.health > 0){
                        explosions.push_back(Explosion(
                            attack.x,
                            attack.y,
                            explosion_texture
                        ));
                    }

                }
            }
            enemy_attacks_vec = new_attacks;

            // layers: particles | glows | enemy attacks | enemies

            // show particles
            for (Particle& particle: particles){
                SDL_SetTextureAlphaMod(particle.texture, particle.alpha);
                camera.renderCopy(renderer, particle.texture, NULL, &particle.rect);
            }

            // show glows
            for (auto [rect, alpha]: glow_locs){
                SDL_SetTextureAlphaMod(glow_vignette, alpha);
                camera.renderCopy(renderer, glow_vignette, NULL, &rect);
            }

            // clear glow locs
            glow_locs.clear();

            // show enemy attacks
            for (EnemyAttack& enemy_attack: enemy_attacks_vec){
                camera.renderCopy(renderer, enemy_attack.textures[enemy_attack.curr_frame], NULL, &enemy_attack.rect);
            }

            // show enemies
            for (Enemy &enemy: enemies){
                SDL_SetTextureAlphaMod(enemy.textures[enemy.curr_frame], enemy.alpha);
                camera.renderCopy(renderer, enemy.textures[enemy.curr_frame], NULL, &enemy.rect);
            }

            // explosions
            vector<Explosion> new_explosions;
            for (Explosion &explosion: explosions){
                if (explosion.update(renderer)){
                    new_explosions.push_back(explosion);
                }
            }
            explosions = new_explosions;

            // healthbar
            health_bar.update(renderer, player.health, player.max_health);
            ostringstream healthbar_text;
            healthbar_text << player.health * (player.health > 0) << "hp";
            font_renderer.renderTextCentered(renderer, healthbar_text.str(), 300, 10, 25, 0, 0, 0);

            // wave text

            if (wave_start){
                
                // position
                wave_text_x += max(abs(int((300 - wave_text_x) / 20)), 1);

                // background dim
                SDL_SetTextureAlphaMod(dim_texture, max(150 - (150 * abs(int(300 - wave_text_x)) / 350), 0));
                camera.renderCopy(renderer, dim_texture, NULL, &dim_rect);

                // show text
                ostringstream wave_text;
                wave_text << "wave " << wave;
                font_renderer.renderTextCentered(renderer, wave_text.str(), wave_text_x, 350, 90, 255, 255, 255);

                if (wave_text_x >= 700){
                    wave_start = false;
                }

            } else {
                wave_text_x = -300;
            }
            

            // check if player dead
            if (player.health <= 0){

                // player explosion animation
                if (!player_exploded){
                    player_death_explosion = Explosion(player.x, player.y, explosion_texture, player.display_rect.w * 3, player.display_rect.h * 3, 20);
                    player_exploded = true;
                }
                if (!player_death_explosion.update(renderer)){
                    player_death_explosion = Explosion(player.x, player.y, explosion_texture, player.display_rect.w * 3, player.display_rect.h * 3, 20);
                }

                death_transition_alpha += 1;

                SDL_SetTextureAlphaMod(death_transition_background, death_transition_alpha);
                camera.renderCopy(renderer, death_transition_background, NULL, &death_transition_rect);

                // transition finished?
                if (death_transition_alpha == 255){

                    // clear game
                    explosions.clear();
                    enemy_attacks_vec.clear();
                    enemies.clear();
                    particles.clear();
                    missiles.clear();

                    // wave and spawning reset
                    wave = 1;
                    max_enemies = 3;
                    next_spawn_ticks = 0;
                    spawned_already = 0;
                    next_wave_ticks = 0;
                    wave_start = true;

                    // clear death stuff
                    death_transition_alpha = 0;
                    SDL_SetTextureAlphaMod(death_transition_background, 255);

                    // reset player
                    player = Player(renderer);
                    player_exploded = false;

                    // game state
                    game_state = DEATH_SCREEN;

                    // end music
                    Mix_FadeOutMusic(300);
                }

            } else {

                // player update
                player.update(renderer);

            }

        }

        // show render
        SDL_RenderPresent(renderer);
        camera.update(rand_generator);

        frametime = SDL_GetTicks() - framestart;
        if (FRAME_DELAY > frametime){
            SDL_Delay(FRAME_DELAY - frametime);
        } else {
            cout << "lagging..." << enemies.size() << " | " << particles.size() + enemy_attacks_vec.size() << "\n";
        }
        framestart = SDL_GetTicks();

        int new_screen_width;
        int new_screen_height;
        SDL_GetWindowSize(window, &new_screen_width, &new_screen_height);
        camera.scaleBy(new_screen_width, new_screen_height, 600, 700);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
    
}

// cd "Desktop/C++/Overwhelming"
// g++ main.cpp -I"SDL2/include" -L"SDL2/lib" -L"SDL2_image/lib" -L"SDL2_mixer/lib" -Wall -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -o main