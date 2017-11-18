/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// client.h -- primary header for client

//define	PARANOID			// speed sapping error checking

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref.h"
#include "ref_gl/r_text.h"

#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "input.h"
#include "keys.h"
#include "console.h"

//=============================================================================

typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;

typedef struct
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	int			trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	int			fly_stoptime;

	float		frametime;
	int			prevframe;
	
	particle_t  *pr;                // for chained particles (such as EF_SHIPEXHAUST)

} centity_t;

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep

typedef struct
{
	char	name[MAX_QPATH];
	char	cinfo[MAX_QPATH];
	struct image_s	*skin;
	struct image_s	*icon;
	char	iconname[MAX_QPATH];
	struct model_s	*model;
	struct model_s	*weaponmodel[MAX_CLIENTWEAPONMODELS];
	struct model_s  *helmet;
	struct model_s  *lod1;
	struct model_s  *lod2;
} clientinfo_t;

typedef struct
{
	int g_NumWins;
	int g_NumKills;
	int g_NumHeadShots;
	int g_NumBaseKills;
	int g_NumFlagCaptures;
	int g_NumGames;
	int g_NumGodLikes;
	int g_NumKillStreaks;
	int g_NumRampages;
	int g_NumUnstoppables;
	int g_NumFlagReturns;
	int g_NumMindErases;
	int g_NumDisintegrates;
	int g_NumViolations;
	int g_NumMidAirShots;
} steamstats_t;

extern steamstats_t stStats;

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

extern char		scr_playericon[MAX_OSPATH];
extern char		scr_playername[PLAYERNAME_SIZE];
extern float	scr_playericonalpha;

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct
{
	int			timeoutcount;

	int			timedemo_frames;
	int			timedemo_start;

	qboolean	refresh_prepped;	// false if on new level or new ref dll
	qboolean	sound_prepped;		// ambient sounds can start
	qboolean	force_refdef;		// vid has changed, so we can't use a paused refdef

	int			parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	int			predicted_origins[CMD_BACKUP][3];	// for debug comparing against server

	float		predicted_step;				// for stair up smoothing
	unsigned	predicted_step_time;

	vec3_t		predicted_origin;	// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		last_predicted_angles;
	vec3_t		predicted_velocity;	// for speedometer
	vec3_t		prediction_error;

	frame_t		frame;				// received from server
	int			surpressCount;		// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			time;			// this is the time value that the client
								// is rendering at.  always <= cls.realtime
	float		lerpfrac;		// between oldframe and frame

	refdef_t	refdef;

	vec3_t		v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char		layout[1024];		// general 2D overlay
	int			inventory[MAX_ITEMS];

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;
	qboolean	tactical; //used for certain client needs

	// Alien Arena client/server protocol depends on MAX_QPATH being 64.
	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	struct model_s	*model_draw[MAX_MODELS];
	struct cmodel_s	*model_clip[MAX_MODELS];

	struct sfx_s	*sound_precache[MAX_SOUNDS];
	struct image_s	*image_precache[MAX_IMAGES];

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;
} client_state_t;

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
} connstate_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type

typedef enum {key_game, key_console, key_message, key_menu} keydest_t;

typedef struct
{
	connstate_t	state;
	keydest_t	key_dest;

	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametime;			// seconds since last frame

// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information

	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;

	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;
	char		downloadurl[MAX_OSPATH];  // for http downloads
	qboolean	downloadhttp;
	qboolean	downloadfromcommand;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	FILE		*demofile;
} client_static_t;

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	cvar_t	*cl_stereo_separation;
extern	cvar_t	*cl_stereo;
extern	cvar_t	*cl_maxfps;

extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_add_blend;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_noskins;
extern	cvar_t	*cl_autoskins;
extern  cvar_t	*cl_noblood;
extern  cvar_t	*cl_disbeamclr;
extern  cvar_t	*cl_dmlights;
extern  cvar_t	*cl_brass;
extern  cvar_t	*cl_playtaunts;
extern	cvar_t	*cl_centerprint;
extern	cvar_t	*cl_precachecustom;
extern  cvar_t	*cl_simpleitems;
extern	cvar_t	*cl_flicker;

extern	cvar_t	*cl_paindist;
extern	cvar_t	*cl_explosiondist;
extern  cvar_t	*cl_raindist;

extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;

extern	cvar_t	*lookspring;
extern	cvar_t	*lookstrafe;
extern  cvar_t	*m_accel;
extern	cvar_t	*sensitivity;
extern  cvar_t  *menu_sensitivity;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;

extern	cvar_t	*freelook;

extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;

extern	cvar_t	*cl_vwep;

extern  cvar_t  *background_music;

