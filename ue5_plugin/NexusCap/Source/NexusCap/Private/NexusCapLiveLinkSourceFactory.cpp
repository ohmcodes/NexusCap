// Copyright NexusCap Contributors. All Rights Reserved.

#include "NexusCapLiveLinkSourceFactory.h"
#include "NexusCapLiveLinkSource.h"
#include "NexusCapTypes.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "NexusCap"

// ---------------------------------------------------------------------------
// Simple creation panel widget
// ---------------------------------------------------------------------------

class SNexusCapSourcePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNexusCapSourcePanel) {}
        SLATE_ARGUMENT(FOnLiveLinkSourceCreated, OnSourceCreated)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        OnSourceCreated = InArgs._OnSourceCreated;
        Port = NEXUSCAP_DEFAULT_PORT;

        ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(4.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PortLabel", "UDP Listen Port:"))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                [
                    SNew(SSpinBox<int32>)
                    .MinValue(1024)
                    .MaxValue(65535)
                    .Value_Lambda([this]() { return Port; })
                    .OnValueChanged_Lambda([this](int32 NewValue) { Port = NewValue; })
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(4.f)
            [
                SNew(SButton)
                .Text(LOCTEXT("ConnectButton", "Connect"))
                .OnClicked(this, &SNexusCapSourcePanel::OnConnectClicked)
            ]
        ];
    }

private:
    FReply OnConnectClicked()
    {
        // Build connection string: just the port number as a string
        const FString ConnString = FString::FromInt(Port);

        TSharedPtr<ILiveLinkSource> NewSource =
            MakeShared<FNexusCapLiveLinkSource>(Port);

        OnSourceCreated.ExecuteIfBound(NewSource, ConnString);
        return FReply::Handled();
    }

    FOnLiveLinkSourceCreated OnSourceCreated;
    int32                    Port = NEXUSCAP_DEFAULT_PORT;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

FText UNexusCapLiveLinkSourceFactory::GetSourceDisplayName() const
{
    return LOCTEXT("SourceName", "NexusCap IMU");
}

FText UNexusCapLiveLinkSourceFactory::GetSourceTooltip() const
{
    return LOCTEXT("SourceTooltip",
        "Receives IMU quaternion data from NexusCap ESP32 hardware "
        "via UDP for real-time skeletal animation in Live Link.");
}

TSharedPtr<SWidget> UNexusCapLiveLinkSourceFactory::BuildCreationPanel(
    FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const
{
    return SNew(SNexusCapSourcePanel)
        .OnSourceCreated(OnLiveLinkSourceCreated);
}

TSharedPtr<ILiveLinkSource> UNexusCapLiveLinkSourceFactory::CreateSource(
    const FString& ConnectionString) const
{
    int32 Port = FCString::Atoi(*ConnectionString);
    if (Port <= 0) Port = NEXUSCAP_DEFAULT_PORT;
    return MakeShared<FNexusCapLiveLinkSource>(Port);
}

#undef LOCTEXT_NAMESPACE
