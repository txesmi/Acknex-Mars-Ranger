#include <acknex.h>
#include <windows.h>
//#include <default.c>
#include "includes\\ackphysx.h"

/*******************************************************************************************/

#define PRAGMA_POINTER

void RangerBreak ();
void RangerGroundContact ();
void RaceStart ();


/*******************************************************************************************/

#define GROUP_LEVEL    2
#define GROUP_WHEEL    3
#define GROUP_CHASSIS  4
#define GROUP_ANTENNA  5
#define GROUP_GOAL     6

/*******************************************************************************************/

typedef struct {
	long offset; // offset of the list from the start of the WMB file, in bytes
	long length; // length of the list, in bytes
} LIST;

typedef struct {  
	char version[4]; // "WMB7"  
	LIST palettes;// WMB1..6 only
	LIST legacy1; // WMB1..6 only
	LIST textures;// textures list  
	LIST legacy2; // WMB1..6 only
	LIST pvs;     // BSP only 
	LIST bsp_nodes; // BSP only 
	LIST materials; // material names
	LIST legacy3; // WMB1..6 only
	LIST legacy4; // WMB1..6 only
	LIST aabb_hulls; // WMB1..6 only
	LIST bsp_leafs;  // BSP only 
	LIST bsp_blocks; // BSP only 
	LIST legacy5; // WMB1..6 only
	LIST legacy6; // WMB1..6 only
	LIST legacy7; // WMB1..6 only
	LIST objects; // entities, paths, sounds, etc.
	LIST lightmaps; // lightmaps for blocks
	LIST blocks;  // block meshes
	LIST legacy8; // WMB1..6 only
	LIST lightmaps_terrain; // lightmaps for terrains
} WMB_HEADER;


var highscore = 0;

void HighScoreGet(STRING *_filename)
{
	long _size;
	WMB_HEADER *_header = file_load(_filename->chars, NULL, &_size);
	memcpy(&highscore, &_header->legacy2.length, sizeof(var));
	file_load(NULL, _header, &_size);
}

void HighScoreSet(STRING *_filename, var _score)
{
	highscore = _score;
	long _size;
	WMB_HEADER *_header = file_load(_filename->chars, NULL, &_size);
	memcpy(&_header->legacy2.length, &_score, sizeof(var));
	file_save(_filename->chars, _header, _size);
	file_load(NULL, _header, &_size);
}

/*******************************************************************************************/

SOUND *wavEngine = "resources\\sounds\\engine.wav";
var sndEngine = 0;
SOUND *wavCrash = "resources\\sounds\\crash.wav";
SOUND *wavAmbient = "resources\\sounds\\ambient.wav";
var sndAmbient = 0;
SOUND *wavButtonOver = "resources\\sounds\\electric.wav";
SOUND *wavIntro = "resources\\sounds\\intro.wav";
SOUND *wavHighScore = "resources\\sounds\\highscore.wav";



/*******************************************************************************************/

BMAP *bmpArrow = "resources\\images\\arrow.tga";

BMAP *bmpTitle = "resources\\images\\title.tga";
BMAP *bmpPlayOff = "resources\\images\\play_off.tga";
BMAP *bmpEditorOff = "resources\\images\\editor_off.tga";
BMAP *bmpExitOff = "resources\\images\\exit_off.tga";
BMAP *bmpPlayOn = "resources\\images\\play_on.tga";
BMAP *bmpEditorOn = "resources\\images\\editor_on.tga";
BMAP *bmpExitOn = "resources\\images\\exit_on.tga";

PANEL *panTitle =
{
	bmap = bmpTitle;
}

#define MAIN_PLAY   1
#define MAIN_EDITOR 2
#define MAIN_EXIT   3

#define MAIN_PLAY_POS    0
#define MAIN_EDITOR_POS  171
#define MAIN_EXIT_POS    342

PANEL *panMain =
{
	button(0,   0, bmpPlayOn,    bmpPlayOff,   bmpPlayOn,    ButtonPlay, NULL, ButtonOver);
	button(171, 0, bmpEditorOff, bmpEditorOff, bmpEditorOff, NULL,       NULL, NULL);
	button(342, 0, bmpExitOn,    bmpExitOff,   bmpExitOn,    ButtonExit, NULL, ButtonOver);
}

