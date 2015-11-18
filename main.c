#include "SDL2/SDL.h"
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_DEPTH 20.0

//Convert a point scaled such that 1.0, 1.0 is at the upper right-hand
//corner of the screen and -1.0, -1.0 is at the bottom right to pixel coords
#define PI 3.141592653589793
#define TO_SCREEN_Y(y) ((int)((SCREEN_HEIGHT-(y*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_X(x) ((int)((SCREEN_WIDTH+(x*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_Z(z) ((unsigned short)((z) > SCREEN_DEPTH || z < 0 ? 65535 : ((z*65535.0)/SCREEN_DEPTH)))
#define DEG_TO_RAD(a) ((((float)a)*PI)/180.0)

float focal_length;
unsigned short *zbuf;

typedef struct point {
    float x;
    float y;
} point;

//Should just make these float too
//so as to reduce the amount of casting
//until we're actually drawing on the screen
//and thereby improve speed and precision
typedef struct screen_point {
    int x;
    int y;
    float u;
    float v;
    unsigned short z;
} screen_point;

typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} color;

typedef struct vertex {
    float x;
    float y;
    float z;
    float u;
    float v;
    color *c;
} vertex;

//For now, a texture is an array of monochrome data
typedef struct texture {
    int height;
    int width;
    unsigned int* data;
} texture;

typedef struct triangle {
    vertex v[3];
    texture *t;
} triangle;

typedef struct node {
    void *payload;
    struct node *next;
} node;

typedef struct list {
    node *root;
} list;

typedef struct object {
    list tri_list;
    float x;
    float y;
    float z;
} object;

#define list_for_each(l, i, n) for((i) = (l)->root, (n) = 0; (i) != NULL; (i) = (i)->next, (n)++)
#define new(x) ((x*)malloc(sizeof(x)))

void clear_zbuf() {
    
    memset((void*)zbuf, 255, SCREEN_PIXELS*2);  
}

int init_zbuf() {
    
    zbuf = (unsigned short*)malloc(SCREEN_PIXELS*2);
    
    if(!zbuf)
        return 0;
    
    clear_zbuf();  
        
    return 1;
}

void clone_color(color* src, color* dst) {
    
    dst->r = src->r;
    dst->g = src->g;
    dst->b = src->b;
    dst->a = src->a;
}

node *list_get_last(list *target) {
    
    int i;
    node *list_node;
    
    list_for_each(target, list_node, i) {
        
        if(list_node->next == NULL)
            return list_node;
    }
    
    return NULL;
}

void list_push(list *target, void* item) {
    
    node *last     = list_get_last(target);
    node *new_node = new(node);

    if(!new_node) 
         return;
    

    new_node->payload = item;
    new_node->next = NULL;
        
    if(!last) {
     
        target->root = new_node;
    } else {      
     
        last->next = new_node;
    }
}

void dump_list(list *target) {
    
    node *item;
    
    printf("list.root = 0x%08x\n", (unsigned int)target->root);
    item = target->root;
    
    while(item) {
        
        printf("   Item 0x%08x:\n", (unsigned int)item);
        printf("      payload: 0x%08x\n", (unsigned int)item->payload);
        printf("      next: 0x%08x\n", (unsigned int)item->next);
        item = item->next;
    }
}

color *new_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    
    color *ret_color = new(color);
    
    if(!ret_color)
        return ret_color;
    
    ret_color->r = r;
    ret_color->g = g;
    ret_color->b = b;
    ret_color->a = a;
    
    return ret_color;
}

void clone_vertex(vertex *src, vertex* dst) {
    
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->c = src->c;
}

