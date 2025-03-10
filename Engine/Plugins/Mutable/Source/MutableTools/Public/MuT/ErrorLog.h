// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/PlatformCrt.h"
#include "MuR/Ptr.h"
#include "MuR/RefCounted.h"


namespace mu
{

	// Forward declarations
	class ErrorLog;
	typedef Ptr<ErrorLog> ErrorLogPtr;
	typedef Ptr<const ErrorLog> ErrorLogPtrConst;


	//! Types of message stored in the log
	//! \ingroup tools
	typedef enum
	{
        ELMT_NONE=0,
		ELMT_ERROR,
		ELMT_WARNING,
		ELMT_INFO
	} ErrorLogMessageType;

	//! Categories of message stored in the log for the purpose of limiiting duplication of non-identical messages
	//! \ingroup tools
	typedef enum
	{
		ELMSB_ALL = 0,
		ELMSB_UNKNOWN_TAG
	} ErrorLogMessageSpamBin;

	struct ErrorLogMessageAttachedDataView 
	{
		const float* m_unassignedUVs = nullptr;
		size_t m_unassignedUVsSize = 0;
	};

	//! Class used to store the error, warning and information messages from several processes
	//! performed by the tools library, like model transformation or compilation.
	//! \ingroup tools
	class MUTABLETOOLS_API ErrorLog : public RefCounted
	{
	public:

		//-----------------------------------------------------------------------------------------
		// Life cycle
		//-----------------------------------------------------------------------------------------
		ErrorLog();

		//-----------------------------------------------------------------------------------------
		// Own interface
		//-----------------------------------------------------------------------------------------

		//! Get the number of messages.
		//! If a message type is provided, return the number of that type of message only.
		int32 GetMessageCount( ErrorLogMessageType type = ELMT_NONE ) const;

		//! Get the text of a message.
		//! \param index index of the message from 0 to GetMessageCount(ELMT_NONE)-1
		const FString& GetMessageText( int32 index ) const;

		//! Get the opaque context of a message.
		//! \param index index of the message from 0 to GetMessageCount(ELMT_NONE)-1
		const void* GetMessageContext(int32 index) const;
		const void* GetMessageContext2(int32 index) const;

		//!
		ErrorLogMessageType GetMessageType( int32 index ) const;

		//!
		ErrorLogMessageSpamBin GetMessageSpamBin(int32 index) const;

		//! Get the attached data of a message.
		//! \param index index of message data from 0 to GetMessageCount(ELMT_NONE)-1
		ErrorLogMessageAttachedDataView GetMessageAttachedData( int32 index ) const;

		//!
		void Log() const;

		//!
		void Merge( const ErrorLog* pOther );

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
		~ErrorLog();

	private:

		Private* m_pD;

	};
	
	MUTABLETOOLS_API extern const TCHAR* s_opNames[];
}