extern  cvar_t	*cl_IRC_connect_at_startup;
extern	cvar_t	*cl_IRC_server;
extern	cvar_t	*cl_IRC_channel;
extern	cvar_t	*cl_IRC_port;
extern	cvar_t	*cl_IRC_override_nickname;
extern	cvar_t	*cl_IRC_nickname;
extern	cvar_t	*cl_IRC_kick_rejoin;
extern	cvar_t	*cl_IRC_reconnect_delay;

extern	cvar_t	*cl_latest_game_version;

extern	cvar_t	*cl_test;

typedef struct
{
	int		key;				// so entities can reuse same entry
	vec3_t	color;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
} cdlight_t;

extern	centity_t	cl_entities[MAX_EDICTS];
extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define	MAX_PARSE_ENTITIES	1024
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;

qboolean	CL_CheckOrDownloadFile (char *filename);

void CL_AddNetgraph (void);

//ROGUE
typedef struct cl_sustain
{
	int			id;
	int			type;
	int			endtime;
	int			nextthink;
	int			thinkinterval;
	vec3_t		org;
	vec3_t		dir;
	int			color;
	int			count;
	int			magnitude;
	void		(*think)(struct cl_sustain *self);
} cl_sustain_t;

#define MAX_SUSTAINS		32

//=================================================

#define	PARTICLE_GRAVITY	40
#define BLASTER_PARTICLE_COLOR		0xe0
// PMM
#define INSTANT_PARTICLE	-10000.0
// PGM
// ========

// Berserker client entities code
#define     MAX_CLENTITIES     256
#define CLM_BOUNCE		1
#define CLM_FRICTION	2
#define CLM_ROTATE		4
#define CLM_NOSHADOW	8
#define CLM_STOPPED		16
#define CLM_BRASS		32
#define CLM_GLASS		64

typedef struct clentity_s {
	struct clentity_s *next;
	float time;
	vec3_t org;
	vec3_t lastOrg;
	vec3_t vel;
	vec3_t accel;
	struct model_s *model;
	float ang;
	float avel;
	float alpha;
	float alphavel;
	int flags;

} clentity_t;

void CL_AddClEntities(void);
void CL_ClearClEntities(void);
void CL_ClearEffects (void);
void CL_ClearTEnts (void);

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits);
void CL_ParseFrame (void);

void CL_ParseTEnt (void);
void CL_ParseConfigString (void);
void CL_ParseMuzzleFlash (void);
void CL_ParseMuzzleFlash2 (void);
void SmokeAndFlash(vec3_t origin);

void CL_SetLightstyle (int i);

void CL_RunParticles (void);
void CL_RunDLights (void);
void CL_RunLightStyles (void);

void CL_AddEntities (void);
void CL_AddDLights (void);
void CL_AddTEnts (void);
void CL_AddLightStyles (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);
void CL_RegisterModels (void);

void CL_Quit_f (void);

void IN_Accumulate (void);

void CL_ParseLayout (void);

//
// cl_main
//
extern qboolean send_packet_now;
extern int server_tickrate;
void CL_Init (void);

void CL_FixUpGender(void);
void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_Precache_f (void);
void CL_RequestNextDownload (void);

//
// cl_input
//
typedef struct
{
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd, qboolean reset_accum);

void IN_CenterView (void);

char *Key_KeynumToString (int keynum);

extern qboolean mouse_available;

void IN_MLookDown (void);
void IN_MLookUp (void);

//cl_irc.c
void CL_IRCSetup(void);
void CL_InitIRC(void);
void CL_IRCInitiateShutdown(void);
void CL_IRCWaitShutdown(void);
void CL_IRCSay(void);
qboolean CL_IRCIsConnected(void);
qboolean CL_IRCIsRunning(void);

//
// cl_demo.c
//
void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern	char *svc_strings[256];
extern  void Q_strncpyz( char *dest, const char *src, size_t size );

void CL_ParseServerMessage (void);
void CL_LoadClientinfo (clientinfo_t *ci, char *s);
void SHOWNET(char *s);
void CL_ParseClientinfo (int player);
void CL_DownloadFileName (char *dest, int destlen, char *fn);
void CL_DownloadComplete (void);
void CL_Download_f (void);

//
// cl_scrn.c
//
void SCR_IRCPrintf (char *fmt, ...);

//
// cl_stats.c
//

typedef struct _PLAYERSTATS {
	char playername[PLAYERNAME_SIZE];
	int totalfrags;
	double totaltime;
	int ranking;
} PLAYERSTATS;

typedef struct _STATSLOGINSTATE {
	int requestType;
	qboolean validated;
	qboolean hashed;
	char old_password[256];
} LOGINSTATE;

#define STATSLOGIN 1
#define STATSPWCHANGE 2

