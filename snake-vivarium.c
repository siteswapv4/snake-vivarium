#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "ArrayList.h"


#define LIF_SUCCESS 0
#define LIF_FAILURE 1

#define LIF_WINDOW_NAME "LIF"
#define LIF_WINDOW_WIDTH 960
#define LIF_WINDOW_HEIGHT 544

#define LIF_LOGICAL_WIDTH 4800
#define LIF_LOGICAL_HEIGHT 2720

#define LIF_FRUIT_DETECT 1000
#define LIF_FRUIT_EAT 50


typedef struct
{
    int len;
    int* sizes;
    double direction;
    double** points;
    double* distances;
    double lastDistance;
    bool rotation;
    SDL_Color* colors;
}LIF_Snake;

typedef struct
{
    int posx;
    int posy;
    int size;
}LIF_Fruit;


typedef struct
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    ArrayList* snakes;
    ArrayList* fruits;
}LIF_Environment;


void LIF_DestroySnake(void* data)
{
    LIF_Snake* snake = data;

    for (int i = 0; i < snake->len; i++)
    {
        SDL_free(snake->points[i]);
    }

    SDL_free(snake->sizes);
    SDL_free(snake->points);
    SDL_free(snake->distances);
    SDL_free(snake->colors);
    SDL_free(snake);
}

LIF_Snake* LIF_CreateSnake(int posx, int posy, int len, int startSize, int endSize, SDL_Color c1, SDL_Color c2, SDL_Color c3)
{
    LIF_Snake* snake = (LIF_Snake*)SDL_calloc(1, sizeof(LIF_Snake));

    snake->len = len;
    snake->sizes = (int*)SDL_malloc(snake->len * sizeof(int));
    snake->points = (double**)SDL_malloc(snake->len * sizeof(double*));
    snake->distances = (double*)SDL_malloc(snake->len * sizeof(double));
    snake->colors = (SDL_Color*)SDL_malloc(snake->len * sizeof(SDL_Color));

    for (int i = 0; i < snake->len; i++)
    {
        if (i == 0)
        {
            snake->colors[i] = c1;
        }
        else if (i > snake->len - 10)
        {
            snake->colors[i] = c3;
        }
        else
        {
            snake->colors[i].r = (SDL_sin(i) + 1) / 2.0 * c2.r;
            snake->colors[i].g = (SDL_sin(i) + 1) / 2.0 * c2.g;
            snake->colors[i].b = (SDL_sin(i) + 1) / 2.0 * c2.b;
            snake->colors[i].a = c2.a;
        }

        snake->points[i] = SDL_malloc(2 * sizeof(double));
        snake->points[i][0] = posx + 50 * i;
        snake->points[i][1] = posy;

        snake->sizes[i] = startSize + (endSize - startSize) * ((double)i / snake->len);
        snake->distances[i] = snake->sizes[i] / 3;
    }

    return snake;
}

void LIF_DestroyFruit(void* data)
{
    LIF_Fruit* fruit = data;

    SDL_free(fruit);
}

LIF_Fruit* LIF_CreateFruit(int posx, int posy)
{
    LIF_Fruit* fruit = (LIF_Fruit*)SDL_calloc(1, sizeof(LIF_Fruit));

    fruit->posx = posx;
    fruit->posy = posy;
    fruit->size = 25;

    return fruit;
}

int LIF_MoveSnake(LIF_Snake* snake, ArrayList* fruits)
{
    double* pos = snake->points[0];

    double minFruitDistance = -1;
    int minFruitIndex = -1;

    double distance = SDL_sqrt(SDL_abs(SDL_pow(pos[0] - LIF_LOGICAL_WIDTH / 2, 2) + SDL_pow(pos[1] - LIF_LOGICAL_HEIGHT / 2, 2)));

    for (int i = fruits->len - 1; i >= 0; i--)
    {
        LIF_Fruit* fruit = fruits->data[i];

        double fruitDistance = SDL_sqrt(SDL_abs(SDL_pow(pos[0] - fruit->posx, 2) + SDL_pow(pos[1] - fruit->posy, 2)));

        if ((minFruitDistance == -1) || (fruitDistance < minFruitDistance))
        {
            minFruitDistance = fruitDistance;
            minFruitIndex = i;
        }
    }

    if ((minFruitIndex != -1) && (minFruitDistance < LIF_FRUIT_DETECT))
    {
        LIF_Fruit* fruit = fruits->data[minFruitIndex];

        if (minFruitDistance < LIF_FRUIT_EAT)
        {
            LIF_DestroyFruit(arrayListRemoveAt(fruits, minFruitIndex));
        }
        else if (minFruitDistance < LIF_FRUIT_DETECT)
        {
            snake->direction = SDL_atan2(fruit->posx - pos[0], fruit->posy - pos[1]);
        }

        pos[0] += 15 * SDL_sin(snake->direction);
        pos[1] += 15 * SDL_cos(snake->direction);
    }
    else
    {
        if ((SDL_rand(200)) == 0)
        {
            snake->rotation = !snake->rotation;
        }

        if (snake->lastDistance < distance)
        {
            if (snake->rotation)
            {
                snake->direction += 0.2 * (SDL_rand(100) / 100.0);
            }
            else
            {
                snake->direction -= 0.2 * (SDL_rand(100) / 100.0);
            }
        }
        else
        {
            if (snake->rotation)
            {
                snake->direction += 0.01;
            }
            else
            {
                snake->direction -= 0.01;
            }
        }

        pos[0] += 10 * SDL_sin(snake->direction);
        pos[1] += 10 * SDL_cos(snake->direction);
    }

    snake->lastDistance = distance;

    return LIF_SUCCESS;
}

