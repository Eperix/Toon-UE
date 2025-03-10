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

	class NodeImageBinarise;
	typedef Ptr<NodeImageBinarise> NodeImageBinarisePtr;
	typedef Ptr<const NodeImageBinarise> NodeImageBinarisePtrConst;


	//! Node that multiplies the colors of an image, channel by channel.
	//! \ingroup model
	class MUTABLETOOLS_API NodeImageBinarise : public NodeImage
	{
	public:

		NodeImageBinarise();

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

		//! Colour to multiply by the image.
		NodeScalarPtr GetThreshold() const;
		void SetThreshold( NodeScalarPtr );

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
		~NodeImageBinarise();

	private:

		Private* m_pD;

	};


}
