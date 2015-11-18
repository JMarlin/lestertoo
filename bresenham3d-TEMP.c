#include "SDL2/SDL.h"
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_DEPTH 5.0

//Convert a point scaled such that 1.0, 1.0 is at the upper right-hand
//corner of the screen and -1.0, -1.0 is at the bottom right to pixel coords
#define PI 3.141592653589793
#define TO_SCREEN_Y(y) ((int)((SCREEN_HEIGHT-(y*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_X(x) ((int)((SCREEN_WIDTH+(x*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_Z(z) ((unsigned char)((z) > SCREEN_DEPTH ? 0 : (255.0 - ((z*255.0)/SCREEN_DEPTH))))
#define DEG_TO_RAD(a) ((((float)a)*PI)/180.0)
#define MAX(a,b) (((a)>(b)?(a):(b)))
#define ABS(a) (((a)<0)?-(a):(a))
#define ZSGN(a) (((a)<0)?-1:(a)>0?1:0)

void point3d(SDL_Renderer* r, int x, int y, int z) {
	
	z = z > 255 ? 255 : z;	
	SDL_SetRenderDrawColor(r, z, z, z, 0xFF);
	SDL_RenderDrawPoint(r, x, y);
}

//Do a standard 2D bresenham, but draw it so that 'y' values are plotted on z-axis
//and plotted y-coordinate stays fixed at scanline
void scanline3d(SDL_Renderer *r, int scanline, int x0, int z0, int x1, int z1) {
	
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dz = abs(z1-z0), sz = z0<z1 ? 1 : -1;
	int err = (dx>dz ? dx : dz)/2, te;
	
	for(;;) {
		point3d(r, x0, scanline, z0);
		if(x0==x1 && z0 == z1) return;
		te = err;
		if(te > -dx) { err -= dz; x0 += sx; }
		if(te < dz) { err += dx; z0 += sz; }
	}
}