int LIF_UpdateSnake(LIF_Snake* snake)
{
    double* p1;
    double* p2;
    double distance;
    double x;
    double y;
    double angle;

    for (int i = 1; i < snake->len; i++)
    {
        p1 = snake->points[i - 1];
        p2 = snake->points[i];
        distance = snake->distances[i - 1];

        x = p2[0] - p1[0];
        y = p2[1] - p1[1];

        angle = SDL_atan2(x, y);

        p2[0] = p1[0] + distance * SDL_sin(angle);
        p2[1] = p1[1] + distance * SDL_cos(angle);
    }

    return LIF_SUCCESS;
}

int LIF_RenderFruit(LIF_Environment* environment, LIF_Fruit* fruit)
{
    SDL_SetRenderDrawColor(environment->renderer, 0, 255, 0, 255);

    SDL_FRect rect = {fruit->posx - fruit->size / 2, fruit->posy - fruit->size / 2, fruit->size, fruit->size};

    SDL_RenderFillRect(environment->renderer, &rect);

    return LIF_SUCCESS;
}

int LIF_RenderSnake(LIF_Environment* environment, LIF_Snake* snake)
{
    for (int i = snake->len - 1; i >= 0; i--)
    {
        SDL_FRect rect = {SDL_floor(snake->points[i][0]) - snake->sizes[i] / 2, SDL_floor(snake->points[i][1]) - snake->sizes[i] / 2, snake->sizes[i], snake->sizes[i]};

        SDL_SetRenderDrawColor(environment->renderer, snake->colors[i].r, snake->colors[i].g, snake->colors[i].b, snake->colors[i].a);

        SDL_RenderFillRect(environment->renderer, &rect);
    }

    return LIF_SUCCESS;
}



SDL_AppResult SDL_AppInit(void** data, int argc, char* argv[])
{
    LIF_Environment* environment = SDL_calloc(1, sizeof(LIF_Environment));
	*data = environment;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        goto error;
	}

	if (!SDL_CreateWindowAndRenderer(LIF_WINDOW_NAME, LIF_WINDOW_WIDTH, LIF_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &environment->window, &environment->renderer))
	{
		goto error;
	}
	
	SDL_SetRenderVSync(environment->renderer, 1);
    SDL_SetRenderLogicalPresentation(environment->renderer, LIF_LOGICAL_WIDTH, LIF_LOGICAL_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_srand(0);

    environment->snakes = newArrayList();
    environment->fruits = newArrayList();

    arrayListAdd(environment->snakes, LIF_CreateSnake(1000, 1000, 200, 70, 10, (SDL_Color){200, 0, 0, 255}, (SDL_Color){255, 0, 0, 255}, (SDL_Color){255, 255, 255, 255}));
    arrayListAdd(environment->snakes, LIF_CreateSnake(4000, 2000, 100, 50, 5, (SDL_Color){0, 0, 255, 255}, (SDL_Color){0, 0, 255, 255}, (SDL_Color){0, 0, 0, 255}));
    arrayListAdd(environment->snakes, LIF_CreateSnake(3000, 1500, 300, 100, 20, (SDL_Color){155, 155, 155, 255}, (SDL_Color){155, 155, 155, 255}, (SDL_Color){0, 255, 0, 255}));

    return SDL_APP_CONTINUE;

error:
    SDL_Log("%s", SDL_GetError());

    return SDL_APP_FAILURE;
}


SDL_AppResult SDL_AppEvent(void* data, SDL_Event* event)
{
	LIF_Environment* environment = data;
	
	SDL_ConvertEventToRenderCoordinates(environment->renderer, event);
	
	switch (event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
            
        case SDL_EVENT_KEY_DOWN:
        	if (!event->key.repeat)
      		{
      			if (event->key.scancode == SDL_SCANCODE_F11)
        		{
		        	if ((SDL_GetWindowFlags(environment->window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN)
		        	{
		        		SDL_SetWindowFullscreen(environment->window, false);
		        	}
		        	else
		        	{
		        		SDL_SetWindowFullscreen(environment->window, true);
		        	}
            	}
        	}
        	break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            arrayListAdd(environment->fruits, LIF_CreateFruit(event->button.x, event->button.y));
            break;

        default:
            break;
    }
	
	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* data)
{
	LIF_Environment* environment = data;

    SDL_SetRenderDrawColor(environment->renderer, 65, 107, 223, 255);
    SDL_RenderClear(environment->renderer);

    for (int i = 0; i < environment->fruits->len; i++)
    {
        LIF_Fruit* fruit = environment->fruits->data[i];

        LIF_RenderFruit(environment, fruit);
    }

    for (int i = 0; i < environment->snakes->len; i++)
    {
        LIF_Snake* snake = environment->snakes->data[i];

        LIF_MoveSnake(snake, environment->fruits);

        LIF_UpdateSnake(snake);

        LIF_RenderSnake(environment, snake);
    }

    SDL_RenderPresent(environment->renderer);

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void* data, SDL_AppResult result)
{
	LIF_Environment* environment = data;
	
    if (environment->renderer)
    {
        SDL_DestroyRenderer(environment->renderer);
	}

    if (environment->window)
    {
        SDL_DestroyWindow(environment->window);
	}
	
	if (environment->snakes)
	{
		freeArrayList(environment->snakes, LIF_DestroySnake);
	}
	
	if (environment->fruits)
	{
		freeArrayList(environment->fruits, LIF_DestroyFruit);
	}

    SDL_free(environment);
}
