#include <acknex.h>
#include <windows.h>
//#include <default.c>
#include <ackphysx.h>

#define PRAGMA_POINTER


SOUND *wavEngine = "resources\\sounds\\engine.wav";
var sndEngine = 0;
SOUND *wavCrash = "resources\\sounds\\crash.wav";
SOUND *wavAmbient = "resources\\sounds\\ambient.wav";
var sndAmbient = 0;
SOUND *wavButtonOver = "resources\\sounds\\electric.wav";
SOUND *wavIntro = "resources\\sounds\\intro.wav";




typedef struct 
{
	ENTITY *chassis;
	ENTITY *wheel[2];
	ENTITY *antenna[3];
	
	var active;
	ENTITY *traction;
	var direction;
	var speed;
	var inContact;
	
} RANGER;

RANGER *ranger =
{
	chassis = NULL;
	wheel = NULL;
	antenna = NULL;
	
	active = 0;
	traction = NULL;
	direction = 1;
	speed = 0;
	inContact = 0;
}



void RangerCreate ();
void RangerBreak ();


BMAP *bmpArrow = "resources\\images\\arrow.tga";

BMAP *bmpTitle = "resources\\images\\title.tga";
BMAP *bmpPlayOff = "resources\\images\\play_off.tga";
BMAP *bmpEditorOff = "resources\\images\\editor_off.tga";
BMAP *bmpExitOff = "resources\\images\\exit_off.tga";
BMAP *bmpPlayOn = "resources\\images\\play_on.tga";
BMAP *bmpEditorOn = "resources\\images\\editor_on.tga";
BMAP *bmpExitOn = "resources\\images\\exit_on.tga";



PANEL* panTitle =
{
	window(0, 0, 16, 16, bmpTitle, NULL, NULL);
}

PANEL* panMain =
{
	button(  0, 0, bmpPlayOn,   bmpPlayOff,   bmpPlayOn,   ButtonPlay,NULL,ButtonOver);
	button(171, 0, bmpEditorOn, bmpEditorOff, bmpEditorOn, NULL,NULL,ButtonOver);
	button(342, 0, bmpExitOn,   bmpExitOff,   bmpExitOn,   ButtonExit,NULL,ButtonOver);
}


MATERIAL *mtlPerpective =
{
	effect = "shaders\\perspective.fx";
}

void RangerFlip ()
{
	if(!ranger->active) return;
	
	pX_pause(1);

	pXcon_remove(ranger->antenna[0]);
	pXcon_remove(ranger->antenna[1]);
	pXcon_remove(ranger->antenna[2]);
	pXent_settype(ranger->antenna[0], 0, PH_BOX);
	pXent_settype(ranger->antenna[1], 0, PH_BOX);
	pXent_settype(ranger->antenna[2], 0, PH_BOX);
	
	wait(1);
	
	VECTOR vPos;
	
	vec_set(&vPos, vector(0, -70 * ranger->direction, 40));
	vec_rotateaxis(&vPos, vector(1, 0, 0), ranger->chassis->roll);
	vec_add(&vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[0]->x, &vPos);
	vec_set(&ranger->antenna[0]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[0], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[0], ranger->chassis, 0);
	vec_set(&vPos, vector(0, -70 * ranger->direction, 10));
	vec_rotateaxis(&vPos, vector(1, 0, 0), ranger->chassis->roll);
	vec_add(&vPos, &ranger->chassis->x);
	pXcon_setparams1(ranger->antenna[0], &vPos, vector(1, 0, 0), vector(50000, 1, 0));
	pXcon_setparams2(ranger->antenna[0], vector(0, 0, 0), NULL, NULL);
	
	vec_set(&vPos, vector(0,-70 * ranger->direction, 70));
	vec_rotateaxis(&vPos, vector(1,0,0), ranger->chassis->roll);
	vec_add(&vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[1]->x, &vPos);
	vec_set(&ranger->antenna[1]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[1], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[1], ranger->antenna[0], 0);
	pXcon_setparams1(ranger->antenna[1], vector(0, ranger->antenna[1]->y, ranger->antenna[0]->z), vector(1, 0, 0), NULL);
	pXcon_setparams2(ranger->antenna[1], vector(-2, 2, 0), NULL, NULL);
	
	vec_set(&vPos, vector(0, -70 * ranger->direction, 100));
	vec_rotateaxis(&vPos, vector(1,0,0), ranger->chassis->roll);
	vec_add(&vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[2]->x, &vPos);
	vec_set(&ranger->antenna[2]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[2], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[2], ranger->antenna[1], 0);
	pXcon_setparams1(ranger->antenna[2], vector(0, ranger->antenna[2]->y, ranger->antenna[1]->z), vector(1, 0, 0), NULL);
	pXcon_setparams2(ranger->antenna[2], vector(-4, 4, 0), NULL, NULL);
	
	
	pXent_setgroup(ranger->antenna[0], 2);
	pXent_setgroup(ranger->antenna[1], 2);
	pXent_setgroup(ranger->antenna[2], 3);

	pXent_setbodyflagall(NX_BF_FROZEN_POS_X, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_PAN, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_TILT, 1);
	
	pXent_setcollisionflag(ranger->antenna[2], NULL, NX_NOTIFY_ON_TOUCH);
	ranger->antenna[2]->event = RangerBreak;
	
	ranger->direction *= -1;
	
	if(ranger->direction > 0)
	{
		ranger->traction = ranger->wheel[1];
	}
	else
	{
		ranger->traction = ranger->wheel[0];
	}
	
	pX_pause(0);
	
}

