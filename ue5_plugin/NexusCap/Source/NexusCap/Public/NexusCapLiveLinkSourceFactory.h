// Copyright NexusCap Contributors. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "LiveLinkSourceFactory.h"
#include "NexusCapLiveLinkSourceFactory.generated.h"

/**
 * UNexusCapLiveLinkSourceFactory
 *
 * Registers NexusCap as a source option in the Live Link panel.
 * Exposes a minimal editor widget that lets the user configure
 * the UDP listen port before connecting.
 */
UCLASS()
class NEXUSCAP_API UNexusCapLiveLinkSourceFactory
    : public ULiveLinkSourceFactory
{
    GENERATED_BODY()

public:
    // ULiveLinkSourceFactory interface
    virtual FText GetSourceDisplayName() const override;
    virtual FText GetSourceTooltip() const override;
    virtual EMenuType GetMenuType() const override
    {
        return EMenuType::SubPanel;
    }
    virtual TSharedPtr<SWidget> BuildCreationPanel(
        FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const override;
    virtual TSharedPtr<ILiveLinkSource> CreateSource(
        const FString& ConnectionString) const override;
};
