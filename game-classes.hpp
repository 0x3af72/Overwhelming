#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_image.h"

#include "functions.hpp"
#include "audio.hpp"

using namespace std;

#pragma once

class Camera{
    public:
        // camera variables
        int x = 0;
        int y = 0;
        float wmult = 1;
        float hmult = 1;

        // shake variables
        int shake_time = 0;
        uniform_int_distribution<int> rand_coord;
        
        // methods
        void update(default_random_engine& rand_generator);
        void shake(int amount, int magnitude, bool interrupt);

        void scaleBy(float new_width, float new_height, float width, float height);

        void renderCopy(
            SDL_Renderer *renderer, 
            SDL_Texture* texture, 
            const SDL_Rect* source, 
            const SDL_Rect* dest
        );

        void renderCopyEx(
            SDL_Renderer* renderer,
            SDL_Texture* texture,
            const SDL_Rect* source,
            const SDL_Rect* dest,
            const double angle,
            const SDL_Point *center,
            const SDL_RendererFlip flip
        );
};

void Camera::update(default_random_engine& rand_generator){
    if (shake_time > 0){
        x = rand_coord(rand_generator);
        y = rand_coord(rand_generator);
        shake_time -= 1;
    }

    // reset camera position at end of shake
    if (!shake_time){
        x = 0;
        y = 0;
    }

}

void Camera::shake(int amount = 50, int magnitude = 4, bool interrupt = false){
    // interrupt shake?
    if (!(!interrupt && shake_time)){
        shake_time = amount;
        rand_coord = uniform_int_distribution<int>(-magnitude, magnitude);
    }
}

void Camera::scaleBy(float new_width, float new_height, float width, float height){
    wmult = new_width / width;
    hmult = new_height / height;
}

void Camera::renderCopy(
        SDL_Renderer *renderer, 
        SDL_Texture* texture, 
        const SDL_Rect* source, 
        const SDL_Rect* dest
    ){
    SDL_Rect new_dest = {int((dest -> x + x) * wmult), int((dest -> y + y) * hmult), int(dest -> w * wmult), int(dest -> h * hmult)};
    SDL_RenderCopy(renderer, texture, source, &new_dest);
}

void Camera::renderCopyEx(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect* source,
    const SDL_Rect* dest,
    const double angle,
    const SDL_Point *center,
    const SDL_RendererFlip flip
    ){
        SDL_Rect new_dest = {int((dest -> x + x) * wmult), int((dest -> y + y) * hmult), int(dest -> w * wmult), int(dest -> h * hmult)};
        SDL_RenderCopyEx(renderer, texture, source, &new_dest, angle, center, flip);
    }

Camera camera;

class Particle{
    public:
        // game variables
        float x;
        float y;
        float x_vel;
        float y_vel;
        float width;
        float height;
        int alpha;
        int lose_alpha;
        float gravity;

        // sdl variables
        SDL_Texture *texture;
        SDL_Rect rect;

        // methods
        Particle(SDL_Texture *texture_, float x_, float y_, float x_vel_, float y_vel_, int alpha_, float width_, float height_, float gravity_, int lose_alpha_);
        bool update();
};

Particle::Particle(SDL_Texture *texture_, float x_, float y_, float x_vel_, float y_vel_, int alpha_, float width_, float height_, float gravity_ = 0, int lose_alpha_ = 6){
    // texture
    texture = texture_;

    // positions
    x = x_;
    y = y_;
    x_vel = x_vel_;
    y_vel = y_vel_;

    // alpha
    alpha = alpha_;
    lose_alpha = lose_alpha_;

    // gravity
    gravity = gravity_;

    // dimensions
    width = width_;
    height = height_;
}

bool Particle::update(){
    // calculations
    x += x_vel;
    y += y_vel;
    y_vel += gravity;
    rect = {int(x), int(y), int(width), int(height)};
    centerRect(rect);
    alpha -= lose_alpha;
    if (alpha < 1){
        alpha = 1;
    }
    
    return (alpha != 1);
}

class Missile{
    public:
        // game variables
        int x;
        int y;
        int speed;

        // sdl variables
        SDL_Texture *texture;
        SDL_Rect rect;

        // methods
        Missile(SDL_Texture *texture_, int x_, int y_, int speed_);
        bool update(SDL_Renderer *renderer);
};

Missile::Missile(SDL_Texture *texture_, int x_, int y_, int speed_){
    // texture
    texture = texture_;

    // game variables
    x = x_;
    y = y_;
    speed = speed_;

    rect = {x, y, 40, 57};
}