//This could be more elegantly done by creating a raster-interpolate 'class' which
//can keep track of its own bresenham values internally and run it's internals in steps 
//via a 'member function'
void render_triangle(triangle* tri, SDL_Renderer *rend) {
	
	int temp;
	int dx_x1, sx_x1, dy_x1, sy_x1,	err_x1,	te_x1;
	int dx_z1, sx_z1, dy_z1, sy_z1,	err_z1,	te_z1;
	int dx_x2, sx_x2, dy_x2, sy_x2,	err_x2,	te_x2;
	int dx_z2, sx_z2, dy_z2, sy_z2,	err_z2,	te_z2;
	int dx_x3, sx_x3, dy_x3, sy_x3,	err_x3,	te_x3;
	int dx_z3, sx_z3, dy_z3, sy_z3,	err_z3,	te_z3;
	int current_s;
	int cur_x1, cur_x2, cur_x3;
	int cur_z1, cur_z2, cur_z3;
	

    
    int i;
    point p[3];
    float zval[2];
    float vec_a[3];
    float vec_b[3];
    float cross[3];
    float mag;
    float normal_angle;
    float lighting_pct;
    float r, g, b;
    unsigned char f, s, t, e, done_1, done_2;
    
    //Calculate the surface normal
    //subtract 3 from 2 and 1, translating it to the origin
    vec_a[0] = tri->v[0].x - tri->v[2].x;
    vec_a[1] = tri->v[0].y - tri->v[2].y;
    vec_a[2] = tri->v[0].z - tri->v[2].z;
    vec_b[0] = tri->v[1].x - tri->v[2].x;
    vec_b[1] = tri->v[1].y - tri->v[2].y;
    vec_b[2] = tri->v[1].z - tri->v[2].z;
    
    //calculate the cross product using 1 as vector a and 2 as vector b
    cross[0] = vec_a[1]*vec_b[2] - vec_a[2]*vec_b[1];
    cross[1] = vec_a[2]*vec_b[0] - vec_a[0]*vec_b[2];
    cross[2] = vec_a[0]*vec_b[1] - vec_a[1]*vec_b[0]; 
    
    //normalize the result vector
    mag = sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
    cross[0] /= mag;
    cross[1] /= mag;
    cross[2] /= mag;
        
    //Calculate the normal's angle vs the camera view direction
    normal_angle = acos(-cross[2]);
    
    //If the normal is facing away from the camera, don't bother drawing it
    if(normal_angle >= (PI/2))
        return;
    
    lighting_pct = ((2.0*(PI - normal_angle)) / PI) - 1.0;
    r = tri->v[0].c->r * lighting_pct;
    r = r > 255.0 ? 255 : r;     
    g = tri->v[0].c->g * lighting_pct;
    g = g > 255.0 ? 255 : g;
    b = tri->v[0].c->b * lighting_pct;
    b = b > 255.0 ? 255 : b;
    
    //Also perform a TO_SCREEN_Z of the z-value to be used in z-buffer calculations
    for(i = 0; i < 3; i++) {
        
        project(&(tri->v[i]), &p[i]);
        zval[i] = TO_SCREEN_Z(tri->v[i].z);
        p[i].x = TO_SCREEN_X(p[i].x);
        p[i].y = TO_SCREEN_Y(p[i].y);
    }
	
	//Now that they're sorted, calculate the bresenham values for each line
	//For the x of the first line ([x1, y1, z1] -> [x2, y2, z2])
	dx_x1 = abs(x2 - x1);
	sx_x1 = x1 < x2 ? 1 : -1;
	dy_x1 = abs(y2 - y1);
	sy_x1 = y1 < y2 ? 1 : -1; 
	err_x1 = (dx_x1 > dy_x1 ? dx_x1 : -dy_x1) / 2;
	te_x1;
	
	//For the z of the first line ([x1, y1, z1] -> [x2, y2, z2])
	dx_z1 = abs(z2 - z1);
	sx_z1 = z1 < z2 ? 1 : -1;
	dy_z1 = abs(y2 - y1);
	sy_z1 = y1 < y2 ? 1 : -1; 
	err_z1 = (dx_z1 > dy_z1 ? dx_z1 : -dy_z1) / 2;
	te_z1;
	
	//For the x of the second line ([x2, y2, z2] -> [x3, y3, z3])
	dx_x2 = abs(x3 - x2);
	sx_x2 = x2 < x3 ? 1 : -1;
	dy_x2 = abs(y3 - y2);
	sy_x2 = y2 < y3 ? 1 : -1; 
	err_x2 = (dx_x2 > dy_x2 ? dx_x2 : -dy_x2) / 2;
	te_x2;
	
	//For the z of the second line ([x2, y2, z2] -> [x3, y3, z3])
	dx_z2 = abs(z3 - z2);
	sx_z2 = z2 < z3 ? 1 : -1;
	dy_z2 = abs(y3 - y2);
	sy_z2 = y2 < y3 ? 1 : -1; 
	err_z2 = (dx_z2 > dy_z2 ? dx_z2 : -dy_z2) / 2;
	te_z2;
	
	//For the x of the third line ([x1, y1, z1] -> [x3, y3, z3])
	dx_x3 = abs(x3 - x1);
	sx_x3 = x1 < x3 ? 1 : -1;
	dy_x3 = abs(y3 - y1);
	sy_x3 = y1 < y3 ? 1 : -1; 
	err_x3 = (dx_x3 > dy_x3 ? dx_x3 : -dy_x3) / 2;
	te_x3;
	
	//For the z of the third line ([x1, y1, z1] -> [x3, y3, z3])
	dx_z3 = abs(z3 - z1);
	sx_z3 = z1 < z3 ? 1 : -1;
	dy_z3 = abs(y3 - y1);
	sy_z3 = y1 < y3 ? 1 : -1; 
	err_z3 = (dx_z3 > dy_z3 ? dx_z3 : -dy_z3) / 2;
	te_z3;
	
	//Set the important scanlines
	current_s = y1;
	cur_x1 = x1;
	cur_z1 = z1;
	cur_x2 = x2;
	cur_z2 = z2;
	cur_x3 = x1;
	cur_z3 = z1;
	
	while(current_s <= y3) {
		
		//Run either line 1 or line 2
		if(current_s < y2) {
			
			//Draw the current scanline from line 1 to line 3
			scanline3d(r, current_s, cur_x1, cur_z1, cur_x3, cur_z3);
			
			//Run the x of line 1 until we've stepped y
			while(1) {
				
				te_x1 = err_x1;
				
				if (te_x1 > -dx_x1) {
					
					//x1-increase
					err_x1 -= dy_x1;
					cur_x1 += sx_x1;
				}
				
				if (te_x1 < dy_x1) { 
					
					//s-increase
					err_x1 += dx_x1;
					break;
				}
			}
			
			//Run the z of line 1 until we've stepped y
			while(1) {
				
				te_z1 = err_z1;
				
				if (te_z1 > -dx_z1) {
					
					//z1-increase
					err_z1 -= dy_z1;
					cur_z1 += sx_z1;
				}
				
				if (te_z1 < dy_z1) { 
					
					//s-increase
					err_z1 += dx_z1;
					break;
				}
			}
		} else {
			
			//Draw the current scanline from line 2 to line 3 
			scanline3d(r, current_s, cur_x2, cur_z2, cur_x3, cur_z3);
			
			//Run the x of line 2 until we've stepped y
			while(1) {
				
				te_x2 = err_x2;
				
				if (te_x2 > -dx_x2) {
					
					//x2-increase
					err_x2 -= dy_x2;
					cur_x2 += sx_x2;
				}
				
				if (te_x2 < dy_x2) { 
					
					//s-increase
					err_x2 += dx_x2;
					break;
				}
			}
			
			//Run the z of line 2 until we've stepped y
			while(1) {
				
				te_z2 = err_z2;
				
				if (te_z2 > -dx_z2) {
					
					//z2-increase
					err_z2 -= dy_z2;
					cur_z2 += sx_z2;
				}
				
				if (te_z2 < dy_z2) { 
					
					//s-increase
					err_z2 += dx_z2;
					break;
				}
			}
		}

		//Run line 3		
		//Run the x of line 3 until we've stepped y
		while(1) {
			
			te_x3 = err_x3;
			
			if (te_x3 > -dx_x3) {
				
				//x3-increase
				err_x3 -= dy_x3;
				cur_x3 += sx_x3;
			}
			
			if (te_x3 < dy_x3) { 
				
				//s-increase
				err_x3 += dx_x3;
				break;
			}
		}
		
		//Run the z of line 3 until we've stepped y
		while(1) {
			
			te_z3 = err_z3;
			
			if (te_z3 > -dx_z3) {
				
				//z3-increase
				err_z3 -= dy_z3;
				cur_z3 += sx_z3;
			}
			
			if (te_z3 < dy_z3) { 
				
				//s-increase
				err_z3 += dx_z3;
				break;
			}
		}
		
		//Move to the next scanline		
		current_s++;
	}
}
 
