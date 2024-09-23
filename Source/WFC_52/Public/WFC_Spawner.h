// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dom/JsonObject.h"
#include "WFC_Spawner.generated.h"

class FJsonObject;

//Struct to store the JSON data
USTRUCT(BlueprintType)
struct FWFCPrototypes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PrototypeID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Mesh_Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RotationType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rotation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString X_Plus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Y_Minus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString X_Minus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Y_Plus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> Neighbors_Y_Minus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> Neighbors_X_Plus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> Neighbors_Y_Plus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> Neighbors_X_Minus;
};

UCLASS()
class WFC_52_API AWFC_Spawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWFC_Spawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintReadWrite)
	TArray<FWFCPrototypes> Prototypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MapSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UStaticMesh*> Meshes;

	TArray<TArray<TArray<int32>>> MapTiles;
	TArray<TArray<TArray<UStaticMeshComponent*>>> DebugMeshes;
	TArray<TArray<UStaticMeshComponent*>> FinalMeshes;
	TArray<FVector2d> PropagationCoordinateStack;
	bool bFinishedWfc = false;
	bool bSpawnDebugMeshes = false;

	UFUNCTION(BlueprintCallable)
	void StartWfcGeneration();
	UFUNCTION(BlueprintCallable)
	void LoadJson(FWFCPrototypes PrototypeData);

	void InitializeMap();
	
	void MainLoop();

	void CollapseLowestEntropyCell();

	void Propagate(FVector2d CellCoordinate);

	bool ReplaceMapTiles(TArray<int32> PossibleNeighbors, FVector2d CellCoordinate);

	void AddCellToPropagationstack(FVector2d CellToBeAdded, TArray<FVector2d> PropagatedCells);

	void RenderCurrentCellMeshes(FVector2d CellCoordinate);

	bool CheckIfFullyCollapsed();

	//Timer
	double mainTimer = 0;
	double initializeMapTimer = 0;
	double collapseLowestEntropyCellTimer = 0;
	double propagateTimer = 0;
	double replaceMapTilesTimer = 0;
	double addCellToPropagationStackTimer = 0;
	double renderCurrentCellMeshesTimer = 0;
	double checkIfFullyCollapsedTimer = 0;
	
	
	FTimerHandle MemberTimerHandle;
		
};