bool Missile::update(SDL_Renderer *renderer){
    // rect
    rect = {x, y, 40, 57};
    centerRect(rect);

    // movement
    y -= speed;

    // show texture
    camera.renderCopy(renderer, texture, NULL, &rect);

    return (rect.y + rect.h > 0);
}

class Player{
    public:
        // game variables
        int x = 300;
        int y = 600;
        int speed = 3;
        int damage = 10;
        int health = 1000;
        int max_health = 1000;

        // heart animation
        SDL_Rect heart_rect = {0, 0, 25, 25};
        float heart_rad = 25;
        int heart_shrink = -1;

        // sdl variables
        SDL_Texture *texture;
        SDL_Texture *hitbox_texture;
        SDL_Rect display_rect = {0, 0, 60, 49};
        SDL_Rect rect = {0, 0, 20, 20};

        // methods
        Player(SDL_Renderer *renderer);
        void update(SDL_Renderer *renderer);
};

Player::Player(SDL_Renderer *renderer){
    // texture
    texture = loadTexture(renderer, "assets/player/player.png");

    // hitbox texture
    hitbox_texture = loadTexture(renderer, "assets/heart/heart.png");
}

void Player::update(SDL_Renderer *renderer){
    // rect
    rect.x = x;
    rect.y = y + 10;
    display_rect.x = x;
    display_rect.y = y;
    heart_rect.x = x;
    heart_rect.y = y + 10;
    centerRect(rect);
    centerRect(display_rect);
    centerRect(heart_rect);

    // heart animation
    heart_rad += 0.3f * heart_shrink;
    heart_shrink *= -1 + 2 * !(heart_rad <= 0 || heart_rad >= 25);
    heart_rect.w = heart_rad;
    heart_rect.h = heart_rad;

    // show texture
    camera.renderCopy(renderer, texture, NULL, &display_rect);

    // show hitbox texture
    camera.renderCopy(renderer, hitbox_texture, NULL, &heart_rect);
}

class EnemyAttack{
    public:
        // game variables
        int alive_ticks = 0;
        float x;
        float y;
        float xvel;
        float yvel;
        int width;
        int height;
        int damage;

        // glow
        float glow_radius;
        float glow_max;
        float glow_min;
        int glow_direction = -1;

        // const
        int HIT_PLAYER = 1;
        int OUT_OF_SCREEN = 2;

        // sdl variables
        int curr_frame = 0;
        int next_frame_ticks = 0;
        vector<SDL_Texture*> textures;
        SDL_Rect rect;
        
        // methods
        EnemyAttack(float x_, float y_, int width_, int height_, vector<SDL_Texture*> textures_, int damage_, float xvel_, float yvel_, int next_frame_ticks_);
        int update(Player &player);
};

EnemyAttack::EnemyAttack(float x_, float y_, int width_, int height_, vector<SDL_Texture*> textures_, int damage_, float xvel_, float yvel_, int next_frame_ticks_ = 1){
    // game variables
    x = x_;
    y = y_;
    width = width_;
    height = height_;
    damage = damage_;
    xvel = xvel_;
    yvel = yvel_;

    // glow radius
    glow_max = width * 2.5f;
    glow_min = width * 2;
    glow_radius = glow_max;

    // texture
    next_frame_ticks = next_frame_ticks_;
    textures = textures_;

}

int EnemyAttack::update(Player &player){
    alive_ticks += 1;

    // rect
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    centerRect(rect);

    // calculations
    x += xvel;
    y += yvel;

    // frame
    if (alive_ticks % next_frame_ticks == 0){
        curr_frame = (curr_frame + 1) % textures.size();
    }

    // glow
    glow_radius += 0.2f * glow_direction;
    glow_direction *= -1 + 2 * !(glow_radius <= glow_min || glow_radius >= glow_max);

    // intersection
    if (SDL_HasIntersection(&player.rect, &rect)){
        return HIT_PLAYER;
    }

    return !(x > -100 && x < 700 && y > -100 && y < 800) * OUT_OF_SCREEN;
}

class Enemy{
    public:

        // game variables
        string type;
        int health;
        int alive_ticks;
        float x;
        float y;
        float width;
        float height;
        float speed;

        // attacks
        float attack_rotation = 0;
        float attack_rotation_vel;
        float attack_xvel;
        float attack_yvel;
        float attack_xvel_mult;
        float attack_yvel_mult;
        int shot_num;
        int shot_cooldown_curr;
        int shot_cooldown;

        // transition
        int alpha = 0;
        int transition_speed;

        // movement
        int next_move_ticks = 0;
        int dist_to_target = 0;
        int x_target;
        int y_target;
        float x_vel;
        float y_vel;

