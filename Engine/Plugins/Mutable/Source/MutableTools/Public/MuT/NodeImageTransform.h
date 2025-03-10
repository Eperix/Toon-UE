// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuR/Ptr.h"
#include "MuR/RefCounted.h"
#include "MuT/Node.h"
#include "MuT/NodeImage.h"

namespace mu
{

	// Forward definitions
	class NodeScalar;
	typedef Ptr<NodeScalar> NodeScalarPtr;
	typedef Ptr<const NodeScalar> NodeScalarPtrConst;

	class NodeImageTransform;
	typedef Ptr<NodeImageTransform> NodeImageTransformPtr;
	typedef Ptr<const NodeImageTransform> NodeImageTransformPtrConst;

	enum class EAddressMode;

	//! Node that multiplies the colors of an image, channel by channel.
	//! \ingroup model
	class MUTABLETOOLS_API NodeImageTransform : public NodeImage
	{
	public:

		NodeImageTransform();

		//-----------------------------------------------------------------------------------------
		// Node Interface
		//-----------------------------------------------------------------------------------------

        const FNodeType* GetType() const override;
		static const FNodeType* GetStaticType();

		//-----------------------------------------------------------------------------------------
		// Own Interface
		//-----------------------------------------------------------------------------------------

		//! Base image to multiply.
		NodeImagePtr GetBase() const;
		void SetBase( NodeImagePtr );
	
		NodeScalarPtr GetOffsetX() const;
		NodeScalarPtr GetOffsetY() const;
		NodeScalarPtr GetScaleX() const;
		NodeScalarPtr GetScaleY() const;
		NodeScalarPtr GetRotation() const;
		void SetOffsetX( NodeScalarPtr pNode );
		void SetOffsetY( NodeScalarPtr pNode );
		void SetScaleX( NodeScalarPtr pNode );
		void SetScaleY( NodeScalarPtr pNode );
		void SetRotation( NodeScalarPtr pNode );

		EAddressMode GetAddressMode() const;
		void SetAddressMode(EAddressMode AddressMode);

		uint16 GetSizeX() const;
		void SetSizeX(uint16 SizeX); 

		uint16 GetSizeY() const;
		void SetSizeY(uint16 SizeY); 

		bool GetKeepAspectRatio() const;
		void SetKeepAspectRatio(bool bKeepAspectRatio);

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
		~NodeImageTransform();

	private:

		Private* m_pD;

	};


}