void RangerBreak ()
{
	if(!ranger->active) 
		return;
	ranger->active = 1;
	
	snd_play(wavCrash, 50, 0);
	
	snd_stop(sndEngine);
	
	pXcon_remove(ranger->wheel[0]);
	pXcon_remove(ranger->wheel[1]);
	pXcon_remove(ranger->antenna[0]);
	pXcon_remove(ranger->antenna[1]);
	pXcon_remove(ranger->antenna[2]);
	
	pXent_setgroup(ranger->chassis, 3);
	pXent_setgroup(ranger->antenna[0], 3);
	pXent_setgroup(ranger->antenna[1], 3);
	pXent_setgroup(ranger->antenna[2], 3);
	
	
	wait(-2);

	pXent_settype(ranger->chassis, 0, PH_BOX);
	pXent_settype(ranger->wheel[0], 0, PH_SPHERE);
	pXent_settype(ranger->wheel[1], 0, PH_SPHERE);
	pXent_settype(ranger->antenna[0], 0, PH_BOX);
	pXent_settype(ranger->antenna[1], 0, PH_BOX);
	pXent_settype(ranger->antenna[2], 0, PH_BOX);
	
	ent_remove(ranger->chassis);
	ent_remove(ranger->wheel[0]);
	ent_remove(ranger->wheel[1]);
	ent_remove(ranger->antenna[0]);
	ent_remove(ranger->antenna[1]);
	ent_remove(ranger->antenna[2]);
	
	RangerCreate ();
}

void RangerGroundContact ()
{
	if(ranger->traction == me)
		ranger->inContact = 1;
}

void RangerModelsCreate ()
{
}