        // sdl variables
        int curr_frame = 0;
        int frame_delay_ticks;
        vector<SDL_Texture*> textures;
        SDL_Rect rect;

        // methods
        Enemy(
            string type_, 
            vector<SDL_Texture*> textures_, 
            float x_, 
            float y_, 
            float attack_rotation_vel_, 
            float attack_xvel_mult_, 
            float attack_yvel_mult_, 
            float width_, 
            float height_, 
            float speed_, 
            int frame_delay_ticks_, 
            int health_, 
            int shot_cooldown_,
            int shot_num_,
            int transition_speed_
        );
        bool update(default_random_engine rand_generator);
        void shoot(vector<SDL_Texture*> attack_textures, vector<EnemyAttack> &attacks, int attack_width, int attack_height, int attack_damage, int attack_next_frame_ticks);
};

Enemy::Enemy(
            string type_, 
            vector<SDL_Texture*> textures_, 
            float x_, 
            float y_, 
            float attack_rotation_vel_, 
            float attack_xvel_mult_, 
            float attack_yvel_mult_, 
            float width_, 
            float height_, 
            float speed_, 
            int frame_delay_ticks_, 
            int health_, 
            int shot_cooldown_, 
            int shot_num_,
            int transition_speed_ = 5
        ){
    // texture
    textures = textures_;
    frame_delay_ticks = frame_delay_ticks_;

    // game variables
    type = type_;
    x = x_;
    y = y_;
    width = width_;
    height = height_;
    speed = speed_;
    health = health_;

    // shooting
    attack_rotation_vel = attack_rotation_vel_;
    attack_xvel_mult = attack_xvel_mult_;
    attack_yvel_mult = attack_yvel_mult_;
    shot_cooldown = shot_cooldown_;
    shot_cooldown_curr = shot_cooldown;
    shot_num = shot_num_;

    // rect
    rect = {int(x), int(y), int(width), int(height)};

    // transition
    transition_speed = transition_speed_;
}

void Enemy::shoot(vector<SDL_Texture*> attack_textures, vector<EnemyAttack> &attacks, int attack_width, int attack_height, int attack_damage, int attack_next_frame_ticks){
    for (int i = 0; i != shot_num; i++){
        attack_rotation += attack_rotation_vel;
        attack_xvel = sin(radians(attack_rotation)) * attack_xvel_mult;
        attack_yvel = cos(radians(attack_rotation)) * attack_yvel_mult;
        attacks.push_back(EnemyAttack(x, rect.y + rect.h, attack_width, attack_height, attack_textures, attack_damage, attack_xvel, attack_yvel, attack_next_frame_ticks));
    }
}

bool Enemy::update(default_random_engine rand_generator){
    alive_ticks += 1;

    // rect
    rect = {int(x), int(y), int(width), int(height)};
    centerRect(rect);

    // shooting
    shot_cooldown_curr -= 1;

    // random movement
    if (!next_move_ticks){

        // random target
        uniform_int_distribution<int> x_coord_dist(0 + width / 2, 600 - width / 2);
        uniform_int_distribution<int> y_coord_dist(0 + height / 2, 500 - height / 2); // don't go too close to the bottom
        x_target = x_coord_dist(rand_generator);
        y_target = y_coord_dist(rand_generator);

        // get angles
        int targ_angle = angle(x, y, x_target, y_target);

        // get velocities
        x_vel = sin(targ_angle * M_PI / 180);
        y_vel = cos(targ_angle * M_PI / 180);

        // distance
        dist_to_target = hypot(x_target - x, y_target - y);

        // set next move tick
        next_move_ticks = 120;
    }

    // move
    if (dist_to_target){
        dist_to_target -= 1;
        x += x_vel * speed * (alpha == 255); // move after transition
        y += y_vel * speed * (alpha == 255);
    } else {
        next_move_ticks -= 1;
    }

    // texture
    if (alive_ticks % frame_delay_ticks == 0){
        curr_frame = (curr_frame + 1) % textures.size();
    }

    // alpha
    if (alpha != 255){
        alpha += min(3, 255 - alpha);
    }

    return health > 0;
}

class Explosion{
    public:
        // game variables
        int alive_ticks = 0;

        // sdl variables
        int curr_frame = 0;
        int next_frame_ticks;
        vector<SDL_Texture*> textures;
        SDL_Rect rect;

        // methods
        Explosion(int x_, int y_, vector<SDL_Texture*> textures_, int width_, int height_, int next_frame_ticks_);
        bool update(SDL_Renderer *renderer);
};