/*******************************************************************************************/

TEXT *txtLevels =
{
	pos_x = 10;
	pos_y = 10;
	
	strings = 1024;
	
	//flags = SHOW;
}

STRING *strLevel = "";

/*******************************************************************************************/

int goalCount = 0;
ENTITY *entGoal = NULL;

var gTimer = 0;
var gTimerLast = 0;

FONT *fntTitle = "Arial#24b";
FONT *fntStats = "Arial#20";
FONT *fntTimer = "Arial#40";

PANEL *panLevel =
{
	pos_x = 10;
	pos_y = 10;
	digits(0,    0, strLevel,        fntTitle, 1, NULL);
	digits(10,  30, "Best:   %6.3f", fntStats, 1, highscore);
	digits(10,  50, "Last:   %6.3f", fntStats, 1, gTimerLast);
	digits(140, 30, "%6.3f",         fntTimer, 1, gTimer);
	
}

/*******************************************************************************************/

MATERIAL *mtlPerpective =
{
	effect = "shaders\\perspective.fx";
}

MATERIAL *mtlSpecSolid =
{
	effect = "shaders\\SpecSolid.fx";
}

MATERIAL *mtlTriplanar =
{
	effect = "shaders\\triplanar.fx";
}

/*******************************************************************************************/

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


/*******************************************************************************************/

void GoalHit()
{
	goalCount -= 1;
	if(goalCount > 0)
		return;
	
	wait(1);
	
	RangerBreak();
	
	if(gTimer < highscore)
	{
		HighScoreSet(strLevel, gTimer);
		snd_play(wavHighScore, 50, 0);
	}
	
	gTimerLast = gTimer;
	
}

void EventChassis()
{
	if(!ranger->active) 
		return;
	
	ENTITY *_ent = NULL;
	if(you)
	{
		if((you->skill99 > 0) && (you->body))
		{
			pXent_settype(you, 0, 0);
			_ent = you;
			GoalHit();
		}
		else
		{
			RangerBreak();
			return;
		}
	}

	wait(1);
	
	if(_ent)
		ent_remove(_ent);
}

void EventWheel()
{
	if(!ranger->active) 
		return;
	
	ENTITY *_ent = NULL;
	if(you)
	{
		if((you->skill99 > 0) && (you->body))
		{
			pXent_settype(you, 0, 0);
			_ent = you;
			GoalHit();
		}
		else
		{
			if(ranger->traction == me)
				ranger->inContact = 1;
			return;
		}
	}

	wait(1);
	
	if(_ent)
		ent_remove(_ent);
}

void EventAntenna()
{
	if(!ranger->active) 
		return;
	
	ENTITY *ent = NULL;
	if(you)
	{
		if((you->skill99 > 0) && (you->body))
		{
			pXent_settype(you, 0, 0);
			ent = you;
			GoalHit();
		}
	}
	wait(1);
	
	if(ent)
		ent_remove(ent);
}

/*******************************************************************************************/

