// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "GeneratedCppIncludes.h"
#include "Public/MyMaterialExpression.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMyMaterialExpression() {}
// Cross Module References
	CUSTOMNODE_API UClass* Z_Construct_UClass_UMyMaterialExpression_NoRegister();
	CUSTOMNODE_API UClass* Z_Construct_UClass_UMyMaterialExpression();
	ENGINE_API UClass* Z_Construct_UClass_UMaterialExpression();
	UPackage* Z_Construct_UPackage__Script_CustomNode();
	ENGINE_API UScriptStruct* Z_Construct_UScriptStruct_FExpressionInput();
// End Cross Module References
	void UMyMaterialExpression::StaticRegisterNativesUMyMaterialExpression()
	{
	}
	UClass* Z_Construct_UClass_UMyMaterialExpression_NoRegister()
	{
		return UMyMaterialExpression::StaticClass();
	}
	UClass* Z_Construct_UClass_UMyMaterialExpression()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			static UObject* (*const DependentSingletons[])() = {
				(UObject* (*)())Z_Construct_UClass_UMaterialExpression,
				(UObject* (*)())Z_Construct_UPackage__Script_CustomNode,
			};
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
				{ "HideCategories", "Object Object" },
				{ "IncludePath", "MyMaterialExpression.h" },
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_myIndex_MetaData[] = {
				{ "Category", "MyMaterial" },
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
			static const UE4CodeGen_Private::FFloatPropertyParams NewProp_myIndex = { UE4CodeGen_Private::EPropertyClass::Float, "myIndex", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000001, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, myIndex), METADATA_PARAMS(NewProp_myIndex_MetaData, ARRAY_COUNT(NewProp_myIndex_MetaData)) };
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Input5_MetaData[] = {
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
			static const UE4CodeGen_Private::FStructPropertyParams NewProp_Input5 = { UE4CodeGen_Private::EPropertyClass::Struct, "Input5", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000000, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, Input5), Z_Construct_UScriptStruct_FExpressionInput, METADATA_PARAMS(NewProp_Input5_MetaData, ARRAY_COUNT(NewProp_Input5_MetaData)) };
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Input4_MetaData[] = {
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
			static const UE4CodeGen_Private::FStructPropertyParams NewProp_Input4 = { UE4CodeGen_Private::EPropertyClass::Struct, "Input4", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000000, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, Input4), Z_Construct_UScriptStruct_FExpressionInput, METADATA_PARAMS(NewProp_Input4_MetaData, ARRAY_COUNT(NewProp_Input4_MetaData)) };
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Input3_MetaData[] = {
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
			static const UE4CodeGen_Private::FStructPropertyParams NewProp_Input3 = { UE4CodeGen_Private::EPropertyClass::Struct, "Input3", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000000, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, Input3), Z_Construct_UScriptStruct_FExpressionInput, METADATA_PARAMS(NewProp_Input3_MetaData, ARRAY_COUNT(NewProp_Input3_MetaData)) };
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Input2_MetaData[] = {
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
			};
#endif
			static const UE4CodeGen_Private::FStructPropertyParams NewProp_Input2 = { UE4CodeGen_Private::EPropertyClass::Struct, "Input2", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000000, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, Input2), Z_Construct_UScriptStruct_FExpressionInput, METADATA_PARAMS(NewProp_Input2_MetaData, ARRAY_COUNT(NewProp_Input2_MetaData)) };
#if WITH_METADATA
			static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Input1_MetaData[] = {
				{ "ModuleRelativePath", "Public/MyMaterialExpression.h" },
				{ "ToolTip", "???\xca\xbd\xda\xb5???????" },
			};
#endif
			static const UE4CodeGen_Private::FStructPropertyParams NewProp_Input1 = { UE4CodeGen_Private::EPropertyClass::Struct, "Input1", RF_Public|RF_Transient|RF_MarkAsNative, 0x0010000000000000, 1, nullptr, STRUCT_OFFSET(UMyMaterialExpression, Input1), Z_Construct_UScriptStruct_FExpressionInput, METADATA_PARAMS(NewProp_Input1_MetaData, ARRAY_COUNT(NewProp_Input1_MetaData)) };
			static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[] = {
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_myIndex,
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_Input5,
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_Input4,
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_Input3,
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_Input2,
				(const UE4CodeGen_Private::FPropertyParamsBase*)&NewProp_Input1,
			};
			static const FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
				TCppClassTypeTraits<UMyMaterialExpression>::IsAbstract,
			};
			static const UE4CodeGen_Private::FClassParams ClassParams = {
				&UMyMaterialExpression::StaticClass,
				DependentSingletons, ARRAY_COUNT(DependentSingletons),
				0x00082080u,
				nullptr, 0,
				PropPointers, ARRAY_COUNT(PropPointers),
				nullptr,
				&StaticCppClassTypeInfo,
				nullptr, 0,
				METADATA_PARAMS(Class_MetaDataParams, ARRAY_COUNT(Class_MetaDataParams))
			};
			UE4CodeGen_Private::ConstructUClass(OuterClass, ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UMyMaterialExpression, 1456983449);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UMyMaterialExpression(Z_Construct_UClass_UMyMaterialExpression, &UMyMaterialExpression::StaticClass, TEXT("/Script/CustomNode"), TEXT("UMyMaterialExpression"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UMyMaterialExpression);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
