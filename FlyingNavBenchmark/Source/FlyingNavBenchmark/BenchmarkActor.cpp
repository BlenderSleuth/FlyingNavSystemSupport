// Copyright Ben Sutherland 2021. All rights reserved.

#include "BenchmarkActor.h"
#include "FlyingNavigationData.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "CSVExporter.h"
#include "NavigationSystem.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

static_assert(PATH_BENCHMARK, "Requires the PATH_BENCHMARK to be enabled in .build.cs");
// REMEMBER TO UNCOMMENT PublicDefinitions.Add("PATH_BENCHMARK=1") in FlyingNavSystem.Build.cs

// Sets default values
ABenchmarkActor::ABenchmarkActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	bDrawBenchmarkPaths = false;
	bBenchmarkOnBeginPlay = false;
	BenchmarkResolutions = TArray<float>({256.f, 128.f, 64.f, 32.f, 16.f, 8.f});
	//Filename = FString(TEXT("Benchmark.csv"));

	static const ConstructorHelpers::FObjectFinder<UDataTable> DataTable(TEXT("DataTable'/Game/BenchmarkData.BenchmarkData'"));
	BenchmarkDataTable = DataTable.Object;
	static const ConstructorHelpers::FObjectFinder<UDataTable> SettingsTable(TEXT("DataTable'/Game/BenchmarkSettings.BenchmarkSettings'"));
	QuerySettings = SettingsTable.Object;
}

FORCEINLINE FString AsPercent(const double Proportion)
{
	return FString::FromInt(FMath::RoundToInt(Proportion * 1e2)) + TEXT("%");
}
FORCEINLINE float FormatDouble(const double Value)
{
	return FCString::Atof(*FString::Printf(TEXT("%.2f"), Value));
}

