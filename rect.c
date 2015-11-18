#include "SDL2/SDL.h"
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT

typedef struct rect {
	unsigned int top;
	unsigned int left;
	unsigned int bottom;
	unsigned int right;
} rect;

void drawRect(SDL_Renderer* renderer, rect r) {
	
	SDL_RenderDrawLine(renderer, r.left, r.top, r.right, r.top);
	SDL_RenderDrawLine(renderer, r.right, r.top, r.right, r.bottom);
	SDL_RenderDrawLine(renderer, r.right, r.bottom, r.left, r.bottom);
	SDL_RenderDrawLine(renderer, r.left, r.bottom, r.left, r.top);
}

rect* splitRect(SDL_Renderer* renderer, rect rdest, rect rknife, int* out_count) {
	
	rect baserect;
	rect* outrect;
	int rect_count = 0;
	unsigned int i;
	
	baserect.top = rdest.top;
	baserect.left = rdest.left;
	baserect.bottom = rdest.bottom;
	baserect.right = rdest.right;

	if(rknife.left > baserect.left && rknife.left < baserect.right)
		rect_count++;
		
	if(rknife.right > baserect.left && rknife.right < baserect.right)
		rect_count++;
	
	if(rknife.top < baserect.bottom && rknife.top > baserect.top)
		rect_count++;		
		
	if(rknife.bottom < baserect.bottom && rknife.bottom > baserect.top)
		rect_count++;	
		
	outrect = (rect*)malloc(sizeof(rect)*rect_count);
	rect_count = 0;
	
	//Split by left edge
	if(rknife.left > baserect.left && rknife.left < baserect.right) {
		
		outrect[rect_count].top = baserect.top;
		outrect[rect_count].bottom = baserect.bottom;
		outrect[rect_count].right = rknife.left;
		outrect[rect_count].left = baserect.left;
		
		baserect.left = rknife.left;
		
		rect_count++;
	}

	//Split by top edge
	if(rknife.top < baserect.bottom && rknife.top > baserect.top) {
		
		outrect[rect_count].top = baserect.top;
		outrect[rect_count].bottom = rknife.top;
		outrect[rect_count].right = baserect.right;
		outrect[rect_count].left = baserect.left;
		
		baserect.top = rknife.top;
		
		rect_count++;
	}

	//Split by right edge
	if(rknife.right > baserect.left && rknife.right < baserect.right) {
		
		outrect[rect_count].top = baserect.top;
		outrect[rect_count].bottom = baserect.bottom;
		outrect[rect_count].right = baserect.right;
		outrect[rect_count].left = rknife.right;
		
		baserect.right = rknife.right;
		
		rect_count++;
	}

	//Split by right edge
	if(rknife.bottom > baserect.top && rknife.bottom < baserect.bottom) {
		
		outrect[rect_count].top = rknife.bottom;
		outrect[rect_count].bottom = baserect.bottom;
		outrect[rect_count].right = baserect.right;
		outrect[rect_count].left = baserect.left;
		
		baserect.bottom = rknife.bottom;
		
		rect_count++;
	}

	out_count[0] = rect_count;

	return outrect;	
}

void drawOccluded(SDL_Renderer* renderer, rect baserect, rect* splitrects, int rect_count) {
	
	int split_count = 0;
	int total_count = 0;
	rect* out_rects = (rect*)0;
	int i;
	
	for(i = 0; i < rect_count; i++) {
		
		rect* split_rects = splitRect(renderer, baserect, splitrects[0], &split_count);
		
		if(!split_count)
			continue;
		
		if(out_rect)
	}
	
	SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0x0, 0xFF);
	
	for(i = 0; i < rect_count; i++)
		drawRect(renderer, split_rects[i]);
		
	free(split_rects);
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
	rect baserect;
	rect* splitrects;
	int rect_count = 4;
	int done = 0;
	int i;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {

        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("LESTER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(window == NULL) {

        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if(renderer == NULL) {

        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

	splitrects = (rect*)malloc(sizeof(rect)*rect_count);
	
	splitrects[0].top = 10;
	splitrects[0].left = 10;
	splitrects[0].bottom = 210;
	splitrects[0].right = 310;
	
	splitrects[1].top = 120;
	splitrects[1].left = 450;
	splitrects[1].bottom = 400;
	splitrects[1].right = 510;
	
	splitrects[2].top = 250;
	splitrects[2].left = 200;
	splitrects[2].bottom = 470;
	splitrects[2].right = 400;
	
	splitrects[3].top = 200;
	splitrects[3].left = 300;
	splitrects[3].bottom = 300;
	splitrects[3].right = 400;

	SDL_SetRenderDrawColor(renderer, 0x20, 0x50, 0x60, 0xFF);
	SDL_RenderClear(renderer);
	baserect.top = 100;
	baserect.left = 100;
	baserect.bottom = 380;
	baserect.right = 540;
	//SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0x0, 0xFF);
	//drawRect(renderer, baserect);
	//SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	//drawRect(renderer, r2);
	drawOccluded(renderer, baserect, splitrects);
    //splitRect(renderer, r2, r);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	for(i = 0; i < rect_count; i++)
		drawRect(renderer, splitrects[i]);
	SDL_RenderPresent(renderer);

    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