void RangerFlip ()
{
	if(!ranger->active) 
		return;
	
	pX_pause(1);
	
	ENTITY *_ent = entGoal;
	for(; _ent; _ent = _ent->parent)
	{
		pXent_setcollisionflag(ranger->antenna[0], _ent, NX_IGNORE_PAIR);
		pXent_setcollisionflag(ranger->antenna[1], _ent, NX_IGNORE_PAIR);
		pXent_setcollisionflag(ranger->antenna[2], _ent, NX_IGNORE_PAIR);
	}
	
	pXcon_remove(ranger->antenna[0]);
	pXcon_remove(ranger->antenna[1]);
	pXcon_remove(ranger->antenna[2]);
	pXent_settype(ranger->antenna[0], 0, PH_BOX);
	pXent_settype(ranger->antenna[1], 0, PH_BOX);
	pXent_settype(ranger->antenna[2], 0, PH_BOX);
	
	VECTOR _vPos;
	
	vec_set(&_vPos, vector(0, -70 * ranger->direction, 40));
	vec_rotateaxis(&_vPos, vector(1, 0, 0), ranger->chassis->roll);
	vec_add(&_vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[0]->x, &_vPos);
	vec_set(&ranger->antenna[0]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[0], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[0], ranger->chassis, 0);
	vec_set(&_vPos, vector(0, -70 * ranger->direction, 10));
	vec_rotateaxis(&_vPos, vector(1, 0, 0), ranger->chassis->roll);
	vec_add(&_vPos, &ranger->chassis->x);
	pXcon_setparams1(ranger->antenna[0], &_vPos, vector(1, 0, 0), vector(50000, 1, 0));
	pXcon_setparams2(ranger->antenna[0], vector(0, 0, 0), NULL, NULL);
	
	vec_set(&_vPos, vector(0,-70 * ranger->direction, 70));
	vec_rotateaxis(&_vPos, vector(1,0,0), ranger->chassis->roll);
	vec_add(&_vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[1]->x, &_vPos);
	vec_set(&ranger->antenna[1]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[1], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[1], ranger->antenna[0], 0);
	pXcon_setparams1(ranger->antenna[1], vector(0, ranger->antenna[1]->y, ranger->antenna[0]->z), vector(1, 0, 0), NULL);
	pXcon_setparams2(ranger->antenna[1], vector(-2, 2, 0), NULL, NULL);
	
	vec_set(&_vPos, vector(0, -70 * ranger->direction, 100));
	vec_rotateaxis(&_vPos, vector(1,0,0), ranger->chassis->roll);
	vec_add(&_vPos, &ranger->chassis->x);
	vec_set(&ranger->antenna[2]->x, &_vPos);
	vec_set(&ranger->antenna[2]->pan, &ranger->chassis->pan);
	pXent_settype(ranger->antenna[2], PH_RIGID, PH_BOX);
	pXcon_add(PH_HINGE, ranger->antenna[2], ranger->antenna[1], 0);
	pXcon_setparams1(ranger->antenna[2], vector(0, ranger->antenna[2]->y, ranger->antenna[1]->z), vector(1, 0, 0), NULL);
	pXcon_setparams2(ranger->antenna[2], vector(-4, 4, 0), NULL, NULL);
	
	pXent_setgroup(ranger->antenna[0], GROUP_CHASSIS);
	pXent_setgroup(ranger->antenna[1], GROUP_CHASSIS);
	pXent_setgroup(ranger->antenna[2], GROUP_ANTENNA);
	
	pXent_setcollisionflag(ranger->antenna[2], NULL, NX_NOTIFY_ON_START_TOUCH);
	
	
	ENTITY *_ent = entGoal;
	for(; _ent; _ent = _ent->parent)
	{
		pXent_setcollisionflag(ranger->antenna[0], _ent, NX_NOTIFY_ON_START_TOUCH);
		pXent_setcollisionflag(ranger->antenna[1], _ent, NX_NOTIFY_ON_START_TOUCH);
		pXent_setcollisionflag(ranger->antenna[2], _ent, NX_NOTIFY_ON_START_TOUCH);
	}
	
	pXent_setbodyflagall(NX_BF_FROZEN_POS_X, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_PAN, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_TILT, 1);
	
	ranger->direction *= -1;
	
	if(ranger->direction > 0)
		ranger->traction = ranger->wheel[1];
	else
		ranger->traction = ranger->wheel[0];
	
	mtlSpecSolid->skill1 = floatv(ranger->direction);
	
	pX_pause(0);
}

