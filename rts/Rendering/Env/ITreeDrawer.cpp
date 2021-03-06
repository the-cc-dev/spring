/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ITreeDrawer.h"
#include "NullTreeDrawer.h"
#include "AdvTreeDrawer.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"

CONFIG(int, TreeRadius)
	.defaultValue((int) (5.5f * 256))
	.headlessValue(0)
	.minimumValue(0);

ITreeDrawer* treeDrawer = nullptr;


ITreeDrawer::ITreeDrawer(): CEventClient("[ITreeDrawer]", 314444, false)
	, drawTrees(true)
	, wireFrameMode(false)
{
	eventHandler.AddClient(this);
	baseTreeDistance = configHandler->GetInt("TreeRadius") / 256.0f;

	treesX = mapDims.mapx / TREE_SQUARE_SIZE;
	treesY = mapDims.mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
}

ITreeDrawer::~ITreeDrawer() {
	eventHandler.RemoveClient(this);
	configHandler->Set("TreeRadius", (unsigned int) (baseTreeDistance * 256));
}



void ITreeDrawer::AddTrees()
{
	const auto& activeFeatureIDs = featureHandler->GetActiveFeatureIDs();

	for (const int featureID: activeFeatureIDs) {
		const CFeature* f = featureHandler->GetFeature(featureID);

		if (f->def->drawType >= DRAWTYPE_TREE) {
			AddTree(f->id, f->def->drawType - 1, f->pos, 1.0f);
		}
	}
}


void ITreeDrawer::AddTree(int treeID, int treeType, const float3& pos, float size)
{
	TreeStruct ts;
	ts.id = treeID;
	ts.type = treeType;
	ts.pos = pos;

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x) / (treeSquareSize)) +
		(((int)pos.z) / (treeSquareSize) * treesX);

	spring::VectorInsertUnique(treeSquares[treeSquareIdx].trees, ts, true);
	ResetPos(pos);
}

void ITreeDrawer::DeleteTree(int treeID, const float3& pos)
{
	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x / (treeSquareSize))) +
		(((int)pos.z / (treeSquareSize) * treesX));

	spring::VectorEraseIf(treeSquares[treeSquareIdx].trees, [treeID](const TreeStruct& ts) { return (treeID == ts.id); });
	ResetPos(pos);
}


void ITreeDrawer::ResetPos(const float3& pos)
{
	const int x = (int)(pos.x / TREE_SQUARE_SIZE / SQUARE_SIZE);
	const int y = (int)(pos.z / TREE_SQUARE_SIZE / SQUARE_SIZE);

	TreeSquareStruct& tss = treeSquares[y * treesX + x];

	if (tss.dispList != 0) {
		delDispLists.push_back(tss.dispList);
		tss.dispList = 0;
	}
	if (tss.farDispList != 0) {
		delDispLists.push_back(tss.farDispList);
		tss.farDispList = 0;
	}
}


ITreeDrawer* ITreeDrawer::GetTreeDrawer()
{
	ITreeDrawer* td = nullptr;

	try {
		td = new CAdvTreeDrawer();
	} catch (const content_error& e) {
		LOG_L(L_ERROR, "[ITreeDrawer] exception \"%s\"; falling back to NullTreeDrawer", e.what());
		assert(td == nullptr);
	}

	if (td == nullptr)
		td = new CNullTreeDrawer();

	td->AddTrees();
	return td;
}



void ITreeDrawer::SetupState() const {
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.005f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// NOTE:
	//   the info-texture now contains an alpha-component
	//   so binding it here means trees will be invisible
	//   when shadows are disabled
	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);
		glActiveTexture(GL_TEXTURE0);
	}
}

void ITreeDrawer::ResetState() const {
	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}

	glPopAttrib();
}



void ITreeDrawer::Draw()
{
	const float maxDistance = CGlobalRendering::MAX_VIEW_RANGE / (SQUARE_SIZE * TREE_SQUARE_SIZE);
	const float treeDistance = Clamp(baseTreeDistance, 1.0f, maxDistance);

	if (!drawTrees)
		return;

	SetupState();
	Draw(treeDistance);
	ResetState();
}

void ITreeDrawer::Update()
{
	for (auto i = delDispLists.begin(); i != delDispLists.end(); ++i) {
		glDeleteLists(*i, 1);
	}
	delDispLists.clear();
}



void ITreeDrawer::RenderFeatureCreated(const CFeature* feature) {
	// support /give'ing tree objects
	if (feature->def->drawType < DRAWTYPE_TREE)
		return;

	AddTree(feature->id, feature->def->drawType - 1, feature->pos, 1.0f);
}

void ITreeDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos) {
	if (feature->def->drawType < DRAWTYPE_TREE)
		return;

	DeleteTree(feature->id, oldpos);
	AddTree(feature->id, feature->def->drawType - 1, feature->pos, 1.0f);
}

void ITreeDrawer::RenderFeatureDestroyed(const CFeature* feature) {
	if (feature->def->drawType < DRAWTYPE_TREE)
		return;

	DeleteTree(feature->id, feature->pos);

	if (feature->speed.SqLength2D() <= 0.25f)
		return;

	AddFallingTree(feature->id, feature->def->drawType - 1, feature->pos, feature->speed * XZVector);
}

