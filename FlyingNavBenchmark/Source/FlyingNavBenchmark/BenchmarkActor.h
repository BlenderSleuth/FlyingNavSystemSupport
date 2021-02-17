// Copyright Ben Sutherland 2021. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingNavSystemTypes.h"
#include "FlyingObjectInterface.h"
#include "Engine/DataTable.h"
#include "BenchmarkActor.generated.h"

USTRUCT(BlueprintType)
struct FBenchmarkTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	int32 NumLayers;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	int32 NumVoxels;

	UPROPERTY(BlueprintReadOnly, Category = "Benchmark", meta=(DisplayName = "Generation Time (ms)"))
	float GenerationTimeMs;
	
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	float AStarTimeMs;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    int64 AStarIterations;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	FString AStarDistance;

	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	float LazyThetaStarTimeMs;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    int64 LazyThetaStarIterations;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	FString LazyThetaStarDistance;
	
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	float ThetaStarTimeMs;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    int64 ThetaStarIterations;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	FString ThetaStarDistance;

	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	float OctreeRaycastTimeMicroS;
	UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
	float PhysicsLineTraceTimeMicroS;
};

USTRUCT(BlueprintType)
struct FBenchmarkSettingsTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// How much to scale the A* heuristic by. High values can speed up pathfinding, at the cost of accuracy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	float HeuristicScale = 1.f;

	// Makes all nodes, regardless of size, the same cost. Speeds up pathfinding at the cost of accuracy (AI prefers open spaces).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bUseUnitCost = false;

	// Compensates node size even more, by multiplying node cost by 1 for a leaf node, and 0.2f for the root node.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bUseNodeCompensation = false;

	FString ToString() const
	{
		return FString::Printf(TEXT("(Query: HeuristicScale = %.1f, bUseUnitCost = %s, bUseNodeCompensation = %s)"),
            HeuristicScale,
            bUseUnitCost ? TEXT("true") : TEXT("false"),
            bUseNodeCompensation ? TEXT("true") : TEXT("false"));
	}
	FString Filename() const
	{
		return FString::Printf(TEXT("(HS %.1f UC %s NC %s).csv"),
            HeuristicScale,
            bUseUnitCost ? TEXT("true") : TEXT("false"),
            bUseNodeCompensation ? TEXT("true") : TEXT("false"));
	}
};

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor=true))
enum class EBenchmarkAlgorithm: uint8
{
	None			= 0			UMETA(Hidden),
	AStar			= 1 << 0	UMETA(DisplayName = "A *"),
    LazyThetaStar	= 1 << 1	UMETA(DisplayName = "Lazy Theta *"),
    ThetaStar		= 1	<< 2	UMETA(DisplayName = "Theta *")
};
ENUM_CLASS_FLAGS(EBenchmarkAlgorithm)

UCLASS()
class FLYINGNAVBENCHMARK_API ABenchmarkActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABenchmarkActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(MakeEditWidget = true))
	FVector PathStartPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(MakeEditWidget = true))
	FVector PathEndPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(MakeEditWidget = true))
	FVector RaycastStartPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(MakeEditWidget = true))
	FVector RaycastEndPoint;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	bool bDrawBenchmarkPaths;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	bool bBenchmarkOnBeginPlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	bool bBenchmarkRaycasts;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	TArray<float> BenchmarkResolutions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(Bitmask, BitmaskEnum = "EBenchmarkAlgorithm"))
	int32 PathfindingAlgorithms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	UDataTable* BenchmarkDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	UDataTable* QuerySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark, meta=(FilePathFilter="csv"))
	FFilePath SettingsPath;

	UPROPERTY(VisibleAnywhere, Category = Benchmark)
	FString AbsoluteSettingsPath;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	FDirectoryPath OutputPath;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Benchmark)
	FString Filename;

	UFUNCTION(CallInEditor, Category = Benchmark)
	void Benchmark();

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = Benchmark)
    void ClearViewport() const;
	
	UFUNCTION(CallInEditor, Category = Benchmark)
    void SetAbsoluteSettingsPath();
#endif // WITH_EDITOR
	
	virtual void BeginPlay() override;

	void UpdateSVOData();
	
protected:
	UPROPERTY()
	class AFlyingNavigationData* FlyingNavData;
	FSVODataConstPtr SVOData;
};