void RangerBreak ()
{
	if(!ranger->active) 
		return;
	ranger->active = 0;
	
	on_space = NULL;
	on_enter = NULL;
	on_pgup = NULL;
	on_pgdn = NULL;
	
	snd_play(wavCrash, 50, 0);
	
	if(snd_playing(sndEngine))
		snd_stop(sndEngine);
	sndEngine = NULL;
	
	pXcon_remove(ranger->wheel[0]);
	pXcon_remove(ranger->wheel[1]);
	pXcon_remove(ranger->antenna[0]);
	pXcon_remove(ranger->antenna[1]);
	pXcon_remove(ranger->antenna[2]);
	
	pXent_setgroup(ranger->chassis, 3);
	pXent_setgroup(ranger->antenna[0], 3);
	pXent_setgroup(ranger->antenna[1], 3);
	pXent_setgroup(ranger->antenna[2], 3);
	
	var _timer = 16;
	while(_timer > 0)
	{
		physX_run(time_frame / 16);
		_timer -= time_step;
		wait(1);
	}

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
	
	RaceStart ();
}

void RangerCreate (VECTOR *vPos)
{
	VECTOR _vPos;
	vec_set(&_vPos, vPos);
	
	ranger->chassis  = ent_create("resources\\models\\bar.mdl",   &_vPos, NULL);
	
	_vPos.y += 110;
	_vPos.z -= 70;
	ranger->wheel[0] = ent_create("resources\\models\\wheel.mdl", &_vPos, NULL);
	ranger->wheel[0]->material = mtlSpecSolid;
	
	_vPos.y -= 220;
	ranger->wheel[1] = ent_create("resources\\models\\wheel.mdl", &_vPos, NULL);
	ranger->wheel[1]->material = mtlSpecSolid;
	
	vec_set(&_vPos, vector(0, -70, 40));
	vec_add(&_vPos, &ranger->chassis->x);
	ranger->antenna[0] = ent_create("resources\\models\\antena.mdl", &_vPos, NULL);
	_vPos.z += 30;
	ranger->antenna[1] = ent_create("resources\\models\\antena.mdl", &_vPos, NULL);
	_vPos.z += 30;
	ranger->antenna[2] = ent_create("resources\\models\\antena.mdl", &_vPos, NULL);
	
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
	
	pXent_setdamping(ranger->chassis, 0, 100);
	pXent_setdamping(ranger->wheel[0], 0, 100);
	pXent_setdamping(ranger->wheel[1], 0, 100);
	pXent_setdamping(ranger->antenna[0], 0, 100);
	pXent_setdamping(ranger->antenna[1], 0, 100);
	pXent_setdamping(ranger->antenna[2], 0, 100);
	
	pXent_setbodyflagall(NX_BF_FROZEN_POS_X, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_PAN, 1);
	pXent_setbodyflagall(NX_BF_FROZEN_TILT, 1);
	
	pXent_setgroup(level_ent, GROUP_LEVEL);
	pXent_setgroup(ranger->chassis, GROUP_CHASSIS);
	pXent_setgroup(ranger->wheel[0], GROUP_WHEEL);
	pXent_setgroup(ranger->wheel[1], GROUP_WHEEL);
	pXent_setgroup(ranger->antenna[0], GROUP_CHASSIS);
	pXent_setgroup(ranger->antenna[1], GROUP_CHASSIS);
	pXent_setgroup(ranger->antenna[2], GROUP_ANTENNA);
	
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

//	pXent_setcollisionflag(ranger->chassis, NULL, NX_NOTIFY_ON_START_TOUCH);
//	ranger->chassis->event = EventChassis;
	
	pXent_setcollisionflag(ranger->wheel[1], NULL, NX_NOTIFY_ON_TOUCH);
	ranger->wheel[1]->event = EventWheel;
	
	pXent_setcollisionflag(ranger->wheel[0], NULL, NX_NOTIFY_ON_TOUCH);
	ranger->wheel[0]->event = EventWheel;
	
	ranger->antenna[0]->event = EventAntenna;
	ranger->antenna[1]->event = EventAntenna;
	
	pXent_setcollisionflag(ranger->antenna[2], NULL, NX_NOTIFY_ON_START_TOUCH);
	ranger->antenna[2]->event = EventChassis;
	
	pX_setgroupcollision(GROUP_LEVEL, GROUP_CHASSIS, 0);
	pX_setgroupcollision(GROUP_LEVEL, GROUP_WHEEL, 1);
	pX_setgroupcollision(GROUP_LEVEL, GROUP_ANTENNA, 1);
	pX_setgroupcollision(GROUP_CHASSIS, GROUP_WHEEL, 0);
	pX_setgroupcollision(GROUP_CHASSIS, GROUP_ANTENNA, 0);
	pX_setgroupcollision(GROUP_CHASSIS, GROUP_GOAL, 1);
	pX_setgroupcollision(GROUP_WHEEL, GROUP_ANTENNA, 0);
	pX_setgroupcollision(GROUP_WHEEL, GROUP_GOAL, 1);
	
	
	vec_fill(&ranger->wheel[0]->scale_x, 1);
	vec_fill(&ranger->wheel[1]->scale_x, 1);
	ranger->wheel[0]->scale_x *= -1;
	ranger->wheel[1]->scale_x *= -1;
	
	ent_morph(ranger->chassis, "resources\\models\\chassis.mdl");
	ranger->chassis->material = mtlSpecSolid;
	
	ranger->active = 1;
	ranger->direction = -1;
	ranger->speed = 0;
	
	mtlSpecSolid->skill1 = floatv(ranger->direction);
	
	ranger->traction = ranger->wheel[0];
	
	sndEngine = snd_loop(wavEngine, 50, 0);

}

