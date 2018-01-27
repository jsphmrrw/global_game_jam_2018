#define UI_SRC_ID 0

#include "state.h"

#include "player_controller.cpp"
#include "camera.cpp"
#include "map.cpp"
#include "light.cpp"

struct GameData {
    Map map;
    LightState lighting;
    Camera camera;
    PlayerController controller;
    i16 target_entity_id;
    FBO map_render;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    g->lighting = init_light_state();
    g->camera = init_camera(0, 0);
    g->target_entity_id = -1;
    g->map_render = init_fbo(CRT_W, CRT_H);

    request_texture(TEX_SPRITES);
    request_shader(SHADER_CRT);

    return s;
}

void init_game_heavy() {
    GameData *g = (GameData *)state.memory;
    g->map = generate_map();
    add_entity(&g->map, init_rocket_drone(-1, 128, 0));
    g->target_entity_id = 0;
}

void clean_up_game(State *s) {
    GameData *g = (GameData *)s->memory;

    clean_up_fbo(&g->map_render);

    unrequest_shader(SHADER_CRT);
    unrequest_texture(TEX_SPRITES);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_game() {
    GameData *g = (GameData *)state.memory;

    begin_player_controller_update(&g->controller);
    update_player_controller_keyboard(&g->controller);

    if(g->target_entity_id >= 0) {
        Entity *e = g->map.entities+g->target_entity_id;

        if(g->controller.controls[CONTROL_MOVE_UP]) {
            e->y_vel -= 0.1;
        }
        if(g->controller.controls[CONTROL_MOVE_LEFT]) {
            e->x_vel -= 0.1;
        }
        if(g->controller.controls[CONTROL_MOVE_DOWN]) {
            e->y_vel += 0.1;
        }
        if(g->controller.controls[CONTROL_MOVE_RIGHT]) {
            e->x_vel += 0.1;
        }
        if(g->controller.controls[CONTROL_SUICIDE]) {
            r32 e_center_x = e->x + e->w/2,
                e_center_y = e->y + e->h/2;

            for(i16 i = (e_center_x - 96)/8; i < (e_center_x + 96)/8; i++) {
                for(i16 j = (e_center_y - 96)/8; j < (e_center_y + 96)/8; j++) {
                    if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
                       distance2_32(i*8+4, j*8 + 4, e_center_x, e_center_y) <= 96*96) {
                        g->map.tiles[i][j] = 0;
                    }
                }
            }
            delete_entity(&g->map, g->target_entity_id);
            g->target_entity_id = -1;
        }

        g->camera.target_x = g->map.entities[0].x + g->map.entities[0].w/2 - g->map_render.w/2;
        g->camera.target_y = g->map.entities[0].y + g->map.entities[0].h/2 - g->map_render.h/2;
    }

    //do_light(&g->lighting, mouse_x + g->camera.x, mouse_y + g->camera.y, 512, 1.5, 1, 0.7, 0.4);

    update_camera(&g->camera);

    update_map(&g->map);

    //update_light_state(&g->lighting, &g->camera, g->map_render.w, g->map_render.h);
    clear_fbo(&g->map_render);
    bind_fbo(&g->map_render);
    {
        draw_map(&g->map, &g->camera, CRT_W, CRT_H);
    }

    bind_fbo(&crt_render);
    {
        draw_scaled_fbo(&g->map_render, 0, 0, 0, CRT_W, CRT_H);
    }
    bind_fbo(NULL);
}

#undef UI_SRC_ID
