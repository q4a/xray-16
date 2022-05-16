#include "common.h"
#include "shared\cloudconfig.h"
#include "iostructs\v_clouds.h"

vf _main (vi v)
{
	vf 		o;

	o.hpos 		= mul		(m_WVP, v.p);	// xform, input in world coords
	o.color		= v.color;			// copy color
	
	o.color.w	*= pow		(v.p.y,25);
	
	//if (length(float3(v.p.x,0,v.p.z))>CLOUD_FADE)	o.color.w = 0	;

	// generate tcs
	float2 d0	= v.dir.xy*2-1;
	float2 d1	= v.dir.wz*2-1;
	o.tc0		= v.p.xz * CLOUD_TILE0 + d0*timers.z*CLOUD_SPEED0;
	o.tc1		= v.p.xz * CLOUD_TILE1 + d1*timers.z*CLOUD_SPEED1;

	return o;
}
