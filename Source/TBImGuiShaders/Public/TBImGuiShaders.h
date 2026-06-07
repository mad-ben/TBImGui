// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Modules/ModuleInterface.h>
#include <GlobalShader.h>
#include <ShaderParameterUtils.h>

class FTBImGuiShadersModule : public IModuleInterface
{
public:
    
	/// Begin IModuleInterface Override
	virtual void StartupModule() override;
	/// End IModuleInterface Override
};

class TBIMGUISHADERS_API FTBImGuiVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTBImGuiVS);

public:

	FTBImGuiVS() = default;
	FTBImGuiVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ProjectionMatrix.Bind(Initializer.ParameterMap, TEXT("ProjectionMatrix"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
	{
		return true;
	}

	void SetProjectionMatrix(FRHIBatchedShaderParameters& BatchedParameters, const FMatrix44f& InProjectionMatrix)
	{
		SetShaderValue(BatchedParameters, ProjectionMatrix, InProjectionMatrix);
	}

private:

	LAYOUT_FIELD(FShaderParameter, ProjectionMatrix)
};

class TBIMGUISHADERS_API FTBImGuiPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTBImGuiPS);

public:

	FTBImGuiPS() = default;
	FTBImGuiPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
	{
		Texture.Bind(Initializer.ParameterMap, TEXT("Texture"));
		TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));
		UsingSRGB.Bind(Initializer.ParameterMap, TEXT("UsingSRGB"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	void SetTexture(FRHIBatchedShaderParameters& BatchedParameters, FRHITexture* InTexture, FRHISamplerState* InSampler, bool bSRGB)
	{
		SetTextureParameter(BatchedParameters, Texture, TextureSampler, InSampler, InTexture);
		SetShaderValue(BatchedParameters, UsingSRGB, bSRGB ? 1u : 0u);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, Texture);
	LAYOUT_FIELD(FShaderResourceParameter, TextureSampler);
	LAYOUT_FIELD(FShaderParameter, UsingSRGB);
};