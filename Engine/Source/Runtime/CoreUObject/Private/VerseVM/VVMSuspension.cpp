// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_VERSE_VM || defined(__INTELLISENSE__)
#include "VerseVM/VVMSuspension.h"
#include "VerseVM/Inline/VVMAbstractVisitorInline.h"
#include "VerseVM/Inline/VVMCellInline.h"
#include "VerseVM/Inline/VVMMarkStackVisitorInline.h"
#include "VerseVM/VVMBytecodesAndCaptures.h"
#include "VerseVM/VVMCaptureSwitch.h"
#include "VerseVM/VVMCppClassInfo.h"
#include "VerseVM/VVMFailureContext.h"
#include "VerseVM/VVMPackage.h"
#include "VerseVM/VVMProcedure.h"
#include "VerseVM/VVMTask.h"

namespace Verse
{

DEFINE_DERIVED_VCPPCLASSINFO(VSuspension);
DEFINE_DERIVED_VCPPCLASSINFO(VBytecodeSuspension);
DEFINE_DERIVED_VCPPCLASSINFO(VLambdaSuspension);
TGlobalTrivialEmergentTypePtr<&VBytecodeSuspension::StaticCppClassInfo> VBytecodeSuspension::GlobalTrivialEmergentType;
TGlobalTrivialEmergentTypePtr<&VLambdaSuspension::StaticCppClassInfo> VLambdaSuspension::GlobalTrivialEmergentType;

template <typename TVisitor>
void VSuspension::VisitReferencesImpl(TVisitor& Visitor)
{
	Visitor.Visit(FailureContext, TEXT("FailureContext"));
	Visitor.Visit(Task, TEXT("Task"));
	Visitor.Visit(Next, TEXT("Next"));
}

template <typename TVisitor>
void VBytecodeSuspension::VisitReferencesImpl(TVisitor& Visitor)
{
	Visitor.Visit(Procedure, TEXT("Procedure"));
	CaptureSwitch([&Visitor](auto& Captures) {
		Captures.ForEachOperand([&Visitor](EOperandRole, auto& Value, const TCHAR* Name) {
			Visitor.Visit(Value, Name);
		});
	});
}

template <typename TVisitor>
void VLambdaSuspension::VisitReferencesImpl(TVisitor& Visitor)
{
	if constexpr (TVisitor::bIsAbstractVisitor)
	{
		uint64 ScratchNumValues = NumValues;
		Visitor.BeginArray(TEXT("Args"), ScratchNumValues);
		Visitor.Visit(Args(), Args() + NumValues);
		Visitor.EndArray();
	}
	else
	{
		Visitor.Visit(Args(), Args() + NumValues);
	}
}

} // namespace Verse
#endif // WITH_VERSE_VM || defined(__INTELLISENSE__)
