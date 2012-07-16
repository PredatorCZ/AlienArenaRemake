// trace.c

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "qrad.h"
#include <assert.h>

#define	ON_EPSILON	0.1

typedef struct tnode_s
{
	int		type;
	vec3_t	normal;
	float	dist;
	int		children[2];
	int		children_leaf[2]; //valid if the corresponding child is a leaf
    int     pad;
} tnode_t;

tnode_t		*tnodes, *tnode_p;

/*
==============
MakeTnode

Converts the disk node structure into the efficient tracing structure
==============
*/
static int tnode_mask;
void MakeTnode (int nodenum)
{
	tnode_t			*t;
	dplane_t		*plane;
	dface_t			*face;
	int				i, j;
	dnode_t 		*node;
	
	t = tnode_p++;

	node = dnodes + nodenum;
	plane = dplanes + node->planenum;

	t->type = plane->type;
	VectorCopy (plane->normal, t->normal);
	t->dist = plane->dist;
	
	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i] < 0)
		{
            t->children[i] = (dleafs[-node->children[i] - 1].contents & tnode_mask) | (1<<31);
        	t->children_leaf[i] = -node->children[i] - 1;
		}
		else
		{
			t->children[i] = tnode_p - tnodes;
			MakeTnode (node->children[i]);
		}
	}
}


/*
=============
MakeTnodes

Loads the node structure out of a .bsp file to be used for light occlusion
=============
*/
void MakeTnodes (dmodel_t *bm)
{
	// 32 byte align the structs
	tnodes = malloc( (numnodes+1) * sizeof(tnode_t));
	tnodes = (tnode_t *)(((int)tnodes + 31)&~31);
	tnode_p = tnodes;
	tnode_mask = CONTENTS_SOLID|CONTENTS_WINDOW;
	//TODO: or-in CONTENTS_WINDOW in response to a command-line argument

	MakeTnode (0);
}

//==========================================================

/*
=============
BuildPolygons

Applies the same basic algorithm as r_surf.c:BSP_BuildPolygonFromSurface to
every surface in the BSP, precomputing the xyz and st coordinates of each 
vertex of each polygon. Skip non-translucent surfaces for now.
=============
*/
#if 0
typedef struct {
	float	point[3];
	int		st[2];
} tvtex_t;

typedef struct {
	int		numverts;
	tvtex_t	*vtexes;
} tpoly_t;

tpoly_t	tpolys[MAX_MAP_FACES];

void BuildPolygon (int facenum)
{
	dface_t		*face;
	tpoly_t		*poly;
	dedge_t		*edge;
	texinfo_t	*tex;
	float 	*vec;
	int		numverts, i, edgenum;
	
	face = &dfaces[facenum];
	numverts = face->numedges;
	poly = &tpolys[facenum];
	poly->numverts = numverts;
	poly->vtexes = malloc (sizeof(tvtex_t)*numverts);
	tex = &texinfo[face->texinfo];
	
	for (i = 0; i < numverts; i++)
	{
		edgenum = face->firstedge;
		if (edgenum > 0)
		{
			edge = &dedges[edgenum];
			vec = dvertexes[edge->v[0]].point;
		}
		else
		{
			edge = &dedges[-edgenum];
			vec = dvertexes[edge->v[1]].point;
		}
		
		poly->vtexes[i].st[0] = 
			(DotProduct (vec, tex->vecs[0]) + tex->vecs[0][3])/
			texture_sizes[face->texinfo][0];
		poly->vtexes[i].st[1] = 
			(DotProduct (vec, tex->vecs[1]) + tex->vecs[1][3])/
			texture_sizes[face->texinfo][1];
		
		VectorCopy (vec, poly->vtexes[i].point);
	}
}

void BuildPolygons (void)
{
	int i;
	for (i = 0; i < numfaces; i++)
		BuildPolygon(i);
}
#endif

/*
================
PointInBrush

Adapted from CM_ClipBoxToBrush
================
*/
int PointInBrush (vec3_t p1, vec3_t p2, vec3_t pt,
					  dbrush_t *brush, float *fraction, int curface)
{
	int			i;
	dplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	float		d1, d2;
	qboolean	startout;
	float		f;
	dbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return curface;

	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &dbrushsides[brush->firstside+i];
		plane = &dplanes[side->planenum];

		// FIXME: special case for axial

		dist = plane->dist;

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d1 > 0)
			startout = true; //startpoint is not in solid

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return curface;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-ON_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+ON_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}
	
	if (!startout)
	{	// original point was inside brush
		return curface;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < *fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			for (i = 0; i < 3; i++) 
				pt[i] = p1[i] + (p2[i] - p1[i])*enterfrac;
			*fraction = enterfrac;
			return leadside->texinfo;
		}
	}
	return curface;
}