Explosion::Explosion(int x_, int y_, vector<SDL_Texture*> textures_, int width_ = 48, int height_ = 48, int next_frame_ticks_ = 5){
    // rect variables
    rect.x = x_;
    rect.y = y_;
    rect.w = width_;
    rect.h = height_;
    centerRect(rect);

    // textures
    next_frame_ticks = next_frame_ticks_;
    textures = textures_;
}

bool Explosion::update(SDL_Renderer *renderer){
    // update frames
    alive_ticks += 1;
    if (alive_ticks % next_frame_ticks == 0){
        curr_frame += 1;
        if (curr_frame == 5){
            return false;
        }
    }

    // show
    camera.renderCopy(renderer, textures[curr_frame], NULL, &rect);

    return true;
}

class FontRenderer{
    public:
        // unordered map for storing key value pairs.
        unordered_map<string, SDL_Texture*> character_map;
        unordered_map<string, vector<float>> character_size_ratio;

        // methods
        FontRenderer(SDL_Renderer *renderer, string path, string include);
        void renderText(SDL_Renderer *renderer, string text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b);
        int renderTextCentered(SDL_Renderer *renderer, string text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b);
};

FontRenderer::FontRenderer(SDL_Renderer *renderer, string path, string include = "abcdefghijklmnopqrstuvwxyz0123456789"){
    for (auto c: include){
        ostringstream string_stream;
        string_stream << c;
        string new_path = path + c + ".png";
        SDL_Texture* font_texture = loadTexture(renderer, new_path.c_str());

        string string_stream_str = string_stream.str();
        character_map[string_stream_str] = font_texture;

        // size ratio
        int char_w, char_h;
        SDL_QueryTexture(font_texture, NULL, NULL, &char_w, &char_h);
        character_size_ratio[string_stream_str] = {char_w * 0.01f, char_h * 0.01f};

    }
}

void FontRenderer::renderText(SDL_Renderer *renderer, string text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b){
    SDL_Rect char_rect = {x, y, 0, 0};
    for (auto c: text){

        ostringstream string_stream;
        string_stream << c;
        string string_stream_str = string_stream.str();

        if ((character_map.count(string_stream_str) != 0 && character_size_ratio.count(string_stream_str) != 0) || string_stream_str == " "){
            
            if (string_stream_str == " "){
                char_rect.x += 20 * size / 60;

            } else {
                
                char_rect.w = character_size_ratio[string_stream_str][0] * size;
                char_rect.h = character_size_ratio[string_stream_str][1] * size;
                
                // set color
                SDL_SetTextureColorMod(character_map[string_stream_str], r, g, b);

                // render
                camera.renderCopy(renderer, character_map[string_stream_str], NULL, &char_rect);
                char_rect.x += char_rect.w;
            }

        }
    }
}

int FontRenderer::renderTextCentered(SDL_Renderer *renderer, string text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b){

    int centered_x = x;
    int total_width = 0;
    float total_heights = 0;

    for (auto c: text){

        ostringstream string_stream;
        string_stream << c;
        string string_stream_str = string_stream.str();
        
        if ((character_map.count(string_stream_str) != 0 && character_size_ratio.count(string_stream_str) != 0) || string_stream_str == " "){

            if (string_stream_str == " "){
                centered_x -= (20 * size / 60) / 2;

            } else {
                centered_x -= character_size_ratio[string_stream_str][0] * size * 0.5f;
                total_width += character_size_ratio[string_stream_str][0] * size;
                total_heights += character_size_ratio[string_stream_str][1] * size * 0.5f;
            }
        }
    }
    
    int centered_y = y - total_heights / text.length();

    renderText(renderer, text, centered_x, centered_y, size, r, g, b);

    return total_width;
}

class HealthBar{
    public:
        // healthbar variables
        int bar_width;
        int bar_height;

        // sdl variables
        SDL_Texture *texture;
        SDL_Rect rect;

        // methods
        HealthBar(SDL_Renderer *renderer, string texture_path, int bar_width_, int bar_height_);
        void update(SDL_Renderer *renderer, int health, int max_health);
};

HealthBar::HealthBar(SDL_Renderer *renderer, string texture_path, int bar_width_, int bar_height_){
    // texture
    texture = loadTexture(renderer, texture_path.c_str());

    // rect
    bar_width = bar_width_;
    bar_height = bar_height_;
    rect.h = bar_height;
}

void HealthBar::update(SDL_Renderer *renderer, int health, int max_health){
    // bar width multiplied by percentage of health
    rect.w = (float(health) / float(max_health)) * bar_width;
    rect.h = bar_height;
    rect.x = 300;
    rect.y = 0;
    centerRect(rect);

    camera.renderCopy(renderer, texture, NULL, &rect);
}