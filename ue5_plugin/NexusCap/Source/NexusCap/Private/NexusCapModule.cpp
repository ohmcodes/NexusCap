// Copyright NexusCap Contributors. All Rights Reserved.

#include "NexusCapModule.h"
#include "Modules/ModuleManager.h"

void FNexusCapModule::StartupModule()
{
    // Nothing to initialize at module load time.
    // The Live Link source is created on demand via UNexusCapLiveLinkSourceFactory.
}

void FNexusCapModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FNexusCapModule, NexusCap)
