// Fill out your copyright notice in the Description page of Project Settings.


#include "WFC_Spawner.h"

#include "IPropertyTable.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AWFC_Spawner::AWFC_Spawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AWFC_Spawner::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWFC_Spawner::MainLoop()
{
	if (!bFinishedWfc)
	{
		if (CheckIfFullyCollapsed())
		{
			bFinishedWfc = true;

			//Executes only once at the end: 
			//Cycle through whole field again at end of loop to catch any non-collapsed field
			for (int32 i = 0; i < MapTiles.Num(); i++)
			{
				for (int32 j = 0; j < MapTiles[i].Num(); j++)
				{
					RenderCurrentCellMeshes(FVector2d(i,j));
					Propagate(FVector2d(i,j));
				}
			}
		}
		else
		{
			//Find cell with the lowest entropy and collapse it
			CollapseLowestEntropyCell();
		}
	}
}

void AWFC_Spawner::InitializeMap()
{
	double FunctionTimer = FPlatformTime::Seconds();
	//Initialize MapTiles Coordinates X
	MapTiles.SetNum(MapSize);
	//Initialize CurrentMeshes Coordinates X
	DebugMeshes.SetNum(MapSize);
	//Initialize Final Meshes Coordinates X
	FinalMeshes.SetNum(MapSize);
	
	for (int32 i = 0; i < MapSize; i++)
	{
		//Initialize MapTiles Coordinates Y
		MapTiles[i].SetNum(MapSize);
		//Initialize CurrentMeshes Coordinates Y
		DebugMeshes[i].SetNum(MapSize);

		//Initialize Static Mesh Component Containers to later fill with Meshes
		for (int32 j = 0; j < MapSize; j++)
		{
			//Spawn Component containers for final meshes
			FinalMeshes[i].Add(NewObject<UStaticMeshComponent>(
				this,
				UStaticMeshComponent::StaticClass(),
				*FString("Final_" + FString::FromInt(i) + "_" + FString::FromInt(j))));
			
			if (FinalMeshes[i][j])
			{
				FinalMeshes[i][j]->RegisterComponent();
				FinalMeshes[i][j]->AttachToComponent(
					RootComponent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
				
				FinalMeshes[i][j]->CreationMethod = EComponentCreationMethod::Instance;
				FinalMeshes[i][j]->SetRelativeLocation(FVector(i * 200 + 100, j * 200 + 100, 0));
			}
			for (int32 k = 0; k < Prototypes.Num(); k++)
			{
				//Initialize Prototype ID array
				MapTiles[i][j].Add(k);

				//Spawn small debug meshes
				if (bSpawnDebugMeshes)
				{	
					//SpawnComponents for small Debug Meshes
					DebugMeshes[i][j].Add(NewObject<UStaticMeshComponent>(
						this,
						UStaticMeshComponent::StaticClass(),
						*FString("Debug_" + FString::FromInt(i) + "_" + FString::FromInt(j) + "_" + FString::FromInt(k))));
				
					if (DebugMeshes[i][j][k])
					{
						DebugMeshes[i][j][k]->RegisterComponent();
						DebugMeshes[i][j][k]->AttachToComponent(
							RootComponent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
					
						DebugMeshes[i][j][k]->CreationMethod = EComponentCreationMethod::Instance;
					
						//Set scale of smaller debug meshes to all fit in the same size of the larger meshes
						int32 MeshScaleDivisor = UKismetMathLibrary::Sqrt(Meshes.Num() * 4) + 1;
						DebugMeshes[i][j][k]->SetRelativeScale3D(
							FVector(1.0f / MeshScaleDivisor, 1.0f / MeshScaleDivisor, 1.0f / MeshScaleDivisor));
					
						DebugMeshes[i][j][k]->SetRelativeLocation(FVector(
							200 / MeshScaleDivisor * (k % MeshScaleDivisor) + i * 200 + 200 / MeshScaleDivisor / 2,
							floor(k / MeshScaleDivisor) * (200 / MeshScaleDivisor) + 200 * j + 200 / MeshScaleDivisor / 2,
							50));
					
						DebugMeshes[i][j][k]->SetRelativeRotation(
							FRotator(0, Prototypes[k].Rotation * 90, 0));
					
						DebugMeshes[i][j][k]->SetStaticMesh(Meshes[k / 4]);
					}
				}
			}
		}
	}
	InitializeMapTimer += FPlatformTime::Seconds() - FunctionTimer;
}

void AWFC_Spawner::StartWfcGeneration()
{
	MainTimer = FPlatformTime::Seconds();
	InitializeMap();
	
	//Main loop. Check if every cell has been collapsed and if not, collapse the lowest entropy cell
	
	while(!bFinishedWfc)
	{
		MainLoop();
	}
	// print time elapsed
	UE_LOG(LogTemp, Display, TEXT("Full Calculation Time: %f"), FPlatformTime::Seconds() - MainTimer);
	UE_LOG(LogTemp, Display, TEXT("initializeMap Calculation Time: %f"), InitializeMapTimer);
	UE_LOG(LogTemp, Display, TEXT("collapseLowestEntropyCell Calculation Time: %f"), CollapseLowestEntropyCellTimer);
	UE_LOG(LogTemp, Display, TEXT("propagate Calculation Time: %f"), PropagateTimer);
	UE_LOG(LogTemp, Display, TEXT("replaceMapTiles Calculation Time: %f"), ReplaceMapTilesTimer);
	UE_LOG(LogTemp, Display, TEXT("addCellToPropagationStack Calculation Time: %f"), AddCellToPropagationStackTimer);
	UE_LOG(LogTemp, Display, TEXT("renderCurrentCellMeshes Calculation Time: %f"), RenderCurrentCellMeshesTimer);
	UE_LOG(LogTemp, Display, TEXT("checkIfFullyCollapsed Calculation Time: %f"), CheckIfFullyCollapsedTimer);	
}

void AWFC_Spawner::LoadJson(FWfcPrototypes PrototypeData)
{
	Prototypes.Insert(PrototypeData, PrototypeData.PrototypeID);
}

void AWFC_Spawner::CollapseLowestEntropyCell()
{
	const double FunctionTimer = FPlatformTime::Seconds();
	TArray<FVector2D> LowestEntropyCoords;
	int32 LowestEntropySize = 100;

	//Filter all cells and find all cells that have the same lowest entropy except 1(1 means it's already collapsed)
	for (int32 i = 0; i < MapSize; i++)
	{
		for (int32 j = 0; j < MapSize; j++)
		{
			const int32 Entropy = MapTiles[i][j].Num();
			if (Entropy < LowestEntropySize && Entropy != 1)
			{
				LowestEntropySize = Entropy;
				LowestEntropyCoords.Empty();
				LowestEntropyCoords.AddUnique(FVector2D(i,j));
			}
			else if (Entropy == LowestEntropySize)
			{
				LowestEntropyCoords.AddUnique(FVector2D(i,j));
			}
		}
	}

	//Find a random cell to collapse out of the filtered list of possible candidates
	const FVector2D CollapsingCoord = LowestEntropyCoords[FMath::RandRange(0, LowestEntropyCoords.Num() - 1)];
	
	//Replace chosen collapsing tile with a single random prototype ID out of the remaining possible prototype IDs in that cell
	const int32 CollapsedPrototype = MapTiles[CollapsingCoord.X][CollapsingCoord.Y][FMath::RandRange(0, MapTiles[CollapsingCoord.X][CollapsingCoord.Y].Num() - 1)];
	MapTiles[CollapsingCoord.X][CollapsingCoord.Y].SetNum(1);
	MapTiles[CollapsingCoord.X][CollapsingCoord.Y][0] = CollapsedPrototype;
	
	CollapseLowestEntropyCellTimer += FPlatformTime::Seconds() - FunctionTimer;
	
	RenderCurrentCellMeshes(CollapsingCoord);
	Propagate(CollapsingCoord);
}

void AWFC_Spawner::Propagate(const FVector2d CellCoordinate)
{
	//this propagates the changes to the cell to all it's neighbors and their neighbors and deletes all prototype variations that are no longer possible
	double FunctionTimer = FPlatformTime::Seconds();
	TArray<FVector2d> PropagatedCells;
	TArray<int32> PossibleNeighbors;
	PropagationCoordinateStack.AddUnique(CellCoordinate);
	PropagatedCells.Empty();
	
	//Start propagation loop
	while (PropagationCoordinateStack.Num() > 0)
	{
		//Propagate to X+1
		//Add all possible Neighbors to a list
		for (int32 i = 0; i < MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y].Num(); i++)
		{
			const int32 PrototypeID = MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y][i];
			for (int32 j = 0; j < Prototypes[PrototypeID].Neighbors_X_Plus.Num(); j++)
			{
				PossibleNeighbors.AddUnique(Prototypes[PrototypeID].Neighbors_X_Plus[j]);
			}
		}
		PropagateTimer += FPlatformTime::Seconds() - FunctionTimer;
		//Replace prototype cells in storage with new list of possible neighbors
		if (ReplaceMapTiles(PossibleNeighbors, PropagationCoordinateStack[0] + FVector2d(1, 0)))
		{
			AddCellToPropagationstack(PropagationCoordinateStack[0] + FVector2d(1, 0), PropagatedCells);
		}
		FunctionTimer = FPlatformTime::Seconds();
		PossibleNeighbors.Empty();

		//Propagate to X-1
		//Add all possible Neighbors to a list
		for (int32 i = 0; i < MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y].Num(); i++)
		{
			int32 PrototypeID = MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y][i];
			for (int32 j = 0; j < Prototypes[PrototypeID].Neighbors_X_Minus.Num(); j++)
			{
				PossibleNeighbors.AddUnique(Prototypes[PrototypeID].Neighbors_X_Minus[j]);
			}
		}
		PropagateTimer += FPlatformTime::Seconds() - FunctionTimer;
		//Replace prototype cells in storage with new list of possible neighbors
		if (ReplaceMapTiles(PossibleNeighbors, PropagationCoordinateStack[0] + FVector2d(-1, 0)))
		{
			AddCellToPropagationstack(PropagationCoordinateStack[0] + FVector2d(-1, 0), PropagatedCells);
		}
		FunctionTimer = FPlatformTime::Seconds();
		PossibleNeighbors.Empty();

		//Propagate to Y+1
		//Add all possible Neighbors to a list
		for (int32 i = 0; i < MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y].Num(); i++)
		{
			int32 PrototypeID = MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y][i];
			for (int32 j = 0; j < Prototypes[PrototypeID].Neighbors_Y_Plus.Num(); j++)
			{
				PossibleNeighbors.AddUnique(Prototypes[PrototypeID].Neighbors_Y_Plus[j]);
			}
		}
		PropagateTimer += FPlatformTime::Seconds() - FunctionTimer;
		//Replace prototype cells in storage with new list of possible neighbors
		if (ReplaceMapTiles(PossibleNeighbors, PropagationCoordinateStack[0] + FVector2d(0, 1)))
		{
			AddCellToPropagationstack(PropagationCoordinateStack[0] + FVector2d(0, 1), PropagatedCells);
		}
		FunctionTimer = FPlatformTime::Seconds();
		PossibleNeighbors.Empty();

		//Propagate to Y-1
		//Add all possible Neighbors to a list
		for (int32 i = 0; i < MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y].Num(); i++)
		{
			int32 PrototypeID = MapTiles[PropagationCoordinateStack[0].X][PropagationCoordinateStack[0].Y][i];
			for (int32 j = 0; j < Prototypes[PrototypeID].Neighbors_Y_Minus.Num(); j++)
			{
				PossibleNeighbors.AddUnique(Prototypes[PrototypeID].Neighbors_Y_Minus[j]);
			}
		}
		PropagateTimer += FPlatformTime::Seconds() - FunctionTimer;
		//Replace prototype cells in storage with new list of possible neighbors
		if (ReplaceMapTiles(PossibleNeighbors, PropagationCoordinateStack[0] + FVector2d(0, -1)))
		{
			AddCellToPropagationstack(PropagationCoordinateStack[0] + FVector2d(0, -1), PropagatedCells);
		}
		FunctionTimer = FPlatformTime::Seconds();
		
		PossibleNeighbors.Empty();

		//Remove current cell from stack
		PropagatedCells.AddUnique(PropagationCoordinateStack[0]);
		PropagationCoordinateStack.RemoveAt(0);
	}
	PropagateTimer += FPlatformTime::Seconds() - FunctionTimer;
}