/*******************************************************************************************/

void LevelNext ()
{
	txtLevels->skill_x += 1;
	if(txtLevels->skill_x >= txtLevels->skill_y)
		txtLevels->skill_x = 0;
	
	wait(1);
	RangerBreak();
	
}

void LevelPrev ()
{
	txtLevels->skill_x -= 1;
	if(txtLevels->skill_x < 0)
		txtLevels->skill_x = txtLevels->skill_y - 1;
	
	wait(1);
	RangerBreak();
	
}

void RaceStart ()
{
	str_cpy(strLevel, "resources\\");
	str_cat(strLevel, (txtLevels->pstring)[txtLevels->skill_x]);
	level_load(strLevel);
	
	HighScoreGet(strLevel);
	if(highscore < 1)
		highscore = 999999;
	
	
	int _i = 0;
	int _iLast = minv((level_ent->max_y + level_ent->max_z) / 500.0, 500);
	for(; _i < _iLast; _i += 1)
	{
		int _cloud = random(4);
		switch(_cloud)
		{
			case 0: 
				you = ent_create("resources\\images\\cloud00.tga", vec_scale(vector(-2000 - random(5000), level_ent->min_y + random(level_ent->max_y - level_ent->min_y), level_ent->min_z + random(level_ent->max_z - level_ent->min_z)), 1.2), NULL); break;
			case 1: 
				you = ent_create("resources\\images\\cloud01.tga", vec_scale(vector(-2000 - random(5000), level_ent->min_y + random(level_ent->max_y - level_ent->min_y), level_ent->min_z + random(level_ent->max_z - level_ent->min_z)), 1.2), NULL); break;
			case 2: 
				you = ent_create("resources\\images\\cloud02.tga", vec_scale(vector(-2000 - random(5000), level_ent->min_y + random(level_ent->max_y - level_ent->min_y), level_ent->min_z + random(level_ent->max_z - level_ent->min_z)), 1.2), NULL); break;
			case 3: 
				you = ent_create("resources\\images\\cloud03.tga", vec_scale(vector(-2000 - random(5000), level_ent->min_y + random(level_ent->max_y - level_ent->min_y), level_ent->min_z + random(level_ent->max_z - level_ent->min_z)), 1.2), NULL); break;
		}
		vec_fill(&you->scale_x, 4 + random(2));
		set(you, BRIGHT);
		//vec_fill()
	}
	
	var _timeStart = total_ticks;
	gTimer = 0;
	set(panLevel, SHOW);
	
	var _arcNew = 0;
	var _acceleration = 10;
	
	on_space = RangerFlip;
	on_enter = RangerBreak;
	on_pgup = LevelPrev;
	on_pgdn = LevelNext;
	
	while(!key_esc)
	{
		gTimer = (total_ticks - _timeStart) / 16.0;
		
		physX_run(time_frame / 16);
		
		VECTOR vSpeed;
		
		if(ranger->active)
		{
			if(ranger->chassis->z < level_ent->min_z)
			{
				RangerBreak();
				break;
			}
			
			pXent_getangvelocity(ranger->traction, &vSpeed);
			
			if(key_force.y > 0)
			{
//				if(ranger->inContact)
//				{
					if(!key_shift)
					{
						ranger->speed = minv(ranger->speed + time_step * _acceleration, 100);
						pXent_addtorquelocal(ranger->traction, vector(ranger->speed * time_step * ranger->direction, 0, 0));
					}
					else
					{
						ranger->speed = minv(ranger->speed + time_step * _acceleration * 0.1, 10);
						pXent_setangvelocity(ranger->traction, vector(ranger->speed * 15 * ranger->direction, 0, 0));
					}
//				}
				
				
			}
			else if(key_force.y < 0)
			{
				ranger->speed = maxv(ranger->speed -(time_step * _acceleration * 10), 0);
				
				if(ranger->traction == ranger->wheel[1])
					pXent_setangvelocity(ranger->wheel[0], vector(ranger->speed * 15 * ranger->direction, 0, 0));
				else
					pXent_setangvelocity(ranger->wheel[1], vector(ranger->speed * 15 * ranger->direction, 0, 0));
			}
			else
			{
//				if(ranger->direction > 0)
//					ranger->speed = maxv(ranger->speed -(time_step * _acceleration), 0);
//				else
					ranger->speed = maxv(ranger->speed -(time_step * _acceleration * 0.1), 0);
				
				pXent_setangvelocity(ranger->traction, vector(maxv(abs(vSpeed.x), ranger->speed * 15) * ranger->direction, 0, 0));
			}
			
			
			if(!key_shift)
				snd_tune(sndEngine, 50, 50 + maxv(vSpeed.x / 30 ,ranger->speed * 0.5), 0);
			else
				snd_tune(sndEngine, 50, 50 + maxv(vSpeed.x / 30 ,ranger->speed * 5), 0);
			
			if(key_force.x)
				pXent_addtorquelocal(ranger->chassis, vector(key_force.x * time_step * -13, 0, 0));
		}
		else
		{
			break;
		}
		
		pXent_getvelocity(ranger->chassis, &vSpeed, nullvector);
		
		_arcNew = minv(60 + vec_length(&vSpeed) / 50, 100);
		camera->arc -= (camera->arc - _arcNew) * time_step * 0.1;
		
		camera->z = ranger->chassis->z;
		camera->y = ranger->chassis->y;
		
		
		ranger->inContact = maxv(ranger->inContact - time_step, 0);
		
		wait(1);
	}
}