unsigned int test_data[] = {
    0xFF8080, 0xFF6060, 0xFF4040, 0xFF2020, 0xFF0000, 0xD00000, 0xB00000, 0x900000, 0x700000, 0x500000,
    0xFFFF80, 0xFFE060, 0xFFC040, 0xFFA020, 0xFF8000, 0xD06000, 0xB04000, 0x902000, 0x700000, 0x500000,
    0xFFFF80, 0xFFFF60, 0xFFFF40, 0xFFFF20, 0xFFFF00, 0xD0D000, 0xB0B000, 0x909000, 0x707000, 0x505000,
    0x80FF80, 0x60FF60, 0x40FF40, 0x20FF20, 0x00FF00, 0x00D000, 0x00B000, 0x009000, 0x007000, 0x005000,
    0x8080FF, 0x6060FF, 0x4040FF, 0x2020FF, 0x0000FF, 0x0000D0, 0x0000B0, 0x000090, 0x000070, 0x000050,
    0xFF80FF, 0xE060FF, 0xC040FF, 0xA020FF, 0x8000FF, 0x6000D0, 0x4000B0, 0x200090, 0x000070, 0x000050,
    0xFFFFFF, 0xE0E0E0, 0xC0C0C0, 0xA0A0A0, 0x808080, 0x606060, 0x505050, 0x404040, 0x303030, 0x202020,
    0xFFFFFF, 0xE0E0E0, 0xC0C0C0, 0xA0A0A0, 0x808080, 0x606060, 0x505050, 0x404040, 0x303030, 0x202020,
    0xFFFFFF, 0xE0E0E0, 0xC0C0C0, 0xA0A0A0, 0x808080, 0x606060, 0x505050, 0x404040, 0x303030, 0x202020,
    0xFFFFFF, 0xE0E0E0, 0xC0C0C0, 0xA0A0A0, 0x808080, 0x606060, 0x505050, 0x404040, 0x303030, 0x202020
};
texture *new_texture(unsigned char* texture_file) {
    
    texture *ret_texture = new(texture);
    
    if(!ret_texture)
        return ret_texture;
        
    ret_texture->height = 10;
    ret_texture->width = 10;
    ret_texture->data = test_data;
    
    return ret_texture;
}

triangle *new_triangle(vertex *v1, vertex *v2, vertex *v3, texture *t) {
    
    triangle *ret_tri = new(triangle);
    
    if(!ret_tri)
        return ret_tri;
        
    clone_vertex(v1, &(ret_tri->v[0]));
    clone_vertex(v2, &(ret_tri->v[1]));
    clone_vertex(v3, &(ret_tri->v[2]));
    ret_tri->t = t;    
    
    return ret_tri;
}

void purge_list(list *target) {
    
    node *temp_node, *next_node;
    
    for(temp_node = target->root; temp_node != NULL;) {
        next_node = temp_node->next;
        free(temp_node);
        temp_node = next_node;
    }
}

void delete_object(object *obj) {
    
    node *item;
    int i;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        free(item->payload);
    }
    
    purge_list(&(obj->tri_list));
    free(obj);
}

object *new_object() {
    
    object *ret_obj = new(object);
    
    if(!ret_obj)
        return ret_obj;
        
    ret_obj->tri_list.root = NULL;
    ret_obj->x = ret_obj->y = ret_obj->z = 0.0;
    
    return ret_obj;
}

