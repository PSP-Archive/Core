#include "raster.h"
#include "render.h"
#include "control.h"
#include "malloc.h"
#include "math.h"

#include <stdio.h>

#include "formations.c"
#include "player.c"

void sceKernelExitGame();

void *sceKernelAllocPartitionMemory(unsigned long howmuch);
unsigned long sceKernelTotalFreeMemSize();
unsigned int sceDisplayGetVcount();
int sceKernelCreateCallback(char *name, void *callback);
void sceKernelRegisterExitCallback(int id);
void KernelPollCallbacks();
int sceKernelCreateThread(char *name, void *thread, int uk1, int uk2, int uk3,
		int uk4);
void sceKernelStartThread(int id, int a, int b);


//each pixel == 2 bytes
//each line = 1024 bytes
//each frame = 0x44000 bytes

int exit_callback(void)
{
	// Exit game
	sceKernelExitGame();
	return 0;
}


int CallBackThread(void *arg) {
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback);
	sceKernelRegisterExitCallback(cbid);
	KernelPollCallbacks();

	return 0;
}

//mersenne twister
unsigned int mt(){
        static unsigned int s1 = 1;
        s1 ^= (s1 >> 11);
        s1 ^= (s1 << 7) & 0x9d2c5680U;
        s1 ^= (s1 << 15) & 0xefc60000U;
        s1 ^= (s1 >> 18);
        return s1;
}

typedef struct bullet {
	int active;
	int pos[2];
} bullet;

typedef struct target {
	int active;
	int pos[2];
	int base; //base position. If !0, have wave motion
} target;

target targets[15];
int freetargets = 15;

bullet bullets[100] = {{0,{0,0}}};
int hits[10][3]; //active x y

int initsquad() {
	int i;
	int formation = mt() % NO_FORMATIONS;
	int offset = mt() % 360;
	int unit = 0;
	int wave = (mt() % 4 == 0); //1 in 4 that it will have wave motion
	for (i = 0;i < 15;i++) {
		if (targets[i].active)
			continue;
		//we've found one
		targets[i].pos[0] = (offset + formations[formation][unit][0]
				* 3) % 360;
		targets[i].pos[1] = -60 - formations[formation][unit][1];
		targets[i].active = 1;

		if (wave)
			targets[i].base = targets[i].pos[0];
		else
			targets[i].base = 0;
		
		unit++;
		if (unit == 5)
			break;
	}
	freetargets -= unit;
	return 0;
}