var ButtonExit ()
{
	sys_exit(NULL);
}

var ButtonPlay ()
{
	reset(panTitle, SHOW);
	reset(panMain, SHOW);
	
	mouse_mode = 0;
	
	RaceStart ();
}

var buttonActive = 0;

var ButtonOver(var _button, PANEL *_panel)
{
	if(buttonActive == _button) return;
	buttonActive = _button;
	
	snd_play(wavButtonOver, 50, 0);
	
	var timer = 4;
	while(timer > 0)
	{
		var offset = random(timer)-(timer/2);
		if(_button == MAIN_PLAY)
			pan_setbutton(_panel, 1, 0, MAIN_PLAY_POS + offset,   0, bmpPlayOn,   bmpPlayOff,   bmpPlayOn,   bmpPlayOn,   ButtonPlay, NULL, ButtonOver);
//		if(_button == MAIN_EDITOR)
//			pan_setbutton(_panel, 2, 0, MAIN_EDITOR_POS + offset, 0, bmpEditorOn, bmpEditorOff, bmpEditorOn, bmpEditorOn, NULL,       NULL, ButtonOver);
		if(_button == MAIN_EXIT)
			pan_setbutton(_panel, 3, 0, MAIN_EXIT_POS + offset,   0, bmpExitOn,   bmpExitOff,   bmpExitOn,   bmpExitOn,   ButtonExit, NULL, ButtonOver);
			
		timer -= time_step;
		wait(1);
	}
	
	if(_button == 1)
		pan_setbutton(_panel, 1, 0, MAIN_PLAY_POS,   0, bmpPlayOn,   bmpPlayOff,   bmpPlayOn,   bmpPlayOn,   ButtonPlay, NULL, ButtonOver);
//	if(_button == 2)
//		pan_setbutton(_panel, 2, 0, MAIN_EDITOR_POS, 0, bmpEditorOn, bmpEditorOff, bmpEditorOn, bmpEditorOn, NULL,       NULL, ButtonOver);
	if(_button == 3)
		pan_setbutton(_panel, 3, 0, MAIN_EXIT_POS,   0, bmpExitOn,   bmpExitOff,   bmpExitOn,   bmpExitOn,   ButtonExit, NULL, ButtonOver);
	
	if(buttonActive == _button)
		buttonActive = 0;
	
}