void STATS_getStatsDB( void );
void STATS_RequestVerification( void );
void STATS_RequestPwChange (void);
void STATS_AuthenticateStats (char *vstring);
void STATS_ChangePassword (char *vstring);
void STATS_Logout (void);
PLAYERSTATS getPlayerRanking ( PLAYERSTATS player );
PLAYERSTATS getPlayerByRank ( int rank, PLAYERSTATS player );
LOGINSTATE currLoginState;
void STATS_ST_Init (void);
void STATS_ST_Write (void);

//
// cl_updates.c
//

void getLatestGameVersion( void );
qboolean serverIsOutdated (char *server_vstring);
char* VersionUpdateNotice( void );


//
// cl_view.c
//

#define CL_LOADMSG_LENGTH 96

qboolean loadingMessage;
char loadingMessages[5][2][CL_LOADMSG_LENGTH];
float loadingPercent;
int rocketlauncher;
int rocketlauncher_drawn;
int chaingun;
int chaingun_drawn;
int smartgun;
int smartgun_drawn;
int flamethrower;
int flamethrower_drawn;
int disruptor;
int disruptor_drawn;
int beamgun;
int beamgun_drawn;
int vaporizer;
int vaporizer_drawn;
int quad;
int quad_drawn;
int haste;
int haste_drawn;
int inv;
int inv_drawn;
int adren;
int adren_drawn;
int sproing;
int sproing_drawn;
int numitemicons;

void V_Init (void);
void V_RenderView( float stereo_separation );
void V_AddEntity (entity_t *ent);
void V_AddViewEntity (entity_t *ent);
void V_AddParticle (particle_t	*p);
void V_AddLight (vec3_t org, float intensity, float r, float g, float b);
void V_AddTeamLight (vec3_t org, float intensity, float r, float g, float b, int team);
void V_AddLightStyle (int style, float r, float g, float b);
void V_AddStain (vec3_t org, float intensity, float r, float g, float b, float a);

//
// cl_tent.c
//
void CL_RegisterTEntSounds (void);
void CL_RegisterTEntModels(void);

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_CheckPredictionError (void);
trace_t CL_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int skipNumber, int brushMask, qboolean brushOnly, int *entNumber);
trace_t CL_PMSurfaceTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentmask);

//
// cl_fx.c
//
cdlight_t *CL_AllocDlight (int key);
void CL_BigTeleportParticles (vec3_t org);
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old);
void CL_BlasterTrail (vec3_t start, vec3_t end, float color);
void CL_ShipExhaust (vec3_t start, vec3_t end, centity_t *old);
void CL_RocketExhaust (vec3_t start, vec3_t end, centity_t *old);
void CL_BeamgunMark(vec3_t org, vec3_t dir, float dur, qboolean isDis);
void CL_BulletMarks(vec3_t org, vec3_t dir);
void CL_VaporizerMarks(vec3_t org, vec3_t dir);
void CL_BlasterMark(vec3_t org, vec3_t dir);
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags);
void CL_BfgParticles (entity_t *ent);
void CL_AddParticles (void);
void CL_EntityEvent (entity_state_t *ent);
qboolean server_is_team; //used for player visibility light code
void CL_BloodEffect (vec3_t org, vec3_t dir, int color, int count);
void CL_DisruptorBeam (vec3_t start, vec3_t end);
void CL_LaserBeam (vec3_t start, vec3_t end, float color, qboolean use_start);
void CL_VaporizerBeam (vec3_t start, vec3_t end);
void CL_BubbleTrail (vec3_t start, vec3_t end);
void CL_NewLightning (vec3_t start, vec3_t end);
void CL_LightningBall (vec3_t org, vec3_t angles, float color, qboolean from_client);
void CL_BlueTeamLight(vec3_t pos);
void CL_RedTeamLight(vec3_t pos);
void CL_FlagEffects(vec3_t pos, qboolean team);
void CL_PoweredEffects (vec3_t pos, unsigned int nEffect);
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing);
void CL_SayIcon(vec3_t org);
void CL_TeleportParticles (vec3_t org);
void CL_BlasterParticles (vec3_t org, vec3_t dir);
void CL_ExplosionParticles (vec3_t org);
void CL_MuzzleParticles (vec3_t org);
void CL_BlasterMuzzleParticles (vec3_t org, vec3_t angles, float color, float alpha, qboolean from_client);
void CL_SmartMuzzle (vec3_t org, vec3_t angles, qboolean from_client);
void CL_RocketMuzzle (vec3_t org, vec3_t angles, qboolean from_client);
void CL_MEMuzzle (vec3_t org, vec3_t angles, qboolean from_client);
particle_t *CL_BlueFlameParticle (vec3_t org, vec3_t angles, particle_t *previous);
particle_t *CL_FlameThrowerParticle (vec3_t org, vec3_t dir, particle_t *previous);
void CL_Deathfield (vec3_t org, int type);
void CL_BFGExplosionParticles (vec3_t org);
void CL_DustParticles (vec3_t org);
void CL_BlueBlasterParticles (vec3_t org, vec3_t dir);
void CL_ParticleSteamEffect(cl_sustain_t *self);
void CL_ParticleFireEffect2(cl_sustain_t *self);
void CL_ParticleSmokeEffect2(cl_sustain_t *self);
void CL_ParticleDustEffect (cl_sustain_t *self);
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count);
void CL_BulletSparks ( vec3_t org, vec3_t dir);
void CL_FlameThrower (vec3_t org, vec3_t dir, qboolean from_client);
void CL_JetExhaust (vec3_t org, vec3_t dir);
void CL_SplashEffect ( vec3_t org, vec3_t dir, int color, int count, int type);
void CL_LaserSparks ( vec3_t org, vec3_t dir, int color, int count);
void CL_BrassShells(vec3_t org, vec3_t dir, int count);
void CL_MuzzleFlashParticle (vec3_t org, vec3_t angles, qboolean from_client);
void CL_PlasmaFlashParticle (vec3_t org, vec3_t angles, qboolean from_client);

