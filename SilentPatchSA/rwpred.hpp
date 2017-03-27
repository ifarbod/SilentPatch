#pragma once

#include <rwcore.h>
#include <rpworld.h>

template <typename Pred>
RwFrame* RwFrameForAllChildren(RwFrame* frame, Pred callback)
{
	for ( RwFrame* curFrame = frame->child; curFrame != nullptr; curFrame = curFrame->next )
	{
		if ( callback(curFrame) == nullptr )
			break;
	}
	return frame;
}

template <typename Pred>
RwFrame* RwFrameForAllObjects(RwFrame* frame, Pred callback)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&frame->objectList); link != rwLinkListGetTerminator(&frame->objectList); link = rwLLLinkGetNext(link) )
	{
		if ( callback(&rwLLLinkGetData(link, RwObjectHasFrame, lFrame)->object) == nullptr )
			break;
	}

	return frame;
}

template <typename Pred>
RpClump* RpClumpForAllAtomics(RpClump* clump, Pred callback)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&clump->atomicList); link != rwLinkListGetTerminator(&clump->atomicList); link = rwLLLinkGetNext(link) )
	{
		if ( callback(rwLLLinkGetData(link, RpAtomic, inClumpLink)) == nullptr )
			break;
	}
	return clump;
}

template <typename Pred>
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, Pred callback)
{
	for ( RwInt32 i = 0, j = geometry->matList.numMaterials; i < j; i++ )
	{
		if ( callback(geometry->matList.materials[i]) == nullptr )
			break;
	}
	return geometry;
}