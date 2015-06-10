/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "NanoProjectile.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CNanoProjectile, CProjectile, )

CR_REG_METADATA(CNanoProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(creationTime),
		CR_MEMBER(lifeTime),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
))



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNanoProjectile::CNanoProjectile(): CProjectile()
{
	creationTime = lifeTime = 0;
	color[0] = color[1] = color[2] = color[3] = 255;

	checkCol = false;
	drawSorted = false;
}

CNanoProjectile::CNanoProjectile(const float3& pos, const float3& speed, int lifeTime, const float3& color):
	CProjectile(pos, speed, NULL, false, false, false),
	creationTime(gs->frameNum),
	lifeTime(lifeTime)
{
	checkCol = false;
	drawSorted = false;

	this->color[0] = (unsigned char) (color[0] * 255);
	this->color[1] = (unsigned char) (color[1] * 255);
	this->color[2] = (unsigned char) (color[2] * 255);
	this->color[3] = 20;
	drawRadius = 3;

	if (projectileHandler != NULL) {
		projectileHandler->currentNanoParticles += 1;
	}
}

CNanoProjectile::~CNanoProjectile()
{
	if (projectileHandler != NULL) {
		projectileHandler->currentNanoParticles -= 1;
	}
}


void CNanoProjectile::Update()
{
	pos += speed;
	if (gs->frameNum >= creationTime + lifeTime) {
		deleteMe = true;
	}
}

void CNanoProjectile::Draw()
{
	inArray = true;

	#define gfxt projectileDrawer->gfxtex
	va->AddVertexTC(drawPos - camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, gfxt->xstart, gfxt->ystart, color);
	va->AddVertexTC(drawPos + camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, gfxt->xend,   gfxt->ystart, color);
	va->AddVertexTC(drawPos + camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, gfxt->xend,   gfxt->yend,   color);
	va->AddVertexTC(drawPos - camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, gfxt->xstart, gfxt->yend,   color);
	#undef gfxt
}

void CNanoProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::green);
}

int CNanoProjectile::GetProjectilesCount() const
{
	return 0; // nano particles use their own counter
}
