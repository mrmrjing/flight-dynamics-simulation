#include "FlightSimPluginBPLibrary.h"
#include "FlightSimPlugin.h"
#include "Components/StaticMeshComponent.h" // Include the header for StaticMeshComponent

using namespace std;

const float GRAVITY = 9.81f;
const float AIR_DENSITY = 1.225f;

DEFINE_LOG_CATEGORY(LogFlightSimPlugin);

UFlightSimPluginBPLibrary::UFlightSimPluginBPLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void UFlightSimPluginBPLibrary::FlightSimulation(AActor* actor, UStaticMeshComponent* StaticMesh, bool enabled, float wingArea, float liftCoefficient, float dragCoefficient, float frontalArea, float CurrentThrust)
{
    if (!actor)
    {
        UE_LOG(LogFlightSimPlugin, Warning, TEXT("Invalid actor passed to FlightSimulation function"));
        return;
    }

    if (!StaticMesh)
    {
        UE_LOG(LogFlightSimPlugin, Warning, TEXT("Invalid static mesh"));
        return;
    }

    // Get mass
    float mass = StaticMesh->GetMass();
    
    UWorld* World = actor->GetWorld();
    if (!World)
    {
        UE_LOG(LogFlightSimPlugin, Warning, TEXT("Invalid world"));
        return;
    }
    
    // Get Delta time from world
    float DeltaTime = World->GetDeltaSeconds();
    
    // Get the velocity vector
    FVector VelocityVector = StaticMesh->GetComponentVelocity();

    // Set the Z component to 0
    VelocityVector.Z = 0.0f;
    VelocityVector.Y = 0.0f;

    // Calculate the magnitude of the modified velocity vector
    float VelocityMagnitude = VelocityVector.Size() / 100;
    
    // Calculating lift, drag, and weight based on the document
    float lift = 0.5f * AIR_DENSITY * VelocityMagnitude * VelocityMagnitude * wingArea * liftCoefficient;
    float drag = 0.5f * AIR_DENSITY * VelocityMagnitude * VelocityMagnitude * frontalArea * dragCoefficient;
    float weight = mass * GRAVITY;

    // Convert angle for pitch and roll from degree into radians
    FRotator ActorRotation = StaticMesh->GetComponentRotation();
    float AlphaRad = FMath::DegreesToRadians(ActorRotation.Roll);
    float PhiRad = FMath::DegreesToRadians(ActorRotation.Pitch);
    
    // Calulcate forces based on the physics equations highlighted in yellow 
    float Fvert = lift * FMath::Cos(AlphaRad) * FMath::Cos(PhiRad) + CurrentThrust * FMath::Sin(AlphaRad) - drag * FMath::Sin(AlphaRad) - weight;
    float Fforward = CurrentThrust * FMath::Cos(AlphaRad) - lift * FMath::Sin(AlphaRad) - drag * FMath::Cos(AlphaRad);
    float Fsideways = lift * FMath::Cos(AlphaRad) * FMath::Sin(PhiRad);

    // Logs 
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("Fvert is %f"), Fvert);
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("Fforward is %f"), Fforward);
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("Fsideways is %f"), Fsideways);
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("Lift is %f"), lift);
    //UE_LOG(LogFlightSimPlugin, Warning, TEXT("Velocity Vector is %f, %f, %f"), VelocityVector.X, VelocityVector.Y, VelocityVector.Z);
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("Velocity magnitude is %f"), VelocityMagnitude);

    // Apply the calculated forces to the actor
    FVector ForceToAdd = FVector( Fforward, Fsideways, Fvert);
    FVector ForwardVector = -StaticMesh->GetForwardVector();
    UE_LOG(LogFlightSimPlugin, Warning, TEXT("ForwardVector is %f, %f, %f"), ForwardVector.X, ForwardVector.Y, ForwardVector.Z);
    FVector ForwardVectorReference = ForwardVector;
    ForwardVectorReference.Z = 0;
    ForwardVectorReference = ForwardVectorReference / ForwardVectorReference.Size();
    FVector SidewaysVector = ForwardVectorReference.Cross(FVector(0, 0, 1));
    FVector VerticalVector = (ForwardVector.Cross(SidewaysVector));
    VerticalVector.Z = -VerticalVector.Z;

    StaticMesh->AddForce(Fforward * SidewaysVector, NAME_None, true);
    StaticMesh->AddForce(Fsideways * ForwardVector, NAME_None, true);
    StaticMesh->AddForce(Fvert * VerticalVector , NAME_None, true);

}