/*
=============
GetNodeFace

Adapted from CM_TraceToLeaf
=============
*/
int GetNodeFace (int leafnum, vec3_t start, vec3_t end, vec3_t pt)
{
	int			k;
	int			brushnum;
	dleaf_t		*leaf;
	dbrush_t	*b;
	float 		fraction;
	int 		curface;

	leaf = &dleafs[leafnum];
	fraction = 1.1;
	curface = dfaces[leaf->firstleafface].texinfo;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = dleafbrushes[leaf->firstleafbrush+k];
		b = &dbrushes[brushnum];
		curface = PointInBrush (start, end, pt, b, &fraction, curface);
		if (!fraction)
			return curface;
	}
	return curface;
}

/*
=============
GetRGBASample

Gets the RGBA value within the texture of the polygon of a 3D point on the 
polygon.
=============
*/
inline float *GetRGBASample (int node_leaf, vec3_t orig_start, vec3_t orig_stop)
{
	int 		s, t, smax, tmax;
	int			texnum;
	vec3_t		point;
	texinfo_t 	*tex;
	float		*res;
	
	texnum = GetNodeFace (node_leaf, orig_start, orig_stop, point);
	tex = &texinfo[texnum];
	
	smax = texture_sizes[texnum][0];
	tmax = texture_sizes[texnum][1];
	
	s = DotProduct (point, tex->vecs[0]) + tex->vecs[0][3];
	while (s < 0)
		s += smax;
	s %= smax;
	
	t = DotProduct (point, tex->vecs[1]) + tex->vecs[1][3];
	while (t < 0)
		t += tmax;
	t %= tmax;

	
	res = texture_data[texnum]+(t*smax+s)*4;
	return res;
}

//==========================================================

//with texture checking
int TestLine_r_texcheck (int node, int node_leaf, vec3_t orig_start, vec3_t orig_stop, vec3_t set_start, vec3_t stop)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid, _start;
	vec_t *start;
	float	frac;
	int		side;
	int		r;

    start = set_start;

re_test:

	r = 0;
	if (node & (1<<31))
	{
		float *rgba_sample;
		if ((r = node & ~(1<<31)) != CONTENTS_WINDOW)
		{
			return r;
		}
		//translucent, so check texture
		rgba_sample = GetRGBASample (node_leaf, orig_start, orig_stop);
		if (rgba_sample[3] > 0.8)
			return 1;
		return 0;
		return (rgba_sample[3] > 0.8); //TODO more sophisticated alpha checking
		/*TODO: open the texture up, check the transparency.
        We should have a vec3_t output argument to this function so we
        can indicate how much of each color is occluded. Here's what I 
        propose:
            float rgba_sample[4]; //range 0..1 on each axis
            get_rgba_sample(node_face, mid, rgba_sample);
            for (i = 0; i < 3; i++) {
                occlusion[i] *= rgba_sample[i]*rgba_sample[3];
                if (occlusion[i] < 0.01)
                    occlusion[i] = 0.0;
            }
            if (VectorLength(occlusion) < 0.1)
                return 1; //occluded
            return 0; //not occluded
        Note that occlusion would start out as {1.0,1.0,1.0} and have each
        color reduced as the trace passed through more translucent 
        textures.
        */
	}

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}
	
	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
	{
        node = tnode->children[0];
        node_leaf = tnode->children_leaf[0];
        goto re_test;
	}
	
	if (front < ON_EPSILON && back < ON_EPSILON)
	{
        node = tnode->children[1];
        node_leaf = tnode->children_leaf[1];
        goto re_test;
	}

	side = front < 0;

    frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;


    if (r = TestLine_r_texcheck (tnode->children[side], tnode->children_leaf[side], orig_start, orig_stop, start, mid))
		return r;

    node = tnode->children[!side];
    node_leaf = tnode->children_leaf[!side];

    start = _start;
    start[0] = mid[0];
    start[1] = mid[1];
    start[2] = mid[2];

    goto re_test;
}