/*******************************************************************************************/

void MainMenu()
{
	if(snd_playing(sndEngine))
		snd_stop(sndEngine);
	sndEngine = NULL;
	
	reset(panLevel, SHOW);
	
	level_load("");
	
	on_esc = NULL;
	
	// Background setup
	vec_set(&sky_color, vector(67, 127, 207));
	
	// Camera setup
	camera->x = 500;
	camera->y = 0;
	camera->z = 0;
	camera->pan = 180;
	set(camera, ISOMETRIC);
	camera->arc = (1024 / screen_size.x) * 50;
	
	// Create background
	you = ent_createlayer("resources\\models\\plane.mdl", SKY | SCENE, 1);
	you->material = mtlPerpective;
	
	// Show title
	panTitle->pos_x = (screen_size.x / 2) - 256;
	panTitle->pos_y = (screen_size.y / 2) - 134;
	set(panTitle, SHOW);
	
	// Show main buttons
	panMain->pos_x = (screen_size.x / 2) - 256;
	panMain->pos_y = (screen_size.y / 2) + 16;
	set(panMain, SHOW);
	
	// Set mouse over the play button
	SetCursorPos(window_pos.x + panMain->pos_x + 85, window_pos.y + panMain->pos_y + 25);
	mouse_mode = 4;
	mouse_map = bmpArrow;
}

/*******************************************************************************************/

void* NxPhysicsSDK = NULL;

void LevelLoad()
{
	if(!NxPhysicsSDK) 
		return;
	
	sun_angle.pan = 0;
	sun_angle.tilt = 90;
	
	sun_color.blue = 100;
	sun_color.green = 185;
	sun_color.red = 255;
	
	reset(camera, ISOMETRIC);
	camera->x = 2000;
	camera->pan = 180;
	
	if(level_ent)
	{
		c_setminmax(level_ent);
		pXent_settype(level_ent, PH_STATIC, PH_POLY);
//		pXent_setgroup(level_ent, 2)
		level_ent->material = mtlTriplanar;
	}
	
	ENTITY *entStart = NULL;
	entGoal = NULL;
	goalCount = 0;
	
	for (you = ent_next(NULL); you; you = ent_next(you))
	{
		if(you->flags & PASSABLE) 
			continue;
		
		var type = ent_type(you);
		
		if(type < 2 && type > 5)  // blocks, models, or terrain only 
			continue;
		
		if(type == 5) // model
		{
			if(you->emask & DYNAMIC) 
				continue;	// only register static models)
			
			if(str_stri(you->link.name, "start_mdl"))
			{
				if(entStart)
				{
					printf("ERROR\nMore than one start point!");
					
					MainMenu();
					return;
				}
				
				entStart = you;
				continue;
			}
			else if(str_stri(you->link.name, "end_mdl"))
			{
				goalCount += 1;
				you->parent = entGoal;
				entGoal = you;
			}
		}
		
		if(type == 4)
			pXent_settype(you, PH_STATIC, PH_TERRAIN);
		else	
			pXent_settype(you, PH_STATIC, PH_POLY);
	}
	
	if(entStart)
	{
		RangerCreate(vector(0, entStart->y, entStart->z));
		ent_remove(entStart);
		
		ENTITY *_ent = entGoal;
		for(; _ent; _ent = _ent->parent)
		{
			pXent_setcollisionflag(ranger->chassis,  _ent, NX_NOTIFY_ON_START_TOUCH);
			pXent_setcollisionflag(ranger->wheel[0], _ent, NX_NOTIFY_ON_START_TOUCH);
			pXent_setcollisionflag(ranger->wheel[1], _ent, NX_NOTIFY_ON_START_TOUCH);
			pXent_setcollisionflag(ranger->antenna[0], _ent, NX_NOTIFY_ON_START_TOUCH);
			pXent_setcollisionflag(ranger->antenna[1], _ent, NX_NOTIFY_ON_START_TOUCH);
			pXent_setcollisionflag(ranger->antenna[2], _ent, NX_NOTIFY_ON_START_TOUCH);
			
			pXent_setgroup(_ent, GROUP_GOAL);
			_ent->skill99 = 1;
		}
		
		on_esc = MainMenu;
	}
}