/*
//Needs to be updated to work with textures
object *new_cube(float s, color *c) {
    
    object* ret_obj = new_object();
    int i;
        triangle *temp_tri;
    vertex temp_v[3];
    float half_s = s/2.0;
    float points[][5] = {
        {-half_s, half_s, -half_s, 1.0, 0.0}, //Adding U and V 
        {half_s, half_s, -half_s, 0.0, 0.0},
        {half_s, -half_s, -half_s, },
        {-half_s, -half_s, -half_s},
        {-half_s, half_s, half_s},
        {half_s, half_s, half_s},
        {half_s, -half_s, half_s},
        {-half_s, -half_s, half_s},
    };
    const int order[][3] = {
                  {7, 5, 4}, //3
                  {6, 5, 7}, //3
                  {3, 0, 1}, //1
                  {3, 1, 2}, //1
                  {4, 5, 0}, 
                  {1, 0, 5},
                  {6, 7, 3},
                  {3, 2, 6},
                  {6, 1, 5},
                  {6, 2, 1},
                  {7, 4, 0},
                  {0, 3, 7}
              };
    
    if(!ret_obj) {
        
        printf("[new_cube] object allocation failed\n");
        return ret_obj;
    }
    
    printf("[new_cube] new object allocated\n");
        
    if(!ret_obj)
        return ret_obj;
    
    temp_v[0].c = c;
    temp_v[1].c = c;
    temp_v[2].c = c;
    
    for(i = 0; i < 12; i++) {
     
        temp_v[0].x = points[order[i][0]][0];
        temp_v[0].y = points[order[i][0]][1];
        temp_v[0].z = points[order[i][0]][2];
        temp_v[0].u = 0.0;
        temp_v[0].v = 0.0;
        temp_v[1].x = points[order[i][1]][0];
        temp_v[1].y = points[order[i][1]][1];
        temp_v[1].z = points[order[i][1]][2];
        temp_v[1].u = 0.0;
        temp_v[1].v = 0.0;
        temp_v[2].x = points[order[i][2]][0];
        temp_v[2].y = points[order[i][2]][1];
        temp_v[2].z = points[order[i][2]][2];
        
        printf("[new_cube] Creating new triangle (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", temp_v[0].x, temp_v[0].y, temp_v[0].z, temp_v[1].x, temp_v[1].y, temp_v[1].z,  temp_v[2].x, temp_v[2].y, temp_v[2].z);
        
        if(!(temp_tri = new_triangle(&temp_v[0], &temp_v[1], &temp_v[2], new_texture("none")))) {
            
            printf("[new_cube] failed to allocate triangle #%d\n", i+1);
            delete_object(ret_obj);
            return NULL;        
        }
        printf("[new_cube] generated triangle #%d\n", i+1);
        
        list_push(&(ret_obj->tri_list), (void*)temp_tri);
        printf("[new_cube] inserted triangle #%d\n", i+1);
    }
    
    return ret_obj;
}
*/

void translate_object(object* obj, float x, float y, float z) {
    
    triangle *temp_tri;
    node     *item;
    int      i, j;
    
    obj->x += x;
    obj->y += y;
    obj->z += z;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
        
            temp_tri->v[j].x += x;
            temp_tri->v[j].y += y;
            temp_tri->v[j].z += z;
        }
    }
}

void rotate_object_x_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_y, temp_z;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
        
            temp_y = temp_tri->v[j].y;
            temp_z = temp_tri->v[j].z;
        
            temp_tri->v[j].y = (temp_y * cos(rad_angle)) - (temp_z * sin(rad_angle));
            temp_tri->v[j].z = (temp_y * sin(rad_angle)) + (temp_z * cos(rad_angle));
        }
    }
}

void rotate_object_y_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_x, temp_z;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
            
            temp_x = temp_tri->v[j].x;
            temp_z = temp_tri->v[j].z;
            
            temp_tri->v[j].x = (temp_x * cos(rad_angle)) + (temp_z * sin(rad_angle));
            temp_tri->v[j].z = (temp_z * cos(rad_angle)) - (temp_x * sin(rad_angle));
        }
    }
}

void rotate_object_z_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_x, temp_y;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
            
            temp_x = temp_tri->v[j].x;
            temp_y = temp_tri->v[j].y;
            
            temp_tri->v[j].x = (temp_x * cos(rad_angle)) - (temp_y * sin(rad_angle));
            temp_tri->v[j].y = (temp_x * sin(rad_angle)) + (temp_y * cos(rad_angle));
        }
    }
}

void rotate_object_x_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_x_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void rotate_object_y_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_y_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void rotate_object_z_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_z_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void project(vertex* v, screen_point* p) {

    float delta = (v->z == 0.0) ? 1.0 : (focal_length/v->z);

    p->x = TO_SCREEN_X(v->x * delta);
    p->y = TO_SCREEN_Y(v->y * delta);
    p->z = TO_SCREEN_Z(v->z);
    
    p->u = v->u;
    p->v = v->v;
}