void ABenchmarkActor::Benchmark()
{
	UpdateSVOData();

	// Benchmark
	if (BenchmarkDataTable == nullptr || QuerySettings == nullptr || FlyingNavData == nullptr || !SVOData.IsValid() || FlyingNavData->GetSyncPathfindingGraph() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Are you sure you've set up the navigation actor correctly?"))
		return;
	}

	// Transform path points to world space
	const FVector PathStart = GetActorTransform().TransformPosition(PathStartPoint);
	const FVector PathEnd = GetActorTransform().TransformPosition(PathEndPoint);
	const FVector RayStart = GetActorTransform().TransformPosition(RaycastStartPoint);
	const FVector RayEnd = GetActorTransform().TransformPosition(RaycastEndPoint);

	FSVOPathfindingGraph* const PathfindingGraph = FlyingNavData->GetSyncPathfindingGraph();
	
	// Load settings from csv file
	if (FPaths::FileExists(AbsoluteSettingsPath))
	{
		QuerySettings->EmptyTable();
		FString CSVSettings;
		FFileHelper::LoadFileToString(CSVSettings, *AbsoluteSettingsPath);
		QuerySettings->CreateTableFromCSVString(CSVSettings);
	}

	// Run Benchmark for each row
	FString AllBenchmarks;
	QuerySettings->ForeachRow<FBenchmarkSettingsTableRow>(nullptr, [&](const FName& Key, const FBenchmarkSettingsTableRow& SettingsRow)
	{
		BenchmarkDataTable->EmptyTable();

		const float OldResolution = FlyingNavData->MaxDetailSize;

		for (int i = 0; i < BenchmarkResolutions.Num(); i++)
		{
			FlyingNavData->MaxDetailSize = BenchmarkResolutions[i];
			const int32 NumLayers = FlyingNavSystem::GetNumLayers(FlyingNavData->GetOctreeSideLength(), BenchmarkResolutions[i]);
			const int32 NumVoxels = FMath::Pow(8, NumLayers);

			double StartTime = FPlatformTime::Seconds();
			FlyingNavData->SyncBuild();
			double EndTime = FPlatformTime::Seconds();
			const double GenerationTime = EndTime - StartTime;
			SVOData = FlyingNavData->GetSVOData().AsShared();

			// Benchmark pathfinding
			double PathfindingTimes[3]; // Time in seconds
			int64 PathfindingIterations[3];
			double PathfindingDistance[3]; // Percentage of ideal path distance (>100%)
			for (int j = 0; j < 3; j++)
			{
				if (PathfindingAlgorithms & (1 << j))
				{
					const FSVOQuerySettings IdealSettings(
						*SVOData,
						EPathfindingAlgorithm::ThetaStar,
						false,
						1.0f,
						false,
						false);

					// Benchmark against perfect path
					TArray<FNavPathPoint> IdealPathPoints;
					PathfindingGraph->FindPath(PathStart, PathEnd, IdealSettings, IdealPathPoints);

					const uint32 PathPointsCount = IdealPathPoints.Num();
					double IdealPathDistance = 0.f;
					if (PathPointsCount > 0)
					{
						FVector SegmentStart = IdealPathPoints[0].Location;

						for (uint32 PathIndex = 1; PathIndex < PathPointsCount; ++PathIndex)
						{
							const FVector SegmentEnd = IdealPathPoints[PathIndex].Location;
							IdealPathDistance += FVector::Dist(SegmentStart, SegmentEnd);
							SegmentStart = SegmentEnd;
						}
					}

					// Do actual benchmark
					const FSVOQuerySettings Settings(
						*SVOData,
						static_cast<EPathfindingAlgorithm>(j),
						false,
						SettingsRow.HeuristicScale,
						SettingsRow.bUseUnitCost,
						SettingsRow.bUseNodeCompensation);

					Settings.NumIterations = 0;
					FNavPathSharedPtr NavPath = MakeShareable(new FFlyingNavigationPath());
					NavPath->SetNavigationDataUsed(FlyingNavData);

					const int32 PathfindingTrials = 5;
						
					StartTime = FPlatformTime::Seconds();
					for (int k = 0; k < PathfindingTrials; k++)
					{
						PathfindingGraph->FindPath(PathStart, PathEnd, Settings, NavPath->GetPathPoints());
					}
					EndTime = FPlatformTime::Seconds();
					PathfindingTimes[j] = (EndTime - StartTime) / static_cast<double>(PathfindingTrials);

					PathfindingIterations[j] = Settings.NumIterations / PathfindingTrials;
					PathfindingDistance[j] = static_cast<double>(NavPath->GetLength()) / IdealPathDistance;

					// Draw path
					if (bDrawBenchmarkPaths)
					{
						// Translate by NavigationDebugDrawing::PathOffset
						for (FNavPathPoint& PathPoint : NavPath->GetPathPoints())
						{
							PathPoint.Location.Z -= NavigationDebugDrawing::PathOffset.Z;
						}
						FlyingNavData->DrawDebugPath(NavPath.Get(), FColor::MakeRedToGreenColorFromScalar(j / 2.f));
					}
				}
				else
				{
					PathfindingTimes[j] = 0.;
					PathfindingIterations[j] = 0;
					PathfindingDistance[j] = 0.;
				}
			}

			// Raycast benchmark
			double OctreeRaycastTime = 0., LineTraceTime = 0.;
			if (bBenchmarkRaycasts)
			{
				const int32 NumRaycastTrials = 200;
					
				FVector HitLocation;
				bool Hit;
				StartTime = FPlatformTime::Seconds();
				for (int32 k = 0; k < NumRaycastTrials; k++) // Multiple trials
				{
					Hit = FlyingNavData->OctreeRaycast(RayStart, RayEnd, HitLocation);
				}
				EndTime = FPlatformTime::Seconds();
				OctreeRaycastTime = (EndTime - StartTime) / static_cast<double>(NumRaycastTrials);
				//UE_LOG(LogTemp, Warning, TEXT("Octree Hit: %d"), Hit);

				FHitResult HitResult;
				StartTime = FPlatformTime::Seconds();
				for (int32 k = 0; k < NumRaycastTrials; k++) // Multiple trials
				{
					Hit = GetWorld()->LineTraceSingleByChannel(HitResult, RayStart, RayEnd, ECC_WorldStatic);
				}
				EndTime = FPlatformTime::Seconds();
				LineTraceTime = (EndTime - StartTime) / static_cast<double>(NumRaycastTrials);
				//UE_LOG(LogTemp, Warning, TEXT("LineTrace Hit: %d"), Hit);
			}

			FBenchmarkTableRow TableRow;
			const FName RowName(*FString::Printf(TEXT("%d"), NumLayers));
			TableRow.NumLayers = NumLayers;
			TableRow.NumVoxels = NumVoxels;
			TableRow.GenerationTimeMs = FormatDouble(GenerationTime * 1e3);

			TableRow.AStarTimeMs = FormatDouble(PathfindingTimes[0] * 1e3);
			TableRow.AStarIterations = PathfindingIterations[0];
			TableRow.AStarDistance = AsPercent(PathfindingDistance[0]);

			TableRow.LazyThetaStarTimeMs = FormatDouble(PathfindingTimes[1] * 1e3);
			TableRow.LazyThetaStarIterations = PathfindingIterations[1];
			TableRow.LazyThetaStarDistance = AsPercent(PathfindingDistance[1]);

			TableRow.ThetaStarTimeMs = FormatDouble(PathfindingTimes[2] * 1e3);
			TableRow.ThetaStarIterations = PathfindingIterations[2];
			TableRow.ThetaStarDistance = AsPercent(PathfindingDistance[2]);

			TableRow.OctreeRaycastTimeMicroS = FormatDouble(OctreeRaycastTime * 1e6);
			TableRow.PhysicsLineTraceTimeMicroS = FormatDouble(LineTraceTime * 1e6);

			BenchmarkDataTable->AddRow(RowName, TableRow);

			UE_LOG(LogTemp, Warning, TEXT("Added row to benchmark table: %f"), BenchmarkResolutions[i])
		}

		FlyingNavData->MaxDetailSize = OldResolution;

		FString CSVString;
		if (!FCSVExporter(CSVString).WriteTable(*BenchmarkDataTable))
		{
			CSVString = TEXT("Missing RowStruct!\n");
		}

		// Add header and trailing blank lines
		const FString BenchmarkString = FString::Printf(TEXT("\"%s\"\n%s,,,,,,,,,,,,,,,\n,,,,,,,,,,,,,,\n"), *SettingsRow.ToString(), *CSVString);
		AllBenchmarks += BenchmarkString;
	});
	const FString FilePath = FPaths::Combine(OutputPath.Path, Filename);
	FFileHelper::SaveStringToFile(AllBenchmarks, *FilePath);
		
	//FBenchmarkSettingsTableRow* SettingsRow = QuerySettings->FindRow<FBenchmarkSettingsTableRow>(TEXT("Settings"), nullptr, false);
	//checkf(SettingsRow != nullptr, TEXT("Path: %s, Loaded String: %s"), *AbsolutePath, *CSVSettings)
}

#if WITH_EDITOR
void ABenchmarkActor::ClearViewport() const
{
	FlushPersistentDebugLines(GetWorld());
}

void ABenchmarkActor::SetAbsoluteSettingsPath()
{
	AbsoluteSettingsPath = FPaths::ConvertRelativePathToFull(SettingsPath.FilePath);
}
#endif // WITH_EDITOR

void ABenchmarkActor::BeginPlay()
{
	Super::BeginPlay();

	if (bBenchmarkOnBeginPlay)
	{
		Benchmark();
	}
}


void ABenchmarkActor::UpdateSVOData()
{
	// Only one actor of this type in the world
	const TActorIterator<AFlyingNavigationData> It(GetWorld());
	if (It)
	{
		FlyingNavData = *It;
		// If it crashes here, uncomment PublicDefinitions.Add("PATH_BENCHMARK=1"); in FlyingNavSystem.Build.cs
		SVOData = FlyingNavData->GetSVOData().AsShared();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No AFlyingNavigationData actor in world!"))
	}
}