void EntityRemove(ENTITY* ent)
{
	if(!NxPhysicsSDK) 
		return;
	if(ent->body) 
		pXent_settype(ent, 0, 0);		
}

/*******************************************************************************************/

void WindowClose()
{
	// Close PhysX
	physX_destroy();
	NxPhysicsSDK = NULL;
}

void WindowInit ()
{
	d3d_antialias = 9;
	
	// Engine setup
	mouse_pointer = 0;
	
	// Window setup
	video_window(NULL, NULL, 1, "Mars Ranger");
	video_set(sys_metrics(0), sys_metrics(1), 32, 2);
	
	// Load PhysX
	NxPhysicsSDK = physX_load();
	physX_run(0);
	
	// PhysX setup
	pX_setsteprate(120, 128, NX_TIMESTEP_FIXED);
	pX_setgravity(vector(0, 0, -9.8));
	pX_setunit(0.01);
	pX_setccd(1);
	
	// List of levels
	txtLevels->skill_y = txt_for_dir(txtLevels, "resources\\*.wmb");
	
	// Events
	on_exit = WindowClose;
	on_level_load = LevelLoad;
	on_ent_remove = EntityRemove;
	
	// Material of blocks
	mtl_shaded = mtlTriplanar;
}

/*******************************************************************************************/

void Intro()
{
	level_load("");
	
	on_esc = NULL;
	
	// Background setup
	vec_set(&sky_color, vector(67, 127, 207));
	
	// Camera setup
	camera->x = 500;
	camera->y = 0;
	camera->z = 0;
	camera->pan = 180;
	set(camera, ISOMETRIC);
	camera->arc = (1024 / screen_size.x) * 50;
	
	// Create background
	you = ent_createlayer("resources\\models\\plane.mdl", SKY | SCENE, 1);
	you->material = mtlPerpective;
	
	// Show title
	panTitle->pos_x = (screen_size.x / 2) - 256;
	panTitle->pos_y = (screen_size.y / 2) - 134;
	set(panTitle, SHOW);
	
	// Play sounds
	var sndIntro = snd_play(wavIntro, 50, 0);
	sndAmbient = snd_loop(wavAmbient, 50, 0);
	
	// Fade in
	var translucency = 100;
	while(translucency > 0)
	{
		draw_quad(NULL, nullvector, NULL, &screen_size, NULL, nullvector, translucency, 0);
		translucency -= time_step * 5;
		wait (1);
	}
	
	// Wait until intro sound ends
	while(snd_playing(sndIntro)) 
		wait(1);
	
	// Show main buttons
	panMain->pos_x = (screen_size.x / 2) - 256;
	panMain->pos_y = (screen_size.y / 2) + 16;
	set(panMain, SHOW);
	
	// Set mouse over the play button
	SetCursorPos(window_pos.x + panMain->pos_x + 85, window_pos.y + panMain->pos_y + 25);
	mouse_mode = 4;
	mouse_map = bmpArrow;
}

/*******************************************************************************************/

void main ()
{
	WindowInit();
	Intro();
}