//Draw an rgb-colored line along the scanline from x=x1 to x=x2, interpolating
//z-values and only drawing the pixel if the interpolated z-value is less than
//the value already written to the z-buffer
void draw_scanline(SDL_Renderer *r, float scanline, float x0, float z0, float u0, float v0, float x1, float z1, float u1, float v1, texture *tex) {

    unsigned short newz;
    int z_addr;
    unsigned int newu, newv;	
	float dz, dx, du, dv, mz, mu, mv, newz_f, newu_f, newv_f, t; 
                 
    //don't draw off the screen
    if(scanline >= SCREEN_HEIGHT || scanline < 0)
    	return;  
    
    //Clamp u and v values to 1.0 x 1.0 space
    u0 = u0 < 0.0 ? 0.0 : u0 > 1.0 ? 1.0 : u0;
    u1 = u1 < 0.0 ? 0.0 : u1 > 1.0 ? 1.0 : u1;
    v0 = v0 < 0.0 ? 0.0 : v0 > 1.0 ? 1.0 : v0;
    v1 = v1 < 0.0 ? 0.0 : v1 > 1.0 ? 1.0 : v1;
      
    if(x0 > x1) {
     
        //Swap x
        t = x0;
        x0 = x1;
        x1 = t;
        
        //Swap z
        t = z0;
        z0 = z1;
        z1 = t;
        
        //Swap u
        t = u0;
        u0 = u1;
        u1 = t;
        
        //Swap v
        t = u0;
        u0 = u1;
        u1 = t;
    } 
    
    x0 = floor(x0);
	dz = z1 - z0;
    dx = x1 - x0;
    du = u1 - u0;
    dv = v1 - v0;
    mz = dx ? dz/dx : 0;
    mu = dx ? du/dx : 0;
    mv = dx ? dv/dx : 0;
    z_addr = scanline * SCREEN_WIDTH + x0;
       
    for(; x0 <= x1; x0 += 1, z_addr++) {

        if(x0 < SCREEN_WIDTH && x0 >= 0) {

            //Calculate interpolated z value
            newz_f = mz*(x0 - x1) + z1;
            newz = (unsigned short)lround(newz_f >= 65535 ? 65535 : newz_f < 0 ? 0 : newz_f);

            //Check the z buffer and draw the point	
            if(newz < zbuf[z_addr]) {
                
                                    
                    //Calculate interpolated u value
                    newu_f = mu*(x0 - x1) + u1;
                    newu = (unsigned int)lround(newu_f * (tex->width - 1)); //-1
                        
                    //Calculate interpolated v value
                    newv_f = mv*(x0 - x1) + v1;
                    newv = (unsigned int)lround(newv_f * (tex->height - 1)); //-1
                    
                    //Need to make this conform to lighting in the future
                    SDL_SetRenderDrawColor(r, (tex->data[newv * tex->width + newu] & 0xFF0000) >> 16, (tex->data[newv * tex->width + newu] & 0xFF00) >> 8, tex->data[newv * tex->width + newu] & 0xFF, 0xFF);
                
                    //Uncomment the below to view the depth buffer
                    //SDL_SetRenderDrawColor(r, newz >> 8, newz >> 8, newz >> 8, 0xFF);
                    SDL_RenderDrawPoint(r, lround(x0), lround(scanline));
                    zbuf[z_addr] = newz;
            }
        }
    }
}