void RangerCreate ()
{
	ranger->chassis  = ent_create("resources\\models\\bar.mdl",   vector(0,150,260), NULL);
	ranger->wheel[0] = ent_create("resources\\models\\wheel.mdl", vector(0,40,200),  NULL);
	ranger->wheel[1] = ent_create("resources\\models\\wheel.mdl", vector(0,260,200), NULL);
	
	VECTOR vPos;
	vec_set(&vPos, vector(0,70,40));
	vec_add(&vPos, &ranger->chassis->x);
	ranger->antenna[0] = ent_create("resources\\models\\antena.mdl", &vPos, NULL);
	vPos.z += 30;
	ranger->antenna[1] = ent_create("resources\\models\\antena.mdl", &vPos, NULL);
	vPos.z += 30;
	ranger->antenna[2] = ent_create("resources\\models\\antena.mdl", &vPos, NULL);
	
	vec_fill(&ranger->wheel[0]->scale_x, 1.3);
	vec_fill(&ranger->wheel[1]->scale_x, 1.3);
	
	c_setminmax(ranger->chassis);
	c_setminmax(ranger->wheel[0]);
	c_setminmax(ranger->wheel[1]);
	c_setminmax(ranger->antenna[0]);
	c_setminmax(ranger->antenna[1]);
	c_setminmax(ranger->antenna[2]);
	
	pXent_settype(ranger->chassis, PH_RIGID, PH_BOX);
	pXent_settype(ranger->wheel[0], PH_RIGID, PH_SPHERE);
	pXent_settype(ranger->wheel[1], PH_RIGID, PH_SPHERE);
	pXent_settype(ranger->antenna[0], PH_RIGID, PH_BOX);
	pXent_settype(ranger->antenna[1], PH_RIGID, PH_BOX);
	pXent_settype(ranger->antenna[2], PH_RIGID, PH_BOX);
	
	pXent_setfriction(level_ent, 500);
	pXent_setfriction(ranger->chassis, 0);
	pXent_setfriction(ranger->wheel[0], 500);
	pXent_setfriction(ranger->wheel[1], 500);
	pXent_setfriction(ranger->antenna[0], 0);
	pXent_setfriction(ranger->antenna[1], 0);
	pXent_setfriction(ranger->antenna[2], 0);
	
	pXent_setmass(ranger->chassis, 0.7);
	pXent_setmass(ranger->wheel[0], 0.1);
	pXent_setmass(ranger->wheel[1], 0.1);
	pXent_setmass(ranger->antenna[0], 0.001);
	pXent_setmass(ranger->antenna[1], 0.001);
	pXent_setmass(ranger->antenna[2], 0.002);
	
	pXent_setelasticity(ranger->chassis, 10);
	pXent_setelasticity(ranger->wheel[0], 50);
	pXent_setelasticity(ranger->wheel[1], 50);
	pXent_setelasticity(ranger->antenna[0], 10);
	pXent_setelasticity(ranger->antenna[1], 10);
	pXent_setelasticity(ranger->antenna[2], 10);
	
	pXent_setdamping(ranger->chassis, 0, 0);
	pXent_setdamping(ranger->wheel[0], 0, 50);
	pXent_setdamping(ranger->wheel[1], 0, 50);
	pXent_setdamping(ranger->antenna[0], 0, 0);
	pXent_setdamping(ranger->antenna[1], 0, 0);
	pXent_setdamping(ranger->antenna[2], 0, 0);
	
	pXent_setbodyflagall(NX_BF_FROZEN_POS_X, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_PAN, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_TILT, 1);
	
	pXent_setgroup(level_ent, 2);
	pXent_setgroup(ranger->chassis, 2);
	pXent_setgroup(ranger->wheel[0], 3);
	pXent_setgroup(ranger->wheel[1], 3);
	pXent_setgroup(ranger->antenna[0], 2);
	pXent_setgroup(ranger->antenna[1], 2);
	pXent_setgroup(ranger->antenna[2], 2);
	
	pXent_setmaxspeed(ranger->chassis, 500);
	pXent_setmaxspeed(ranger->wheel[0], 1500);
	pXent_setmaxspeed(ranger->wheel[1], 1500);
	
	
	pXcon_add(PH_BALL, ranger->wheel[0], ranger->chassis, 0);
	pXcon_setparams1 (ranger->wheel[0], &ranger->wheel[0]->x, NULL, NULL);
	
	pXcon_add(PH_BALL, ranger->wheel[1], ranger->chassis, 0);
	pXcon_setparams1 (ranger->wheel[1], &ranger->wheel[1]->x, NULL, NULL);
	
	pXcon_add(PH_HINGE, ranger->antenna[0], ranger->chassis, 0);
	pXcon_setparams1(ranger->antenna[0], vector(0, ranger->antenna[0]->y, ranger->chassis->z), vector(1,0,0), NULL);
	pXcon_setparams2(ranger->antenna[0], vector(0, 0, 0), NULL, NULL);
	pXcon_add(PH_HINGE, ranger->antenna[1], ranger->antenna[0], 0);
	pXcon_setparams1(ranger->antenna[1], vector(0, ranger->antenna[1]->y, ranger->antenna[0]->z), vector(1,0,0), NULL);
	pXcon_setparams2(ranger->antenna[1], vector(-2, 2, 0), NULL, NULL);
	pXcon_add(PH_HINGE, ranger->antenna[2], ranger->antenna[1], 0);
	pXcon_setparams1(ranger->antenna[2], vector(0, ranger->antenna[2]->y, ranger->antenna[1]->z), vector(1,0,0), NULL);
	pXcon_setparams2(ranger->antenna[2], vector(-4, 4, 0), NULL, NULL);

	pXent_setcollisionflag(ranger->chassis, NULL, NX_NOTIFY_ON_START_TOUCH);
	ranger->chassis->event = RangerBreak;
	
	pXent_setcollisionflag(ranger->wheel[1], NULL, NX_NOTIFY_ON_TOUCH);
	ranger->wheel[1]->event = RangerGroundContact;
	
	pXent_setcollisionflag(ranger->wheel[0], NULL, NX_NOTIFY_ON_TOUCH);
	ranger->wheel[0]->event = RangerGroundContact;
	
	vec_fill(&ranger->wheel[0]->scale_x, 1);
	vec_fill(&ranger->wheel[1]->scale_x, 1);
	
	ranger->active = 1;
	ranger->direction = 1;
	ranger->speed = 0;
	
	ranger->traction = ranger->wheel[1];
	
	sndEngine = snd_loop(wavEngine, 50, 0);

}