bool AWFC_Spawner::ReplaceMapTiles(TArray<int32> PossibleNeighbors, FVector2d CellCoordinate)
{	
	double FunctionTimer = FPlatformTime::Seconds();
	bool bNeighborlistChanged = false;
	TArray<int32> TempArray;

	//Exit if CellCoordinate points to cell out of bounds
	if (CellCoordinate.X >= MapSize || CellCoordinate.Y >= MapSize || CellCoordinate.X < 0 || CellCoordinate.Y < 0)
	{
		ReplaceMapTilesTimer += FPlatformTime::Seconds() - FunctionTimer;
		return false;
	}
	
	
	for (int32 i = 0; i < MapTiles[CellCoordinate.X][CellCoordinate.Y].Num(); i++)
	{
		if (PossibleNeighbors.Find(MapTiles[CellCoordinate.X][CellCoordinate.Y][i]) == INDEX_NONE)
		{
			bNeighborlistChanged = true;

			//Remove debug meshes that no longer are relevant
			if (bSpawnDebugMeshes)
			{
				DebugMeshes[CellCoordinate.X][CellCoordinate.Y][i]->SetStaticMesh(0);
			}
		}
		else
		{
			TempArray.AddUnique(MapTiles[CellCoordinate.X][CellCoordinate.Y][i]);
		}
	}

	if (bNeighborlistChanged && TempArray.Num() != 0)
	{
		//Replace maptiles array
		MapTiles[CellCoordinate.X][CellCoordinate.Y].Empty();
		MapTiles[CellCoordinate.X][CellCoordinate.Y].Append(TempArray);
		ReplaceMapTilesTimer += FPlatformTime::Seconds() - FunctionTimer;
		return true;
	}
	ReplaceMapTilesTimer += FPlatformTime::Seconds() - FunctionTimer;
	return false;
}