void draw_triangle(SDL_Renderer *rend, triangle* tri) {
    
    int i;
    screen_point p[3];
    float vec_a[3];
    float vec_b[3];
    float cross[3];
    float mag;
    float normal_angle;
    float lighting_pct;
    float r, g, b;
    unsigned char f, s, t, e;
    float dx_1, dx_2, dx_3, dy_1, dy_2,	dy_3, dz_1, dz_2, dz_3, du_1, du_2, du_3, dv_1, dv_2, dv_3;
    float mx_1, mx_2, mx_3, mz_1, mz_2, mz_3, mu_1, mu_2, mu_3, mv_1, mv_2, mv_3;
    float new_x1, new_x2, new_x3, new_z1, new_z2, new_z3, new_u1, new_u2, new_u3, new_v1, new_v2, new_v3;
    float first_orig_x, first_orig_y, first_orig_z, first_orig_u, first_orig_v, second_orig_x, second_orig_y, second_orig_z, second_orig_u, second_orig_v;
	float current_s;
    
    //Don't draw the triangle if it's offscreen
    if(tri->v[0].z < 0 && tri->v[1].z < 0 && tri->v[2].z < 0)
        return;
    
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
    if(normal_angle >= (3*PI/4)) {
        
        return;
    }
    
    //Calculate the shading color based on the first vertex color and the
    //angle between the camera and the surface normal
    lighting_pct = 1.0 - (normal_angle/PI);
    r = (float)tri->v[0].c->r * lighting_pct;
    r = r > 255.0 ? 255 : r;     
    g = (float)tri->v[0].c->g * lighting_pct;
    g = g > 255.0 ? 255 : g;
    b = (float)tri->v[0].c->b * lighting_pct;
    b = b > 255.0 ? 255 : b;
    SDL_SetRenderDrawColor(rend, (unsigned char)r, (unsigned char)g, (unsigned char)b, 0xFF);
    
    //Move the vertices from world space to screen space
    for(i = 0; i < 3; i++) 
        project(&(tri->v[i]), &p[i]);
    
    //sort vertices by ascending y
    f = 0; s = 1; t = 2;
    if(p[f].y > p[s].y) {
        e = s;
        s = f;
        f = e;
    }
    if(p[s].y > p[t].y) {
        e = t;
        t = s;
        s = e;
    }
    if(p[f].y > p[s].y) {
        e = s;
        s = f;
        f = e;
    }
                    
	//Set the important scanlines
	current_s = p[f].y;
	dx_1 = p[s].x - p[f].x;
    dy_1 = p[s].y - p[f].y;
    dz_1 = p[s].z - p[f].z;
    du_1 = p[s].u - p[f].u; 
    dv_1 = p[s].v - p[f].v;
    dx_2 = p[t].x - p[s].x;
    dy_2 = p[t].y - p[s].y;
    dz_2 = p[t].z - p[s].z;
    du_2 = p[t].u - p[s].u; 
    dv_2 = p[t].v - p[s].v;
    dx_3 = p[t].x - p[f].x;
    dy_3 = p[t].y - p[f].y;
    dz_3 = p[t].z - p[f].z;
    du_3 = p[t].u - p[f].u; 
    dv_3 = p[t].v - p[f].v;
    mx_1 = dy_1 ? dx_1 / dy_1 : 0;
    mz_1 = dy_1 ? dz_1 / dy_1 : 0;
    mu_1 = dy_1 ? du_1 / dy_1 : 0;
    mv_1 = dy_1 ? dv_1 / dy_1 : 0;
    mx_2 = dy_2 ? dx_2 / dy_2 : 0;
    mz_2 = dy_2 ? dz_2 / dy_2 : 0;
    mu_2 = dy_2 ? du_2 / dy_2 : 0;
    mv_2 = dy_2 ? dv_2 / dy_2 : 0;
    mx_3 = dy_3 ? dx_3 / dy_3 : 0;
    mz_3 = dy_3 ? dz_3 / dy_3 : 0; 
    mu_3 = dy_3 ? du_3 / dy_3 : 0;
    mv_3 = dy_3 ? dv_3 / dy_3 : 0;
    first_orig_x = p[f].x;
    first_orig_y = p[f].y;
    first_orig_z = p[f].z;
    first_orig_u = p[f].u;
    first_orig_v = p[f].v;
    second_orig_x = p[s].x;
    second_orig_y = p[s].y;
    second_orig_z = p[s].z;
    second_orig_u = p[s].u;
    second_orig_v = p[s].v;
	
	while(current_s < p[t].y) {
		
        if(!(current_s >= SCREEN_HEIGHT || current_s < 0)) {
            
            new_x3 = mx_3*(current_s - first_orig_y) + first_orig_x;
            new_z3 = mz_3*(current_s - first_orig_y) + first_orig_z;
            new_u3 = mu_3*(current_s - first_orig_y) + first_orig_u;
            new_v3 = mv_3*(current_s - first_orig_y) + first_orig_v;
            
            if(current_s < p[s].y) {
                
                new_x1 = mx_1*(current_s - first_orig_y) + first_orig_x;
                new_z1 = mz_1*(current_s - first_orig_y) + first_orig_z;
                new_u1 = mu_1*(current_s - first_orig_y) + first_orig_u;
                new_v1 = mv_1*(current_s - first_orig_y) + first_orig_v;
                
                //Draw the scanline from the first edge to the third 
                draw_scanline(rend, current_s, new_x1, new_z1, new_u1, new_v1, new_x3, new_z3, new_u3, new_v3, tri->t);
            } else {
                
                new_x2 = mx_2*(current_s - second_orig_y) + second_orig_x;
                new_z2 = mz_2*(current_s - second_orig_y) + second_orig_z;
                new_u2 = mu_2*(current_s - second_orig_y) + second_orig_u;
                new_v2 = mv_2*(current_s - second_orig_y) + second_orig_v;
                
                //Draw the scanline from the second edge to the third 
                draw_scanline(rend, current_s, new_x2, new_z2, new_u2, new_v2, new_x3, new_z3, new_u3, new_v3, tri->t);
            }
        }
           
		//Move to the next scanline		
		current_s += 1;
	}
}