function RaceStart ()
{
	wait(2);
	level_load("resources\\ulevel.wmb");
	wait(3);

	reset(camera, ISOMETRIC);
	
	RangerCreate ();
	
	var ArcNew = 0;
	var Aceleracion = 10;
	var OverEngine = 0;
	
	on_space = RangerFlip;
	on_enter = RangerBreak;
	
	while(1)
	{
		VECTOR vSpeed;
		
		if(ranger->active)
		{
			pXent_getangvelocity(ranger->traction, &vSpeed);
			
			if(key_force.y > 0)
			{
				if(ranger->inContact)
				{
					if(!key_shift)
					{
						ranger->speed = minv(ranger->speed + time_step * Aceleracion, 100);
						pXent_addtorquelocal(ranger->traction, vector(ranger->speed * time_step * ranger->direction, 0, 0));
					}
					else
					{
						ranger->speed = minv(ranger->speed + time_step * Aceleracion * 0.1, 10);
						pXent_setangvelocity(ranger->traction, vector(ranger->speed * 15 * ranger->direction, 0, 0));
					}
				}
				
				if(ranger->speed > 300)
					OverEngine += time_step;
				
				if(OverEngine > 32)
					RangerBreak ();
				
			}
			else if(key_force.y < 0)
			{
				ranger->speed = maxv(ranger->speed -(time_step * Aceleracion * 10), 0);
				
				pXent_setangvelocity(ranger->wheel[1], vector(ranger->speed * 15 * ranger->direction, 0, 0));
				pXent_setangvelocity(ranger->wheel[0], vector(ranger->speed * 15 * ranger->direction, 0, 0));
			}
			else
			{
				if(ranger->direction > 0)
					ranger->speed = maxv(ranger->speed -(time_step * Aceleracion), 0);
				else
					ranger->speed = maxv(ranger->speed -(time_step * Aceleracion * 0.1), 0);
				
				pXent_setangvelocity(ranger->traction, vector(maxv(abs(vSpeed.x), ranger->speed * 15) * ranger->direction, 0, 0));
			}
			
			
			if(!key_shift)
				snd_tune(sndEngine, 50, 50 + maxv(vSpeed.x / 30 ,ranger->speed * 0.5), 0);
			else
				snd_tune(sndEngine, 50, 50 + maxv(vSpeed.x / 30 ,ranger->speed * 5), 0);
			
			if(key_force.x)
				pXent_addtorquelocal(ranger->chassis, vector(key_force.x * time_step * 13, 0, 0));
		}
		
		pXent_getvelocity(ranger->chassis, &vSpeed, nullvector);
		
		ArcNew = minv(60 + vec_length(&vSpeed) / 50, 100);
		camera->arc -= (camera->arc - ArcNew) * time_step * 0.1;
		
		camera->z = ranger->chassis->z;
		camera->y = ranger->chassis->y;
		
		
		ranger->inContact = maxv(ranger->inContact - time_step, 0);
		
		wait(1);
	}	
}

