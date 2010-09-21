/*
Copyright (C) 2010 COR Entertainment, LLC.

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

#include <ode/ode.h>

#define MAX_RAGDOLLS 64 
#define MAX_RAGDOLL_OBJECTS 16
#define MAX_RAGDOLL_JOINTS 16
#define MAX_SURFACES 128 //temporary val, just for basic testing, likely we need to do some proximity culling
#define MAX_CONTACTS 4 //this might be best set to 1, check
#define RAGDOLL_DURATION 10000 //10 seconds

//body id's
#define CHEST 0
#define BELLY 1
#define PELVIS 2
#define HEAD 3
#define RIGHTUPPERLEG 4
#define LEFTUPPERLEG 5
#define RIGHTLOWERLEG 6
#define LEFTLOWERLEG 7
#define RIGHTFOOT 8
#define LEFTFOOT 9
#define RIGHTUPPERARM 10
#define LEFTUPPERARM 11
#define RIGHTFOREARM 12
#define LEFTFOREARM 13
#define RIGHTHAND 14
#define LEFTHAND 15

//joint id's
#define MIDSPINE 0
#define LOWSPINE 1
#define NECK 2
#define RIGHTHIP 3
#define LEFTHIP 4
#define RIGHTKNEE 5
#define LEFTKNEE 6
#define RIGHTANKLE 7
#define LEFTANKLE 8
#define RIGHTSHOULDER 9
#define LEFTSHOULDER 10
#define RIGHTELBOW 11
#define LEFTELBOW 12
#define RIGHTWRIST 13
#define LEFTWRIST 14

//Hard coded definitions(if no ragdoll read in - note - these need to be adjusted to actual model sizes)
#define HEAD_H 0.20
#define UPPER_ARM_LEN 0.30
#define FORE_ARM_LEN 0.25
#define HAND_LEN 0.13 // wrist to mid-fingers only
#define FOOT_LEN 0.18 // ankles to base of ball of foot only
#define HEEL_LEN 0.05 

#define NECK_H 1.50
#define SHOULDER_H 1.37
#define CHEST_H 1.35
#define HIP_H 0.86
#define KNEE_H 0.48
#define ANKLE_H 0.08

#define SHOULDER_W 0.41
#define CHEST_W 0.36 // actually wider, but we want narrower than shoulders (esp. with large radius)
#define LEG_W 0.28 // between middles of upper legs
#define PELVIS_W 0.25 // actually wider, but we want smaller than hip width

dWorldID RagDollWorld;
dSpaceID RagDollSpace;

dJointGroupID contactGroup;

int lastODEUpdate;

typedef struct RagDollObject_s {
	dBodyID body;
	dMass mass;
	dGeomID geom;
} RagDollObject_t;

typedef struct RagDoll_s {

	RagDollObject_t RagDollObject[MAX_RAGDOLL_OBJECTS];
	dJointID RagDollJoint[MAX_RAGDOLL_JOINTS];

	//Ragdoll  positions
	vec3_t R_SHOULDER_POS; 
	vec3_t L_SHOULDER_POS;
	vec3_t R_ELBOW_POS;
	vec3_t L_ELBOW_POS;
	vec3_t R_WRIST_POS;
	vec3_t L_WRIST_POS;
	vec3_t R_FINGERS_POS;
	vec3_t L_FINGERS_POS;

	vec3_t R_HIP_POS; 
	vec3_t L_HIP_POS;
	vec3_t R_KNEE_POS; 
	vec3_t L_KNEE_POS; 
	vec3_t R_ANKLE_POS; 
	vec3_t L_ANKLE_POS;
	vec3_t R_HEEL_POS;
	vec3_t L_HEEL_POS;
	vec3_t R_TOES_POS;
	vec3_t L_TOES_POS;

	//mesh information
	model_t *ragDollMesh;
	vec3_t	origin;

	//surface for ragdoll to collide
	dGeomID WorldGeometry[MAX_SURFACES];

	float spawnTime;

	int destroyed;

} RagDoll_t;

RagDoll_t RagDoll[MAX_RAGDOLLS]; 

//Funcs
extern void R_CreateWorldObject( void );
extern void R_DestroyWorldObject( void );
extern void R_RenderAllRagdolls ( void );
extern void R_AddNewRagdoll( vec3_t origin );
extern void R_ClearAllRagdolls( void );
