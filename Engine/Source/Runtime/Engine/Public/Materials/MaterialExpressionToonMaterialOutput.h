#pragma once


#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionToonMaterialOutput.generated.h"

UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionToonMaterialOutput : public UMaterialExpressionCustomOutput
{
public:
	GENERATED_UCLASS_BODY()

	/** Input for scattering coefficient describing how light scatter around and is absorbed. Valid range is [0,+inf[. Unit is 1/cm. */
	// UPROPERTY()
	// FExpressionInput ToonDataA;
	// uint 8bit [0 - 255]
	UPROPERTY()
	FExpressionInput SelfID;
	// uint 8bit [0 - 255]
	UPROPERTY()
	FExpressionInput ObjectID;
	// uint 3bit [0 - 7]
	UPROPERTY()
	FExpressionInput ToonModel;
	// uint 5bit Mask
	UPROPERTY()
	FExpressionInput ShadowCastFlag;
	UPROPERTY()
	// float 8bit [0 - 1]
	FExpressionInput HairShadowOffset;

	// float 8bit [0 - 1]
	UPROPERTY()
	FExpressionInput SpecularSmoothness;
	// float 8bit [-1 - 1]
	UPROPERTY()
	FExpressionInput SpecularOffset;
		
	/** Input for phase function 'g' parameter describing how much forward(g>0) or backward (g<0) light scatter around. Valid range is [-1,1]. */
	UPROPERTY()
	FExpressionInput ToonDataC;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual uint32 GetInputType(int32 InputIndex) override;
	
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual int32 GetNumOutputs() const override;
	virtual FString GetFunctionName() const override;
	virtual FString GetDisplayName() const override;
};