int xmain() {
	int x = 0, y = 0, v;
	ctrl_data_t ctl;
	malloc_init();

	//install callback thread
	v = sceKernelCreateThread("update_thread", CallBackThread,
			0x11, 0xFa0, 0, 0);
	sceKernelStartThread(v, 0, 0);

	rast_init();

	int i, j, angle = 0, shudder = 0;
	int hit[20] = {0};
	
	initsquad();
	initsquad();
	initsquad();
	
	int fdelay = 0;
	sceCtrlSetAnalogMode(1);
	
	while(1) {

		sceCtrlRead(&ctl, 1);
		if (ctl.buttons & 0x10)
			y++;
		if (ctl.buttons & 0x40)
			y--;
		if (ctl.buttons & 0x80) {
			x+= 4;
			angle++;
		}
		if (ctl.buttons & 0x20) {
			x-= 4;
			angle--;
		}
		if (ctl.buttons & 0x4000 && !fdelay) {// X
			for (i = 0;i < 100;i++) {
				if (!bullets[i].active)
					break;
			}
			if (i != 99) {
				bullets[i].pos[0] = x;
				bullets[i].pos[1] = -6;
				bullets[i].active = 1;
			}
			fdelay = 2;
		}
		if (ctl.buttons & 0x01) //select
			sceKernelExitGame();
		if (ctl.buttons & 0x200) //rtrigger
			angle++;
		if (ctl.buttons & 0x100) //ltrigger
			angle--;
		x -= (ctl.analog[0] - 127) * 4 /127;
		angle -= (ctl.analog[0] - 127) * 8 /127;


		if (fdelay) fdelay--;
		
		x %= 360;
		if (x < 0)
			x = 360 + x;
		angle %= 360;
		if (angle < 0)
			angle = 360 + angle;

		rast_fill(0);
		rast_blank_z();
		rend_reset();

		if (shudder) {
			rend_translate(mt() % 10 / 40.0f,mt() % 10 / 40.0f, 0);
			shudder--;
		}

		rend_rotate(0, 0, 1, angle);
		
		rend_pushmatrix();
		for (i = 0;i < 10;i++) {
			rend_rotate(0, 0, 1, 18);
			
			if (hit[i]) {
				rend_colour(0x7c0);
				hit[i]--;
			}else
				rend_colour(0xe40);
				
			rend_vertex(-0.6f, -5, 0);
			rend_vertex( 0.6f, -5, 0);
			rend_vertex( 0.6f, -5, -60);
			rend_vertex(-0.6f, -5, -60);
			rend_poly_fill();

			if (hit[i + 10]) {
				rend_colour(0x7c0);
				hit[i + 10]--;
			}else
				rend_colour(0xe40);		
		
			rend_vertex( 0.6f, 6, 0);
			rend_vertex(-0.6f, 6, 0);
			rend_vertex(-0.6f, 6, -60);
			rend_vertex( 0.6f, 6, -60);
			rend_poly_fill();

			rend_colour(0xfffe);
			rend_line(-0.6f, -6, 0, -0.6f, -5, -60);
			rend_line( 0.6f, -6, 0,  0.6f, -5, -60);
			rend_line(-0.6f, 6, 0, -0.6f, 5, -60);
			rend_line( 0.6f, 6, 0,  0.6f, 5, -60);
		}

		rend_popmatrix();

		rend_texture(player, playerw, playerh);
		rend_pushmatrix();
		rend_colour(0xf800);
		rend_rotate(0, 0, 1, x);
		rend_translate(0,0,-5);
		rend_vertex_tc(-1, -4.6f, 0, 0,  0);
		rend_vertex_tc( 1, -4.6f, 0, 1,  0);
		rend_vertex_tc( 0, -4.6f,-2,0.5, 1);
		rend_poly_fill();
		rend_popmatrix();
		
		rend_colour(0x08bf);
		float dx, dy;
		for (i = 0;i < 15;i++) {
			if (!targets[i].active)
				continue;
			if (targets[i].pos[1] > -1) {
				targets[i].active = 0;
				freetargets++;
			}

			targets[i].pos[1] += 1;

			if (targets[i].base)
				targets[i].pos[0] += (sin(targets[i].pos[1]*24)-
					(sin((targets[i].pos[1] - 2)*24)))*5;
			
			rend_pushmatrix();
			rend_rotate(0, 0, 1, targets[i].pos[0]);
			rend_translate(0, 0, targets[i].pos[1]);
			rend_vertex( 1, -4.6f, 0);
			rend_vertex(-1, -4.6f, 0);
			rend_vertex( 0, -4.6f, 2);
			rend_poly_fill();
			rend_popmatrix();
		}
		
		rend_colour(0x7fe);
		for (i = 0;i < 100;i++) {
			if (!bullets[i].active)
				continue;
			if (bullets[i].pos[1] < -60) {
				bullets[i].active = 0;
				continue;
			}

			for (j = 0;j < 15;j++) {
				if (!targets[j].active)
					continue;
				dx = bullets[i].pos[0] - targets[j].pos[0];
				dy = bullets[i].pos[1] - targets[j].pos[1];
				if (dx * dx + dy * dy < 6) {
					targets[j].active = 0;
					freetargets++;
					shudder = 4;
					hit[targets[j].pos[0] / 18] = 2;
				}
			}

			bullets[i].pos[1] -= 1;
			rend_pushmatrix();
			rend_rotate(0, 0, 1, bullets[i].pos[0]);
			rend_line(0, -4, bullets[i].pos[1], 0, -4, bullets[i].pos[1] + 1);
			rend_popmatrix();
		}

		if (freetargets > 5)
			initsquad();

		rast_flip();
	}
	return 0;

}