void line3d(SDL_Renderer *r, int x0, int y0, int z0, int x1, int y1, int z1) {
	
	int dx_A = abs(x1-x0), sx_A = x0<x1 ? 1 : -1;
	int dy_A = abs(y1-y0), sy_A = y0<y1 ? 1 : -1; 
	int err_A = (dx_A>dy_A ? dx_A : -dy_A)/2, te_A;
	
	int dx_B = abs(z1-z0), sx_B = z0<z1 ? 1 : -1;
	int dy_B = abs(y1-y0), sy_B = y0<y1 ? 1 : -1; 
	int err_B = (dx_B>dy_B ? dx_B : -dy_B)/2, te_B;
	
	int y0_A = y0, y0_B = y0;
	int done = 0;
	
	while(!done) {
		
		//Z loop
		for(;;){
			if (z0==z1 && y0_B==y1) { done = 1; break; }
			te_B = err_B;
			if (te_B >-dx_B) { err_B -= dy_B; z0 += sx_B;}
			if (te_B < dy_B) { err_B += dx_B; y0_B += sy_B; break;}
		}
	
		//X loop
		for(;;){
			point3d(r, x0, y0_A, z0);
			if (x0==x1 && y0_A==y1) break;
			te_A = err_A;
			if (te_A >-dx_A) { err_A -= dy_A; x0 += sx_A; }
			if (te_A < dy_A) { err_A += dx_A; y0_A += sy_A; break;}
		}
	}
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int done = 0;
    int xa = 10;
    int ya = 10;
    int za = 0;
    int xb = 100;
    int yb = 75;
    int zb = 255;
    
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

	SDL_SetRenderDrawColor(renderer, 0x20, 0x50, 0x60, 0xFF);
	SDL_RenderClear(renderer);
	triangle3d(renderer, 320, 0, 70, 0, 300, 0, 500, 400, 255);
	//line3d(renderer, xa, ya, za, xb, yb, zb);
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
