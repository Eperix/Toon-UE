#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionToonLightOutput.generated.h"

/** Toon材质的自定义属性输出. */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionToonLightOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	// float3
	UPROPERTY()
	FExpressionInput ToonLighting;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	// 主要的功能实现在Compile()函数种
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual uint32 GetInputType(int32 InputIndex) override;
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	// 针脚的数量
	virtual int32 GetNumOutputs() const override;
	// 获取针脚属性的函数名
	virtual FString GetFunctionName() const override;
	// 节点的名称
	virtual FString GetDisplayName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};