#include "SDL.h"
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_DEPTH 0.5

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

typedef struct point {
    float x;
    float y;
} point;

void draw_triangle(SDL_Renderer *r, point p0, point p1, point p2) {
    
    SDL_RenderDrawLine(r, TO_SCREEN_X(p0.x), TO_SCREEN_Y(p0.y), TO_SCREEN_X(p1.x), TO_SCREEN_Y(p1.y));
    SDL_RenderDrawLine(r, TO_SCREEN_X(p1.x), TO_SCREEN_Y(p1.y), TO_SCREEN_X(p2.x), TO_SCREEN_Y(p2.y));
    SDL_RenderDrawLine(r, TO_SCREEN_X(p2.x), TO_SCREEN_Y(p2.y), TO_SCREEN_X(p0.x), TO_SCREEN_Y(p0.y));
}

void render_clipped_triangles(SDL_Renderer *r, point p0, point p1, point p2) {    

    int count;
    int on_second_iteration = 0;
    int i;
    float plane_y = 0;
    float scale_factor, dx, dy, ndy;
    unsigned char point_marked[3] = {0, 0, 0};
    point new_point[2]; 
    point apoint[3];
    int fixed[2];
    int original;
    
    apoint[0].x = p0.x;
    apoint[0].y = p0.y;
    apoint[1].x = p1.x;
    apoint[1].y = p1.y;
    apoint[2].x = p2.x;
    apoint[2].y = p2.y;
    
    //Note that in the future we're also going to need to clip on the
    //'U', 'V' and color axes
    while(1) {
        
        count = 0;
        
        //Check each point to see if it's greater than the plane
        for(i = 0; i < 3; i++)
            if((on_second_iteration && apoint[i].y > plane_y) || 
              (!on_second_iteration && apoint[i].y < plane_y)) {

                point_marked[i] = 1;
                count++;
            }
            
        switch(count) {
            
            //If all of the vertices were out of range, 
            //skip drawing the whole thing entirely
            case 3:
                return;
            
            //If one vertex was out, find it's edge intersections and
            //build two new triangles out of it
            case 1:
                //Figure out what the other two points are
                fixed[0] = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 2 : 1 : fixed[0] == 1 ? point_marked[0] ? 2 : 0 : point_marked[0] ? 1 : 0;
                original = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x 'length'
                    dx = apoint[original].x - apoint[fixed[i]].x;
                    
                    //y 'length'
                    dy = apoint[original].y - apoint[fixed[i]].y;
                    
                    //Set the known axis value
                    new_point[i].y = plane_y; //Replace this with a line function
                    
                    //y 'length' of new point
                    ndy = new_point[i].y - apoint[fixed[i]].y;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndy/dy; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as Z and U and V
                    new_point[i].x = scale_factor * dx + apoint[fixed[i]].x;
                }   
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                if(fixed[0] < fixed[1]) {
                    
                    render_clipped_triangles(r, new_point[0], apoint[fixed[0]], apoint[fixed[1]]);
                    render_clipped_triangles(r, new_point[1], new_point[0], apoint[fixed[1]]);
                } else {
                    
                    render_clipped_triangles(r, new_point[0], apoint[fixed[1]], apoint[fixed[0]]);
                    render_clipped_triangles(r, new_point[1], apoint[fixed[1]], new_point[0]);
                }
                
                //Exit the function early for dat tail recursion              
                return;
            
            case 2:
                //Figure out which point we're keeping
                original = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[0] = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 1 : 2 : fixed[0] == 1 ? point_marked[0] ? 0 : 2 : point_marked[0] ? 0 : 1;
                            
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x 'length'
                    dx = apoint[original].x - apoint[fixed[i]].x;
                    
                    //y 'length'
                    dy = apoint[original].y - apoint[fixed[i]].y;
                    
                    //Set the known axis value
                    new_point[i].y = plane_y; //Replace this with a line function
                    
                    //y 'length' of new point
                    ndy = new_point[i].y - apoint[fixed[i]].y;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndy/dy; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as Z and U and V
                    new_point[i].x = scale_factor * dx + apoint[fixed[i]].x;
                }    
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                if(fixed[0] < fixed[1])                     
                    render_clipped_triangles(r, new_point[0], new_point[1], apoint[original]);
                else 
                    render_clipped_triangles(r, new_point[1], new_point[0], apoint[original]);
                    
                //Exit the function early for dat tail recursion  
                return;
            
            //If there were no intersections we won't do anything and 
            //allow execution to flow through
            case 0:
            default:
                break; 
        }

        //If we hit case 0 both times above, all points on this
        //triangle lie in the drawable area and we can leave this
        //clipping loop and flow down to do the drawing of the 
        //processed triangle         
        if(on_second_iteration)
            break;
        
        on_second_iteration = 1;
        plane_y = SCREEN_DEPTH;
    }    
    
    //If we got this far, the triangle is drawable. So we should do that. Or whatever.
    SDL_SetRenderDrawColor(r, 0xFF, 0xFF, 0x0, 0xFF);
    draw_triangle(r, p0, p1, p2);   
}

