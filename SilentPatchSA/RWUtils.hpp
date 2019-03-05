#pragma once

#include <rwcore.h>
#include <rpworld.h>

class RwScopedRenderState
{
public:
	RwScopedRenderState( RwRenderState state )
		: m_state( state )
	{
		[[maybe_unused]] RwBool result = RwRenderStateGet( m_state, &m_value );
		assert( result != FALSE );
	}

	~RwScopedRenderState()
	{
		[[maybe_unused]] RwBool result = RwRenderStateSet( m_state, m_value );
		assert( result != FALSE );
	}

private:
	RwScopedRenderState( const RwScopedRenderState& ) = delete;

	const RwRenderState m_state;
	void* m_value = nullptr;
};

template <typename Pred>
Pred RwFrameForAllChildren(RwFrame* frame, Pred&& callback)
{
	for ( RwFrame* curFrame = frame->child; curFrame != nullptr; curFrame = curFrame->next )
	{
		if ( std::forward<Pred>(callback)(curFrame) == nullptr )
			break;
	}
	return std::forward<Pred>(callback);
}

template <typename Pred>
Pred RwFrameForAllObjects(RwFrame* frame, Pred&& callback)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&frame->objectList); link != rwLinkListGetTerminator(&frame->objectList); link = rwLLLinkGetNext(link) )
	{
		if ( std::forward<Pred>(callback)(&rwLLLinkGetData(link, RwObjectHasFrame, lFrame)->object) == nullptr )
			break;
	}

	return std::forward<Pred>(callback);
}

template <typename Pred>
Pred RpClumpForAllAtomics(RpClump* clump, Pred&& callback)
{
	for ( RwLLLink* link = rwLinkListGetFirstLLLink(&clump->atomicList); link != rwLinkListGetTerminator(&clump->atomicList); link = rwLLLinkGetNext(link) )
	{
		if ( std::forward<Pred>(callback)(rwLLLinkGetData(link, RpAtomic, inClumpLink)) == nullptr )
			break;
	}
	return std::forward<Pred>(callback);
}

template <typename Pred>
Pred RpGeometryForAllMaterials(RpGeometry* geometry, Pred&& callback)
{
	for ( RwInt32 i = 0, j = geometry->matList.numMaterials; i < j; i++ )
	{
		if ( std::forward<Pred>(callback)(geometry->matList.materials[i]) == nullptr )
			break;
	}
	return std::forward<Pred>(callback);
}