void clip_and_render(SDL_Renderer *r, triangle* tri) {    

    int count;
    int on_second_iteration = 0;
    int i;
    float plane_z = 0.1; //This is how far in front of camera the front clipping plane is
    float scale_factor, dx, dy, dz, du, dv, ndz;
    unsigned char point_marked[3] = {0, 0, 0};
    vertex new_point[2]; 
    triangle out_triangle[2];
    int fixed[2];
    int original;
    
    
    //Note that in the future we're also going to need to clip on the
    //'U', 'V' and color axes
    while(1) {
        
        count = 0;
        
        //Check each point to see if it's greater than the plane
        for(i = 0; i < 3; i++) {
            
            if((on_second_iteration && tri->v[i].z > plane_z) || 
              (!on_second_iteration && tri->v[i].z < plane_z)) {

                point_marked[i] = 1;
                count++;
            } else {
                
                point_marked[i] = 0;
            }
        }
            
        switch(count) {
            
            //If all of the vertices were out of range, 
            //skip drawing the whole thing entirely
            case 3:
                return;
                break;
            
            //If one vertex was out, find it's edge intersections and
            //build two new triangles out of it
            case 1:
                //Figure out what the other two points are
                fixed[0] = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 2 : 1 : fixed[0] == 1 ? point_marked[0] ? 2 : 0 : point_marked[0] ? 1 : 0;
                original = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x,y, and z 'length'
                    dx = tri->v[original].x - tri->v[fixed[i]].x;
                    dy = tri->v[original].y - tri->v[fixed[i]].y;
                    dz = tri->v[original].z - tri->v[fixed[i]].z;
                    du = tri->v[original].u - tri->v[fixed[i]].u;
                    dv = tri->v[original].v - tri->v[fixed[i]].v;
                                       
                    //Set the known axis value
                    new_point[i].z = plane_z; //Replace this with a line function
                    
                    //z 'length' of new point
                    ndz = new_point[i].z - tri->v[fixed[i]].z;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndz/dz; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as U and V
                    //Update: AND NOW WE ARE, MOTHERFUCKER
                    new_point[i].x = scale_factor * dx + tri->v[fixed[i]].x;
                    new_point[i].y = scale_factor * dy + tri->v[fixed[i]].y;
                    new_point[i].u = scale_factor * du + tri->v[fixed[i]].u;
                    new_point[i].v = scale_factor * dv + tri->v[fixed[i]].v;
                    
                    //Copy the color information
                    new_point[i].c = tri->v[fixed[i]].c;
                }   
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                //Build the first triangle
                clone_vertex(&new_point[0], &(out_triangle[0].v[original]));
                clone_vertex(&(tri->v[fixed[0]]), &(out_triangle[0].v[fixed[0]]));
                clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[0].v[fixed[1]]));
                    
                //Build the second triangle    
                clone_vertex(&new_point[1], &(out_triangle[1].v[original]));
                clone_vertex(&new_point[0], &(out_triangle[1].v[fixed[0]]));
                clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[1].v[fixed[1]]));
                
                //Run the new triangles through another round of processing
                clip_and_render(r, &out_triangle[0]);
                clip_and_render(r, &out_triangle[1]);
                
                //Exit the function early for dat tail recursion              
                return;
                break;
            
            case 2:
                //Figure out which point we're keeping
                original = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[0] = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 1 : 2 : fixed[0] == 1 ? point_marked[0] ? 0 : 2 : point_marked[0] ? 0 : 1;
                            
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x,y, and z 'length'
                    dx = tri->v[original].x - tri->v[fixed[i]].x;
                    dy = tri->v[original].y - tri->v[fixed[i]].y;
                    dz = tri->v[original].z - tri->v[fixed[i]].z;
                    du = tri->v[original].u - tri->v[fixed[i]].u;
                    dv = tri->v[original].v - tri->v[fixed[i]].v;
                                       
                    //Set the known axis value
                    new_point[i].z = plane_z; //Replace this with a line function
                    
                    //z 'length' of new point
                    ndz = new_point[i].z - tri->v[fixed[i]].z;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndz/dz; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as U and V
                    //Update: AND NOW WE ARE, MOTHERFUCKER
                    new_point[i].x = scale_factor * dx + tri->v[fixed[i]].x;
                    new_point[i].y = scale_factor * dy + tri->v[fixed[i]].y;
                    new_point[i].u = scale_factor * du + tri->v[fixed[i]].u;
                    new_point[i].v = scale_factor * dv + tri->v[fixed[i]].v;
                    
                    //Copy the color information
                    new_point[i].c = tri->v[fixed[i]].c;
                }      
                
                //Start building the new triangles, maintaining the CW or CCW ordering 
                clone_vertex(&(tri->v[original]), &(out_triangle[0].v[original]));
                clone_vertex(&new_point[0], &(out_triangle[0].v[fixed[0]]));
                clone_vertex(&new_point[1], &(out_triangle[0].v[fixed[1]]));
                
                //Send through processing again
                clip_and_render(r, &out_triangle[0]);
                    
                //Exit the function early for dat tail recursion  
                return;
                break;
            
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
        plane_z = SCREEN_DEPTH;
    }    
    
    //If we got this far, the triangle is drawable. So we should do that. Or whatever.
    draw_triangle(r, tri);   
}