void render_clipped_triangles_old(SDL_Renderer *r, point p0, point p1, point p2) {    
    //begin split_tri (one, two, three)
    //    count = 0
    //    check one with back (if out, count++ and mark out) 
    //    check two with back (if out, count++ and mark out)
    //    check three with back (if out, count++ and mark out)
    //    switch count
    //        1: get new back intersections a and b
    //           split_tri(a, two, three) //actually make sure we build the right tri
    //           split_tri(b, two, three) //same note
    //           return
    //        2: get new back intersections a and b //different from above for the aforementioned reasons
    //           split_tri(a, b, three) //same note as always
    //           return
    //        0: //do nothing and flow through
    //    endswitch
    //    //Could probably shorten this by making two passes at the above while swapping planes
    //    count = 0
    //    check one with front (if out, count++ and mark out) 
    //    check two with front (if out, count++ and mark out)
    //    check three with front (if out, count++ and mark out)
    //    switch count
    //        1: get new front intersections a and b
    //           split_tri(a, two, three) //actually make sure we build the right tri
    //           split_tri(b, two, three) //same note
    //           return
    //        2: get new front intersections a and b //different from above for the aforementioned reasons
    //           split_tri(a, b, three) //same note as always
    //           return
    //        0: //do nothing and flow through
    //    endswitch
    //    //There are no triangles out, so we can pass this one along
    //    draw_triangle(one, two, three)
    //end  

    int count;
    int on_second_iteration = 0;
    int i;
    float plane_y = 0;
    unsigned char point_marked[3] = {0, 0, 0};
    point new_point[2]; 
    point apoint[3];
    int fixed[2];
    int original;
    
    apoint[0].x = p0.x;
    apoint[0].y = p0.y;
    apoint[1].x = p1.x;
    apoint[1].y = p1.y;
    apoint[2].x = p2.x;
    apoint[2].y = p2.y;
    
    while(1) {
        
        count = 0;
        
        //Check each point to see if it's greater than the plane
        for(i = 0; i < 3; i++)
            if((on_second_iteration && apoint[i].y > plane_y) || 
              (!on_second_iteration && apoint[i].y < plane_y)) {

                point_marked[i] = 1;
                count++;
            }
            
        switch(count) {
            
            //If all of the triangles were out of range, 
            //skip drawing the whole thing entirely
            case 3:
                return;
            
            case 1:
                //Figure out what the other two points are
                fixed[0] = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 2 : 1 : fixed[0] == 1 ? point_marked[0] ? 2 : 0 : point_marked[0] ? 1 : 0;
                original = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                
                //Calculate the new intersection points 
                for(i = 0; i < 2; i++) {
                    
                    new_point[i].y = plane_y;
                    new_point[i].x = apoint[original].x + (((apoint[original].x - apoint[fixed[i]].x)*(plane_y - apoint[original].y))/(apoint[original].y - apoint[fixed[i]].y));
                }   
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                if(fixed[0] < fixed[1]) {
                    
                    render_clipped_triangles(r, new_point[0], apoint[fixed[0]], apoint[fixed[1]]);
                    render_clipped_triangles(r, new_point[1], new_point[0], apoint[fixed[1]]);
                } else {
                    
                    render_clipped_triangles(r, new_point[0], apoint[fixed[1]], apoint[fixed[0]]);
                    render_clipped_triangles(r, new_point[1], apoint[fixed[1]], new_point[0]);
                }
                
                //Exit the function early for dat tail recursion              
                return;
            
            case 2:
                //Figure out which point we're keeping
                original = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[0] = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 1 : 2 : fixed[0] == 1 ? point_marked[0] ? 0 : 2 : point_marked[0] ? 0 : 1;
                            
                //Calculate the new intersection points 
                for(i = 0; i < 2; i++) {
                    
                    new_point[i].y = plane_y;
                    new_point[i].x = apoint[original].x + (((apoint[original].x - apoint[fixed[i]].x)*(plane_y - apoint[original].y))/(apoint[original].y - apoint[fixed[i]].y));
                } 
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                if(fixed[0] < fixed[1])                     
                    render_clipped_triangles(r, new_point[0], new_point[1], apoint[original]);
                else 
                    render_clipped_triangles(r, new_point[1], new_point[0], apoint[original]);
                    
                //Exit the function early for dat tail recursion  
                return;
            
            //If there were no intersections we won't do anything and 
            //allow execution to flow through
            case 0:
            default:
                break; 
        }

        //If we hit case 0 both times above, all points on this
        //triangle lie in the drawable area and we can leave this
        //clipping loop and flow down to do the drawing of the 
        //processed triangle         
        if(on_second_iteration)
            break;
        
        on_second_iteration = 1;
        plane_y = SCREEN_DEPTH;
    }    
    
    //If we got this far, the triangle is drawable. So we should do that. Or whatever.
    SDL_SetRenderDrawColor(r, 0xFF, 0xFF, 0x0, 0xFF);
    draw_triangle(r, p0, p1, p2);   
}