//
// menus
//

#define MAX_PLAYERS 64
typedef struct _SERVERDATA {

	char szHostName[48];
	char szMapName[24], fullMapName[128];
	char szAdmin[64];
	char szVersion[32];
	char szWebsite[32];
	char maxClients[32];
	char fraglimit[32];
	char timelimit[32];
#define SVDATA_PLAYERINFO_NAME 0
#define SVDATA_PLAYERINFO_SCORE 1
#define SVDATA_PLAYERINFO_PING 2
#define SVDATA_PLAYERINFO_TEAM 3
#define SVDATA_PLAYERINFO 4
#define SVDATA_PLAYERINFO_COLSIZE 80
	char playerInfo[MAX_PLAYERS][SVDATA_PLAYERINFO][SVDATA_PLAYERINFO_COLSIZE];
	int	 playerRankings[MAX_PLAYERS];
	char skill[32];
	int players;
	char szPlayers[11];
	int ping;
	char szPing[6];
	netadr_t local_server_netadr;
	char serverInfo[256];
	char modInfo[64];
	qboolean joust; //for client-side prediction

} SERVERDATA;

SERVERDATA connectedserver;

void M_Init (void);
void M_Keydown (int key);
void M_Draw (void);
void M_Menu_Main_f (void);
void M_ForceMenuOff (void);
void M_AddToServerList (netadr_t adr, char *status_string);
void M_UpdateConnectedServerInfo (netadr_t adr, char *status_string);
//
// cl_inv.c
//
void CL_ParseInventory (void);
void CL_KeyInventory (int key);
void CL_DrawInventory (void);

//
// cl_pred.c
//
void CL_PredictMovement (int msec_since_packet);

//
// cl_view, cl_scrn
//
qboolean map_pic_loaded;
int stringLen (char *string);

//
// perf testing
// A performance test measures some form of "something per second" or "total
// accumulated something." However, "special" tests allow you to display any
// text you want.
//

typedef struct perftest_s {
    //these get set by get_perftest for you:
    qboolean    in_use;
    char        name[48];
    //set these when first setting up the perf counter:
    cvar_t      *cvar; //the cvar used for detecting whether to draw it
    char        format[32]; //format one float, never output >32 chars
    float       scale; //for scaling the rate
    qboolean    is_timerate; //true to display "something per second."
    qboolean	is_special; //if you want to update the "text" field yourself
    //update this once per frame:
    float       counter; //for whatever you're measuring
    //these are internal to SCR_showPerfTest:
    float       framecounter;
    qboolean    restart;
    int         start_msec;
    float       update_trigger;
    char        text[32];
} perftest_t;

#define MAX_PERFTESTS 16
perftest_t perftests[MAX_PERFTESTS];
perftest_t *get_perftest(char *name);

//
//mouse
//
void refreshCursorLink (void);
void M_Think_MouseCursor (void);

//
//cl_http.c
//
void CL_InitHttpDownload(void);
void CL_HttpDownloadCleanup(void);
qboolean CL_HttpDownload(void);
void CL_HttpDownloadThink(void);
void CL_ShutdownHttpDownload(void);


//
// Fonts used by various elements
//

extern FNT_auto_t	CL_gameFont;
extern FNT_auto_t	CL_menuFont;
extern FNT_auto_t	CL_centerFont;
extern FNT_auto_t	CL_consoleFont;

//external renderer
extern void R_AddSimpleItem(int type, vec3_t origin);
extern qboolean r_gotFlag;
extern qboolean r_lostFlag;