function ButtonExit ()
{
	sys_exit(NULL);
}

function ButtonPlay ()
{
	reset(panTitle, SHOW);
	reset(panMain, SHOW);
	
	mouse_mode = 0;
	
	RaceStart ();
}

var buttonActive = 0;

function ButtonOver(var _button, PANEL *_panel)
{
	if(buttonActive == _button) return;
	buttonActive = _button;
	
	snd_play(wavButtonOver, 50, 0);
	
	//printf("%.0f", (double)_button);
	
	var Timer = 4;
	while(Timer > 0)
	{
		var offset = random(Timer)-(Timer/2);
		if(_button == 1)
			pan_setbutton(_panel, 1, 0, offset,       0, bmpPlayOn,   bmpPlayOff,   bmpPlayOn,   bmpPlayOn,  ButtonPlay, NULL, ButtonOver);
		if(_button == 2)
			pan_setbutton(_panel, 2, 0, 171 + offset, 0, bmpEditorOn, bmpEditorOff, bmpEditorOn, bmpEditorOn,NULL,       NULL, ButtonOver);
		if(_button == 3)
			pan_setbutton(_panel, 3, 0, 342 + offset, 0, bmpExitOn,   bmpExitOff,   bmpExitOn,   bmpExitOn,  ButtonExit, NULL, ButtonOver);
			
		Timer -= time_step;
		wait(1);
	}
	
	if(_button == 1)
		pan_setbutton(_panel, 1, 0, 0,  0, bmpPlayOn,   bmpPlayOff,   bmpPlayOn,   bmpPlayOn,   ButtonPlay, NULL, ButtonOver);
	if(_button == 2)
		pan_setbutton(_panel, 2, 0, 171,0, bmpEditorOn, bmpEditorOff, bmpEditorOn, bmpEditorOn, NULL,       NULL, ButtonOver);
	if(_button == 3)
		pan_setbutton(_panel, 3, 0, 342,0, bmpExitOn,   bmpExitOff,   bmpExitOn,   bmpExitOn,   ButtonExit, NULL, ButtonOver);
	
	if(buttonActive == _button)
		buttonActive = 0;
	
}

void WindowInit ()
{
	video_window(NULL, NULL, 1, "Mars Ranger");
	video_set(sys_metrics(0), sys_metrics(1), 32, 2);
	
	mouse_pointer = 0;
	
}

void PhysXInit ()
{
	physX_open ();
	
	pX_setsteprate(120, 128, NX_TIMESTEP_FIXED);
	pX_setgravity(vector(0, 0, -9.8));
	pX_setunit(0.01);
	pX_setccd(1);
	
}

void main ()
{
	WindowInit();
	
	PhysXInit();
	
	wait(2);
	
	level_load("");
	wait(1);
	
	vec_set(&sky_color, vector(67, 127, 207));
	camera->arc = (1024 / screen_size.x) * 50;
	
	camera->x = -500;
	camera->y = 0;
	camera->z = 0;
	set(camera, ISOMETRIC);
	
	you = ent_create("resources\\models\\plane.mdl", nullvector, NULL);
	you->material = mtlPerpective;
	
	panTitle->pos_x = (screen_size.x / 2) - 256;
	panTitle->pos_y = (screen_size.y / 2) - 134;
	set(panTitle, SHOW);
	
	var sndIntro = snd_play(wavIntro, 50, 0);
	sndAmbient = snd_loop(wavAmbient, 50, 0);
	
	var translucency = 100;
	while(translucency > 0)
	{
		draw_quad(NULL, nullvector, NULL, &screen_size, NULL, nullvector, translucency, 0);
		translucency -= time_step * 5;
		wait (1);
	}
	
	while(snd_playing(sndIntro)) 
		wait(1);
	
	panMain->pos_x = (screen_size.x / 2) - 256;
	panMain->pos_y = (screen_size.y / 2) + 16;
	set(panMain, SHOW);
	
	
	SetCursorPos(window_pos.x + panMain->pos_x + 85, window_pos.y + panMain->pos_y + 25);
	mouse_mode = 4;
	mouse_map = bmpArrow;
	
	while(!key_esc)
	{
		wait(1);
	}
	
	sys_exit(NULL);
}