void render_triangle(SDL_Renderer *r, point p0, point p1, point p2) {
    
    render_clipped_triangles(r, p0, p1, p2);
}

void rotate_point_global(point *p, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    float temp_x, temp_y;
                 
    temp_x = p->x;
    temp_y = p->y;
    
    p->x = (temp_x * cos(rad_angle)) + (temp_y * sin(rad_angle));
    p->y = (temp_y * cos(rad_angle)) - (temp_x * sin(rad_angle));
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int done = 0, draw_orig = 1;
    point points[3];
    
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
    
    points[0].x = -1.0;
    points[0].y = 0.25;
    points[1].x = 1.0;
    points[1].y = 1.0;
    points[2].x = 0.0;
    points[2].y = -0.5;
    


    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
                
            if(e.type == SDL_KEYDOWN)
                draw_orig = !draw_orig;
        }
        
        //Clear the screen
    	SDL_SetRenderDrawColor(renderer, 0x20, 0x50, 0x60, 0xFF);
    	SDL_RenderClear(renderer);
    
        //Draw the bounding 'planes'
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0x0, 0xFF);
        SDL_RenderDrawLine(renderer, 0, TO_SCREEN_Y(SCREEN_DEPTH) + 1, SCREEN_WIDTH - 1, TO_SCREEN_Y(SCREEN_DEPTH) + 1);
        SDL_RenderDrawLine(renderer, 0, TO_SCREEN_Y(SCREEN_DEPTH), SCREEN_WIDTH - 1, TO_SCREEN_Y(SCREEN_DEPTH));
        SDL_RenderDrawLine(renderer, 0, TO_SCREEN_Y(0), SCREEN_WIDTH - 1, TO_SCREEN_Y(0));
        SDL_RenderDrawLine(renderer, 0, TO_SCREEN_Y(0) - 1, SCREEN_WIDTH - 1, TO_SCREEN_Y(0) - 1);
        
        rotate_point_global(&points[0], 1);
        rotate_point_global(&points[1], 1);
        rotate_point_global(&points[2], 1);
        
        if(draw_orig) {
         
            SDL_SetRenderDrawColor(renderer, 0x0, 0xFF, 0x0, 0xFF);
            draw_triangle(renderer, points[0], points[1], points[2]);
        }
        
        render_triangle(renderer, points[0], points[1], points[2]);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
