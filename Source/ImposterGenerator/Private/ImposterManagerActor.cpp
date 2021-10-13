// malosgao presents.


#include "ImposterManagerActor.h"

#include "EngineUtils.h"

AImposterManagerActor::AImposterManagerActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AImposterManagerActor::DebugCaptureDirections()
{
	check(Core);
	Core->DebugCaptureDirections();
}

void AImposterManagerActor::DebugCaptureBounds()
{
	check(Core);
	Core->DebugCaptureBounds();
}

void AImposterManagerActor::E_Preview()
{
	check(Core);
	Core->PreviewImposter();
}

void AImposterManagerActor::CopyDebugFromCore()
{
	CapturedTexMap = Core->CapturedTexMap;
	MiddleRT = Core->MiddleRT;
}

void AImposterManagerActor::C_Capture()
{
	check(Core);
	Core->Capture();
	CopyDebugFromCore();
}

void AImposterManagerActor::D_Generate()
{
	check(Core);
	Core->GenerateImposter(true);
}

void AImposterManagerActor::A_AddSceneActors()
{
	CapturedActors.Reset();
	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		if (Core->CanAddActorToCapture(Actor))
		{
			CapturedActors.Add(Actor); // add to local array, not core.
		}
	}
	UE_LOG(LogImposterGenerator, Display, TEXT("%d actors added."), CapturedActors.Num());
}

void AImposterManagerActor::B_InitCore()
{
	Core = NewObject<UImposterCore>(this, TEXT("Imposter Core"));
	Core->Init(Settings, CapturedActors, TEXT("Imposter"));
	CopyDebugFromCore();
}