void render_triangle(SDL_Renderer *rend, triangle* tri) {

    clip_and_render(rend, tri);
}

void render_object(SDL_Renderer *r, object *obj) {
    
    node* item;
    int i;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        render_triangle(r, (triangle*)item->payload);
    }
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int fov_angle, player_angle = 90, chg_angle = 0;
    float i = 0.0, step = 0.001, rstep = 0, fps, walkspeed = 0.04;
    color *c;
    object *cube1, *cube2;
    triangle test_tri[2];
    int done = 0;
    int numFrames = 0; 
    Uint32 startTime = SDL_GetTicks(), frame_start;
    char title[255] = "LESTER";

    if(!init_zbuf()) {
        
        printf("Could not init the z-buffer\n");
        return -1;
    }

    if(!(c = new_color(50, 200, 255, 255))) {
        
        printf("Could not allocate a new color\n");
        return -1;
    } 
    
    printf("Color created successfully\n");
    
/*
    if(!(cube1 = new_cube(5.0, new_color(255, 255, 255, 255)))) {
        
        printf("Could not allocate a new cube\n");
        return -1;
    }
    
    if(!(cube2 = new_cube(1.0, c))) {
        
        printf("Could not allocate a new cube\n");
        return -1;
    }

    printf("Cube created successfully\n");
*/
    fov_angle = 50;
    focal_length = 1.0 / (2.0 * tan(DEG_TO_RAD(fov_angle)/2.0));

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

    //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    SDL_SetRelativeMouseMode(SDL_TRUE);