void AWFC_Spawner::AddCellToPropagationstack(FVector2d CellToBeAdded, TArray<FVector2d> PropagatedCells)
{
	double functionTimer = FPlatformTime::Seconds();
	//Don't add cell if it's out of bounds
	if (CellToBeAdded.X >= MapSize || CellToBeAdded.X < 0 || CellToBeAdded.Y >= MapSize || CellToBeAdded.Y < 0)
	{
		return;
	}

	//Don't add cell if it already has been propagated in this cycle
	if (PropagatedCells.Find(CellToBeAdded) == INDEX_NONE)
	{
		PropagationCoordinateStack.AddUnique(CellToBeAdded);
	}
	AddCellToPropagationStackTimer += FPlatformTime::Seconds() - functionTimer;
}

void AWFC_Spawner::RenderCurrentCellMeshes(FVector2d CellCoordinate)
{
	double FunctionTimer = FPlatformTime::Seconds();
	//Render Meshes
	if (bSpawnDebugMeshes)
	{
		for (int32 i = 0; i < Prototypes.Num(); i++)
		{
			DebugMeshes[CellCoordinate.X][CellCoordinate.Y][i]->SetStaticMesh(0);	
		}
	}
	FinalMeshes[CellCoordinate.X][CellCoordinate.Y]->SetStaticMesh(Prototypes[MapTiles[CellCoordinate.X][CellCoordinate.Y][0]].Mesh);
	FinalMeshes[CellCoordinate.X][CellCoordinate.Y]->SetRelativeRotation(FRotator(0, Prototypes[MapTiles[CellCoordinate.X][CellCoordinate.Y][0]].Rotation * 90, 0));
	RenderCurrentCellMeshesTimer += FPlatformTime::Seconds() - FunctionTimer;
}

bool AWFC_Spawner::CheckIfFullyCollapsed()
{
	double functionTimer = FPlatformTime::Seconds();
	bool bFullyCollapsedYet = true;

	for (int32 i = 0; i < MapSize; i++)
	{
		for (int32 j = 0; j < MapSize; j++)
		{
			if (MapTiles[i][j].Num() != 1)
			{
				bFullyCollapsedYet = false;
			}
		}
	}
	CheckIfFullyCollapsedTimer += FPlatformTime::Seconds() - functionTimer;
	return bFullyCollapsedYet;
}