//without texture checking
int TestLine_r (int node, vec3_t set_start, vec3_t stop)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid, _start;
	vec_t *start;
	float	frac;
	int		side;
	int		r;

    start = set_start;

re_test:

	r = 0;
	if (node & (1<<31))
	{
		if ((r = node & ~(1<<31)) != CONTENTS_WINDOW)
		{
			return r;
		}
		return 1;
	}

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}
	
	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
	{
        node = tnode->children[0];
        goto re_test;
	}
	
	if (front < ON_EPSILON && back < ON_EPSILON)
	{
        node = tnode->children[1];
        goto re_test;
	}

	side = front < 0;

    frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;


    if (r = TestLine_r (tnode->children[side], start, mid))
		return r;

    node = tnode->children[!side];

    start = _start;
    start[0] = mid[0];
    start[1] = mid[1];
    start[2] = mid[2];

    goto re_test;
}


int TestLine (vec3_t start, vec3_t stop)
{
	if (doing_texcheck)
		return TestLine_r_texcheck (0, 0, start, stop, start, stop);
	else
		return TestLine_r (0, start, stop);
}

/*
==============================================================================

LINE TRACING

The major lighting operation is a point to point visibility test, performed
by recursive subdivision of the line by the BSP tree.

==============================================================================
*/

typedef struct
{
	vec3_t	backpt;
	int		side;
	int		node;
} tracestack_t;


/*
==============
TestLine
==============
*/
qboolean _TestLine (vec3_t start, vec3_t stop)
{
	int				node;
	float			front, back;
	tracestack_t	*tstack_p;
	int				side;
	float 			frontx,fronty, frontz, backx, backy, backz;
	tracestack_t	tracestack[64];
	tnode_t			*tnode;
	
	frontx = start[0];
	fronty = start[1];
	frontz = start[2];
	backx = stop[0];
	backy = stop[1];
	backz = stop[2];
	
	tstack_p = tracestack;
	node = 0;
	
	while (1)
	{
		if (node == CONTENTS_SOLID)
		{
#if 0
			float	d1, d2, d3;

			d1 = backx - frontx;
			d2 = backy - fronty;
			d3 = backz - frontz;

			if (d1*d1 + d2*d2 + d3*d3 > 1)
#endif
				return false;	// DONE!
		}
		
		while (node < 0)
		{
		// pop up the stack for a back side
			tstack_p--;
			if (tstack_p < tracestack)
				return true;
			node = tstack_p->node;
			
		// set the hit point for this plane
			
			frontx = backx;
			fronty = backy;
			frontz = backz;
			
		// go down the back side

			backx = tstack_p->backpt[0];
			backy = tstack_p->backpt[1];
			backz = tstack_p->backpt[2];
			
			node = tnodes[tstack_p->node].children[!tstack_p->side];
		}

		tnode = &tnodes[node];
		
		switch (tnode->type)
		{
		case PLANE_X:
			front = frontx - tnode->dist;
			back = backx - tnode->dist;
			break;
		case PLANE_Y:
			front = fronty - tnode->dist;
			back = backy - tnode->dist;
			break;
		case PLANE_Z:
			front = frontz - tnode->dist;
			back = backz - tnode->dist;
			break;
		default:
			front = (frontx*tnode->normal[0] + fronty*tnode->normal[1] + frontz*tnode->normal[2]) - tnode->dist;
			back = (backx*tnode->normal[0] + backy*tnode->normal[1] + backz*tnode->normal[2]) - tnode->dist;
			break;
		}

		if (front > -ON_EPSILON && back > -ON_EPSILON)
//		if (front > 0 && back > 0)
		{
			node = tnode->children[0];
			continue;
		}
		
		if (front < ON_EPSILON && back < ON_EPSILON)
//		if (front <= 0 && back <= 0)
		{
			node = tnode->children[1];
			continue;
		}

		side = front < 0;
		
		front = front / (front-back);
	
		tstack_p->node = node;
		tstack_p->side = side;
		tstack_p->backpt[0] = backx;
		tstack_p->backpt[1] = backy;
		tstack_p->backpt[2] = backz;
		
		tstack_p++;
		
		backx = frontx + front*(backx-frontx);
		backy = fronty + front*(backy-fronty);
		backz = frontz + front*(backz-frontz);
		
		node = tnode->children[side];		
	}	
}
