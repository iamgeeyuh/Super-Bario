/**
* Author: Jia Huang
* Assignment: Platformer
* Date due: 2023-11-23, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_LEFT_EDGE 10.0f
#define LEVEL1_RIGHT_EDGE 49.0f
#define LEVEL1_BOTTOM_EDGE -24.5f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "Start.h"
#include "LevelA.h"
#include "LevelB.h"
#include "LevelC.h"

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.0f,
BG_BLUE    = 0.25098039215f,
BG_GREEN   = 0.50196078431f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH  = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

enum AppStatus { RUNNING, TERMINATED };

// ––––– GLOBAL VARIABLES ––––– //
Scene  *g_current_scene;
Start *g_start;
LevelA *g_levelA;
LevelB *g_levelB;
LevelC *g_levelC;

float x;
float y;

Scene   *g_levels[4];

SDL_Window* g_display_window;

GLint fontsheet_texture_id;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool g_is_colliding_bottom = false;

AppStatus g_app_status = RUNNING;

void swtich_to_scene(Scene *scene);
void initialise();
void process_input();
void update();
void render();
void shutdown();

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene *scene)
{
    int current_health = 3;
    if (g_current_scene) current_health = g_current_scene->get_state().player->get_health();
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
    g_current_scene->get_state().player->set_health(current_health);
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, Special Effects!",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-10.0f, 10.0f, -7.5f, 7.5f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    fontsheet_texture_id = Utility::load_texture("assets/fontsheet.png");
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    g_start = new Start();
    g_levelA = new LevelA();
    g_levelB = new LevelB();
    g_levelC = new LevelC();
    
    g_levels[0] = g_start;
    g_levels[1] = g_levelA;
    g_levels[2] = g_levelB;
    g_levels[3] = g_levelC;
    
    // Start at level A
    switch_to_scene(g_levels[0]);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_current_scene->get_state().player->set_movement(glm::vec3(0.0f));
    
    if (g_current_scene->get_state().player->get_win() || g_current_scene->get_state().player->get_lose()) return;
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
                // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_app_status = TERMINATED;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (g_current_scene->get_state().player->get_collided_bottom())
                        {
                            g_current_scene->get_state().player->jump();
                            Mix_PlayChannel(-1,  g_current_scene->get_state().jump_sfx, 0);
                        }
                        break;
                    case SDLK_RETURN:
                        if (g_current_scene == g_levels[0]) {
                            Mix_PlayChannel(-1,  g_current_scene->get_state().press_AB_sfx, 0);
                            g_current_scene->get_state().player->set_next_level(true);
                        }
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (g_current_scene != g_levels[0]) {
        if (key_state[SDL_SCANCODE_LEFT])        g_current_scene->get_state().player->move_left();
        else if (key_state[SDL_SCANCODE_RIGHT])  g_current_scene->get_state().player->move_right();
        else                                     g_current_scene->get_state().player->idle();
    }
    
    if (glm::length( g_current_scene->get_state().player->get_movement()) > 1.0f)
        g_current_scene->get_state().player->normalise_movement();
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        g_current_scene->update(FIXED_TIMESTEP);
        
        g_is_colliding_bottom = g_current_scene->get_state().player->get_collided_bottom();
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    // Prevent the camera from showing anything outside of the "edge" of the level
    g_view_matrix = glm::mat4(1.0f);
    
    if (g_current_scene == g_levels[0]) return;
    
    x = g_current_scene->get_state().player->get_position().x;
    y = g_current_scene->get_state().player->get_position().y;
    
    if (g_current_scene != g_levels[3]) {
        x <= LEVEL1_LEFT_EDGE ? x = -LEVEL1_LEFT_EDGE : x >= LEVEL1_RIGHT_EDGE ? x = -LEVEL1_RIGHT_EDGE : x *= -1;
    } else {
        x *= -1;
    }
    
    y <= LEVEL1_BOTTOM_EDGE ? y = -LEVEL1_BOTTOM_EDGE : y *= -1;
    
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(x, y, 0));
}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // ————— RENDERING THE SCENE (i.e. map, character, enemies...) ————— //
    g_current_scene->render(&g_shader_program);
    
    if (g_current_scene->get_state().player->get_win()) Utility::draw_text(&g_shader_program, fontsheet_texture_id, "You Win", 0.7f, 0.01f, glm::vec3(-x - 2.0f, -y , 0.0f));
    if (g_current_scene->get_state().player->get_lose()) Utility::draw_text(&g_shader_program, fontsheet_texture_id, "You Lose", 0.7f, 0.01f, glm::vec3(-x - 2.0f, -y, 0.0f));
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete g_start;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        
        if (g_current_scene->get_state().next_scene_id >= 0) switch_to_scene(g_levels[g_current_scene->get_state().next_scene_id]);
        
        render();
    }
    
    shutdown();
    return 0;
}