//    translate_object(cube1, 0.0, -3.0, 2.0);
//    translate_object(cube2, 0.0, 0.0, 2.0);
    //rotate_object_y_local(cube, 45);
    //rotate_object_x_local(cube, 45);
    //rotate_object_z_local(cube, 45);
    
    test_tri[0].t = new_texture("none");
    test_tri[0].v[0].x = 0.5;
    test_tri[0].v[0].y = 0.5;
    test_tri[0].v[0].z = 1.0;
    test_tri[0].v[0].u = 1.0;
    test_tri[0].v[0].v = 0.0;
    test_tri[0].v[0].c = c;
    test_tri[0].v[1].x = 0.5;
    test_tri[0].v[1].y = -0.5;
    test_tri[0].v[1].z = 1.0;
    test_tri[0].v[1].u = 1.0;
    test_tri[0].v[1].v = 1.0;
    test_tri[0].v[1].c = c;
    test_tri[0].v[2].x = -0.5;
    test_tri[0].v[2].y = -0.5;
    test_tri[0].v[2].z = 1.0;
    test_tri[0].v[2].u = 0.0;
    test_tri[0].v[2].v = 1.0;
    test_tri[0].v[2].c = c;
    test_tri[1].t = new_texture("none");
    test_tri[1].v[0].x = -0.5;
    test_tri[1].v[0].y = 0.5;
    test_tri[1].v[0].z = 1.0;
    test_tri[1].v[0].u = 0.0;
    test_tri[1].v[0].v = 0.0;
    test_tri[1].v[0].c = c;
    test_tri[1].v[1].x = 0.5;
    test_tri[1].v[1].y = 0.5;
    test_tri[1].v[1].z = 1.0;
    test_tri[1].v[1].u = 1.0;
    test_tri[1].v[1].v = 0.0;
    test_tri[1].v[1].c = c;
    test_tri[1].v[2].x = -0.5;
    test_tri[1].v[2].y = -0.5;
    test_tri[1].v[2].z = 1.0;
    test_tri[1].v[2].u = 0.0;
    test_tri[1].v[2].v = 1.0;
    test_tri[1].v[2].c = c;

    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
                
            if(e.type == SDL_KEYDOWN) {
                
                switch(e.key.keysym.sym) {
                    
                    case SDLK_w:
                        
                        step = walkspeed;
                    break;
                    
                    case SDLK_s:
                    
                        step = -walkspeed;
                    break;
                    
                    case SDLK_a:
                        
                        rstep = -walkspeed;
                    break;
                    
                    case SDLK_d:
                    
                        rstep = walkspeed;
                    break;
                    
                    default:
                        done = 1;
                        break;
                }
            }
            
            if(e.type == SDL_KEYUP) {
                
                switch(e.key.keysym.sym) {
                    
                    case SDLK_a:
                    case SDLK_d:
                        rstep = 0;
                    break;
                    
                    case SDLK_w:
                    case SDLK_s:
                        step = 0;
                    break;
                }
            }
            
            if(e.type == SDL_MOUSEMOTION) {
                                
                chg_angle += e.motion.xrel;
            } 
        }

        frame_start = SDL_GetTicks();
        i += step;
        //translate_object(cube1, 0.0, 0.0, step);
        //rotate_object_y_local(cube1, 1);
        //rotate_object_x_local(cube1, 1);
        //rotate_object_z_local(cube1, 1);       
  /*      rotate_object_y_global(cube2, -chg_angle);    
        translate_object(cube2, -rstep, 0.0, -step);
        rotate_object_y_global(cube1, -chg_angle);
        translate_object(cube1, -rstep, 0.0, -step); */
        //rotate_object_x_local(cube2, 1);
        //rotate_object_z_local(cube2, 1);

        player_angle += chg_angle;
        chg_angle = 0;

        if(player_angle == 360)
            player_angle = 0;
            
        if(player_angle == -1)
            player_angle = 359;

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        clear_zbuf();
        
        //render_object(renderer, cube1);
        //render_object(renderer, cube2);  
        test_tri[0].v[2].z += step;
        test_tri[1].v[0].z += step;
        test_tri[1].v[2].z += step;
        render_triangle(renderer, &test_tri[0]);
        render_triangle(renderer, &test_tri[1]);
        
        SDL_RenderPresent(renderer);
        numFrames++;        
        fps = ( numFrames/(float)(SDL_GetTicks() - startTime) )*1000;
        sprintf(&title, "LESTER %f FPS", fps);
        SDL_SetWindowTitle(window, &title);
        
        if((step < 0 && i <= 0.0) || (step > 0 && i >= 1.0))
            step = -step;
               
        //while((SDL_GetTicks() - frame_start) <= 14);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
